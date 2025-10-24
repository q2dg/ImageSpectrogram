/*
 * imageSpectrogram.c
 *
 * Convert an image (PNG or JPEG) into a WAV file spectrogram-like sound.
 * Equivalent logic to imageSpectrogram.pl (Perl version).
 *
 * Build:
 *   gcc -o imageSpectrogram imageSpectrogram.c -lm -lpng -ljpeg
 *
 * Usage:
 *   ./imageSpectrogram input.png [output.wav]
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <png.h>
#include <jpeglib.h>

#define SAMPLE_RATE 44100
#define BITS_PER_SAMPLE 16
#define NUM_CHANNELS 1
#define TWO_PI (2.0 * M_PI)
#define COLUMN_DURATION 0.2

// WAV header structure
typedef struct {
    char riff[4];
    uint32_t chunk_size;
    char wave[4];
    char fmt[4];
    uint32_t subchunk1_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    char data[4];
    uint32_t data_size;
} WAVHeader;

// Forward declarations
int read_png(const char *filename, unsigned char **rgb, int *width, int *height);
int read_jpeg(const char *filename, unsigned char **rgb, int *width, int *height);
void write_wav_header(FILE *f, uint32_t data_size);
void finalize_wav_header(FILE *f, uint32_t total_samples);
void add_sine(FILE *f, double *freqs, double *amps, int count);
int ends_with(const char *s, const char *suffix);

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s input_image [output.wav]\n", argv[0]);
        return 1;
    }

    const char *input = argv[1];
    char output[256];
    if (argc >= 3) {
        strncpy(output, argv[2], sizeof(output));
    } else {
        snprintf(output, sizeof(output), "%s.wav", input);
    }

    unsigned char *rgb = NULL;
    int width = 0, height = 0;
    int ok = 0;

    if (ends_with(input, ".png") || ends_with(input, ".PNG")) {
        ok = read_png(input, &rgb, &width, &height);
    } else if (ends_with(input, ".jpg") || ends_with(input, ".jpeg") || ends_with(input, ".JPG") || ends_with(input, ".JPEG")) {
        ok = read_jpeg(input, &rgb, &width, &height);
    } else {
        fprintf(stderr, "Unsupported image format (PNG and JPEG only)\n");
        return 1;
    }

    if (!ok) {
        fprintf(stderr, "Failed to read image: %s\n", input);
        return 1;
    }

    FILE *wav = fopen(output, "wb");
    if (!wav) {
        perror("fopen");
        free(rgb);
        return 1;
    }

    write_wav_header(wav, 0); // placeholder

    int total_samples = 0;
    for (int x = 0; x < width; x++) {
        double freqs[2048];
        double amps[2048];
        int count = 0;

        for (int y = 0; y < height; y++) {
            int idx = (y * width + x) * 3;
            double r = rgb[idx];
            double g = rgb[idx + 1];
            double b = rgb[idx + 2];

            if (!(r > 10 || (g > 10 && b > 10)))
                continue;

            double c = 4.25 - 4.25 * (r + g + b) / (256.0 * 3.0);
            double freq = floor(22000.0 - ((double)(y + 1) / (double)(height + 1)) * 22000.0);

            freqs[count] = freq;
            amps[count] = c;
            count++;
        }

        add_sine(wav, freqs, amps, count);
        total_samples += (int)(COLUMN_DURATION * SAMPLE_RATE);
        fprintf(stderr, "\r%3d%%", (int)(100.0 * x / width));
    }

    fprintf(stderr, "\r100%%\n");

    finalize_wav_header(wav, total_samples);
    fclose(wav);
    free(rgb);

    printf("WAV file written: %s\n", output);
    return 0;
}

// ---- Image loading (PNG) ----
int read_png(const char *filename, unsigned char **out, int *width, int *height) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return 0;

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) { fclose(fp); return 0; }

    png_infop info = png_create_info_struct(png);
    if (!info) { png_destroy_read_struct(&png, NULL, NULL); fclose(fp); return 0; }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return 0;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    *width = png_get_image_width(png, info);
    *height = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    if (bit_depth == 16)
        png_set_strip_16(png);

    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    if (color_type == PNG_COLOR_TYPE_RGB_ALPHA || color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_strip_alpha(png);

    png_read_update_info(png, info);

    int rowbytes = png_get_rowbytes(png, info);
    unsigned char *image_data = malloc(rowbytes * (*height));
    if (!image_data) { fclose(fp); png_destroy_read_struct(&png, &info, NULL); return 0; }

    png_bytep *row_pointers = malloc(sizeof(png_bytep) * (*height));
    for (int y = 0; y < *height; y++)
        row_pointers[y] = image_data + y * rowbytes;

    png_read_image(png, row_pointers);
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
    free(row_pointers);

    *out = image_data;
    return 1;
}

// ---- Image loading (JPEG) ----
int read_jpeg(const char *filename, unsigned char **out, int *width, int *height) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return 0;

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, fp);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    *width = cinfo.output_width;
    *height = cinfo.output_height;
    int channels = cinfo.output_components;

    unsigned long data_size = (*width) * (*height) * 3;
    unsigned char *data = malloc(data_size);
    if (!data) { jpeg_destroy_decompress(&cinfo); fclose(fp); return 0; }

    unsigned char *rowptr[1];
    while (cinfo.output_scanline < cinfo.output_height) {
        rowptr[0] = data + 3 * (*width) * cinfo.output_scanline;
        jpeg_read_scanlines(&cinfo, rowptr, 1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(fp);
    *out = data;
    return 1;
}

// ---- WAV handling ----
void write_wav_header(FILE *f, uint32_t data_size) {
    WAVHeader header;
    memcpy(header.riff, "RIFF", 4);
    header.chunk_size = 36 + data_size;
    memcpy(header.wave, "WAVE", 4);
    memcpy(header.fmt, "fmt ", 4);
    header.subchunk1_size = 16;
    header.audio_format = 1;
    header.num_channels = NUM_CHANNELS;
    header.sample_rate = SAMPLE_RATE;
    header.bits_per_sample = BITS_PER_SAMPLE;
    header.byte_rate = SAMPLE_RATE * NUM_CHANNELS * BITS_PER_SAMPLE / 8;
    header.block_align = NUM_CHANNELS * BITS_PER_SAMPLE / 8;
    memcpy(header.data, "data", 4);
    header.data_size = data_size;
    fwrite(&header, sizeof(header), 1, f);
}

void finalize_wav_header(FILE *f, uint32_t total_samples) {
    uint32_t data_size = total_samples * NUM_CHANNELS * (BITS_PER_SAMPLE / 8);
    fseek(f, 4, SEEK_SET);
    uint32_t chunk_size = 36 + data_size;
    fwrite(&chunk_size, 4, 1, f);
    fseek(f, 40, SEEK_SET);
    fwrite(&data_size, 4, 1, f);
}

void add_sine(FILE *f, double *freqs, double *amps, int count) {
    int samples = (int)(COLUMN_DURATION * SAMPLE_RATE);
    for (int pos = 0; pos < samples; pos++) {
        double t = (double)pos / SAMPLE_RATE;
        double val = 0.0;
        for (int i = 0; i < count; i++) {
            val += sin(TWO_PI * freqs[i] * t) * 10.0 / pow(10.0, amps[i]);
        }
        if (count > 0) val /= count;
        int16_t sample = (int16_t)(val * 32767.0);
        fwrite(&sample, sizeof(int16_t), 1, f);
    }
}

int ends_with(const char *s, const char *suffix) {
    if (!s || !suffix) return 0;
    size_t lenstr = strlen(s);
    size_t lensuffix = strlen(suffix);
    if (lensuffix > lenstr) return 0;
    return strcasecmp(s + lenstr - lensuffix, suffix) == 0;
}

/*
 * imageSpectrogram_simple.c
 *
 * Simplified version of imageSpectrogram: reads a PNG and outputs WAV.
 * Dependencies: libpng, libm
 *
 * Build:
 *   gcc -o imageSpectrogram imageSpectrogram_simple.c -lm -lpng
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <png.h>

#define SAMPLE_RATE 44100
#define BITS_PER_SAMPLE 16
#define TWO_PI (2.0 * M_PI)
#define COLUMN_DURATION 0.2

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

void write_wav_header(FILE *f, uint32_t data_size) {
    WAVHeader h = {
        "RIFF", 36 + data_size, "WAVE", "fmt ", 16, 1, 1, SAMPLE_RATE,
        SAMPLE_RATE * 2, 2, BITS_PER_SAMPLE, "data", data_size
    };
    fwrite(&h, sizeof(h), 1, f);
}

void finalize_wav_header(FILE *f, uint32_t total_samples) {
    uint32_t data_size = total_samples * 2;
    fseek(f, 4, SEEK_SET);
    uint32_t chunk = 36 + data_size;
    fwrite(&chunk, 4, 1, f);
    fseek(f, 40, SEEK_SET);
    fwrite(&data_size, 4, 1, f);
}

int read_png(const char *filename, unsigned char **out, int *width, int *height) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return 0;

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) { fclose(fp); return 0; }
    png_infop info = png_create_info_struct(png);
    if (!info) { png_destroy_read_struct(&png, NULL, NULL); fclose(fp); return 0; }
    if (setjmp(png_jmpbuf(png))) { png_destroy_read_struct(&png, &info, NULL); fclose(fp); return 0; }

    png_init_io(png, fp);
    png_read_info(png, info);

    *width = png_get_image_width(png, info);
    *height = png_get_image_height(png, info);
    png_set_expand(png);
    png_set_strip_16(png);
    png_set_gray_to_rgb(png);
    png_set_strip_alpha(png);
    png_read_update_info(png, info);

    int rowbytes = png_get_rowbytes(png, info);
    *out = malloc(rowbytes * (*height));
    png_bytep *rows = malloc(sizeof(png_bytep) * (*height));
    for (int y = 0; y < *height; y++) rows[y] = *out + y * rowbytes;
    png_read_image(png, rows);
    free(rows);
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
    return 1;
}

void add_sine(FILE *f, double *freqs, double *amps, int count) {
    int samples = (int)(COLUMN_DURATION * SAMPLE_RATE);
    for (int pos = 0; pos < samples; pos++) {
        double t = (double)pos / SAMPLE_RATE;
        double v = 0.0;
        for (int i = 0; i < count; i++)
            v += sin(TWO_PI * freqs[i] * t) * 10.0 / pow(10.0, amps[i]);
        if (count > 0) v /= count;
        int16_t s = (int16_t)(v * 32767);
        fwrite(&s, sizeof(s), 1, f);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "Usage: %s image.png [output.wav]\n", argv[0]); return 1; }

    const char *input = argv[1];
    const char *output = (argc > 2) ? argv[2] : "out.wav";

    unsigned char *img = NULL;
    int w = 0, h = 0;
    if (!read_png(input, &img, &w, &h)) { fprintf(stderr, "Error reading PNG.\n"); return 1; }

    FILE *wav = fopen(output, "wb");
    if (!wav) { perror("fopen"); free(img); return 1; }
    write_wav_header(wav, 0);

    int total_samples = 0;
    for (int x = 0; x < w; x++) {
        double f[2048], a[2048]; int n = 0;
        for (int y = 0; y < h; y++) {
            unsigned char *p = img + (y * w + x) * 3;
            double r = p[0], g = p[1], b = p[2];
            if (!(r > 10 || (g > 10 && b > 10))) continue;
            double c = 4.25 - 4.25 * (r + g + b) / (256.0 * 3.0);
            double freq = 22000.0 - ((double)(y + 1) / (h + 1)) * 22000.0;
            f[n] = freq; a[n] = c; n++;
        }
        add_sine(wav, f, a, n);
        total_samples += (int)(COLUMN_DURATION * SAMPLE_RATE);
        fprintf(stderr, "\r%3d%%", (int)(100.0 * x / w));
    }

    fprintf(stderr, "\r100%%\n");
    finalize_wav_header(wav, total_samples);
    fclose(wav);
    free(img);
    printf("WAV written: %s\n", output);
    return 0;
}


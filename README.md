# ImageSpectrogram
It converts an image to a sound clip. The spectrogram of the clip reproduces that image

# Instructions
+ Install dependencies 
  In Ubuntu: sudo apt install build-essential libpng-dev libjpeg-dev
  In Fedora: sudo dnf install gcc libpng-devel libjpeg-turbo-devel
+ Compile: gcc -o imageSpectrogram imageSpectrogram.c -lm -lpng -ljpeg
+ Run: ./imageSpectrogram imageSpectrogram.c originalImage.png [output.wav]

# Notes
Output is a mono 16bits 44100MHz PCM audio file. If not specified output file name, by default is "out.wav"
Code "onlyPng" just reads PNG images; the other one (more complex), reads PNG, JPEG and GIF images

*Project inspired by https://github.com/plurSKI/imageSpectrogram but rewritten in C in order to enhance portability*

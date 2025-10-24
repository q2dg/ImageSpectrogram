# ImageSpectrogram
It converts an image to a sound clip. The spectrogram of the clip reproduces that image

# Instructions
+ Install dependencies 
  + In Ubuntu: *sudo apt install build-essential libpng-dev libjpeg-dev*
  + In Fedora: *sudo dnf install gcc libpng-devel libjpeg-turbo-devel*
+ Compile: *gcc -o imageSpectrogram imageSpectrogram.c -lm -lpng -ljpeg*
+ Run: *./imageSpectrogram imageSpectrogram.c originalImage.png [output.wav]*

# Notes
+ Output is a mono 16bits 44100MHz PCM audio file. If not specified output file name, by default is "out.wav"
+ Code "onlyPng" just reads PNG images (it's compiled the same as the other one but without -ljpeg argument); the other one reads PNG, JPEG and GIF images (but is more complex)
+ For a better result, it's advisable original image was black and white
+ To "see" the spectrogram there are many ways: one can be opening the wav file with the Audacity sound editor and then choose "Spectrogram" option from menu shown when clicking on the three dots buttons belonging to the only track. In "Spectrogram settings" it's possible to change the scale to "Linear" to a betther visualization

*Project inspired by https://github.com/plurSKI/imageSpectrogram but rewritten in C in order to enhance portability*

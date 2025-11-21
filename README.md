# ImageSpectrogram
It converts an image to a sound clip. Then, the spectrogram of that clip will show that image

To "see" the hidden image "drawn" in a spectrogram you can open the wav file with the Audacity sound editor (for instance) and then choose the "Spectrogram" option from menu shown when clicking on the three dots button in the left zone just before the beginning of the track. In "Spectrogram settings" option form same menu it's possible to change the scale to "Linear" to a better visualization

# Instructions
+ Install dependencies 
  + In Ubuntu: *sudo apt install build-essential libpng-dev libjpeg-dev*
  + In Fedora: *sudo dnf install gcc libpng-devel libjpeg-turbo-devel*
+ Compile: *gcc -o imageSpectrogram imageSpectrogram.c -lm -lpng -ljpeg*
+ Run: *./imageSpectrogram originalImage.png [output.wav]*

# Notes
+ Output is a mono 16bits 44100MHz PCM audio file. If not specified output file name, by default is "out.wav"
+ Code "onlyPng" just reads PNG images (it's compiled the same as the other one but without -ljpeg argument); the other one reads PNG, JPEG and GIF images (but is more complex)
+ For a better result, it's advisable original image is black and white
  
*Project inspired by https://github.com/plurSKI/imageSpectrogram but rewritten in C in order to enhance portability*

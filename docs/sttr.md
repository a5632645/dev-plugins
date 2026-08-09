# STTR

STTR is based on [CCRMA's work](https://ccrma.stanford.edu/~hskim08/sttr/index.html), combined with my other experimental results.  
It is a granular effect, but many grain parameters are fixed and not adjustable.  
Basically, it produces additional harmonics on top of the original spectrum; their frequencies are controlled by Hop, and their amplitudes by the window function.  
Through reverse/forward playback, the additional harmonic frequencies move inversely/forwardly relative to the original frequency, similar to piwarp / ring modulator.

![gui](img/sttr_gui.png)

## features

short-time time reversal (or not)  
simple formant shifting (behaves like formant shifting with a short hop, and more like pitch shifting with a long hop)  
more flexible window function to shape the spectrum

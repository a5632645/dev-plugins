
# Green Vocoder  

multiple algorithms vocoder

## signal-flow
NEED REDRAW

## features

- Leaky Burg LPC
- Block Burg LPC
- MFCC(not a real)
- Channel Vocoder
- STFT Vocoder

> [!WARNING]
> The channel vocoder volume is not balanced.

> [!WARNING]
> The Elliptic filter bank is using non-parameter-smoothing biquad filters. Do not do fast modulation or suddenly jump or it will generate high volume click.  

## GUI(master version)

![GUI](gui.png)

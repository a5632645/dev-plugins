#ifndef Helmholtz_H
#define Helmholtz_H


/***********************************************************************

Class Helmholtz implements a period-length detector using Philip McLeod's
Specially Normalized AutoCorrelation function (SNAC).

Function iosamples() takes a pointer to a buffer with n signal input samples 
and a pointer to a buffer where n output samples are stored, 
representing the SNAC function.

Via function setframesize(), analysis frame size can be set to 
128, 256, 512, 1024 or 2048 samples. Default is 1024.

With setoverlap(), analysis frames can be set to overlap each other 
with factor 1, 2, 4 or 8. Default is 1.

Function setbias() sets a bias which favours small lags over large lags in 
the period detection, thereby avoiding low-octave jumps. Default is 0.2

Function setminRMS() is used as a sensitivity setting. Default is RMS 0.003.

With function getperiod(), the last detected period length is returned 
as number of samples with a possible fraction (floating point format). 

Function getfidelity() returns a value between 0. and 1. indicating
to which extent the input signal is periodic. A fidelity of ~0.95 can
be considered to indicate a periodic signal.

Class Helmholtz needs mayer_realfft() and mayer_realifft() or similar
fft functions. Note that Ron Mayer's functions for real fft have a
peculiar organisation of imaginary coefficients (reversed order, sign flipped).

Class Helmholtz uses float for float or double. Depending on the context
where the class is used, you may need to define float. If used with
PD or MaxMsp, it is already defined.

***********************************************************************

Licensed under three-clause BSD license.
 
Katja Vetter, Feb 2012.

***********************************************************************/

/* This section includes the Pure Data API header. If you build Helmholtz
against another DSP framework, you need to define float, and you need to
include Ron Mayer's fft or similar functionality. */

/***********************************************************************/

#define DEFFRAMESIZE 1024       // default analysis framesize
#define DEFOVERLAP 1            // default overlap
#define DEFBIAS 0.2             // default bias
#define DEFMINRMS 0.003         // default minimum RMS 
#define SEEK 0.85               // seek-length as ratio of framesize

#include <span>
#include <cmath>
#include "qwqdsp/spectral/oouras_real_fft.hpp"
#include "qwqdsp/pitch/pitch.hpp"

namespace qwqdsp::pitch {
class Helmholtz
{
public:
    void Init(float fs, int block_size) {
        setframesize(block_size);
        biasfactor = DEFBIAS;
        
        inputbuf.resize(framesize);
        inputbuf2.resize(framesize);
        processbuf.resize(framesize*2+2);
        fs_ = fs;
        periodindex = 0;
        periodlength = 0.;
        fidelity = 0.;
        minrms = DEFMINRMS;

        SetMinPitch(min_pitch_);
        SetMaxPitch(max_pitch_);
    }
    
    void Process(std::span<float const> block) noexcept {
        std::copy_n(block.data(), block.size(), inputbuf.data());
        analyzeframe();
    }

    Pitch GetPitch() const noexcept {
        return pitch_;
    }

    void SetMinPitch(float min_val) noexcept {
        min_pitch_ = min_val;
        max_bin_ = static_cast<int>(std::round(fs_ / min_val));
        max_bin_ = std::min(max_bin_, framesize);
    }

    void SetMaxPitch(float max_val) noexcept {
        max_pitch_ = max_val;
        min_bin_ = static_cast<int>(std::round(fs_ / max_val));
        min_bin_ = std::max(min_bin_, 2);
    }

    void setframesize(int frame)
    {
        if(!((frame==128)|(frame==256)|(frame==512)|(frame==1024)|(frame==2048)))
            frame = DEFFRAMESIZE;
        framesize = frame;
        
        inputbuf.resize(framesize);
        inputbuf2.resize(framesize);
        processbuf.resize(framesize*2+2);
    }

    void setbias(float bias)
    {
        if(bias > 1.) bias = 1.;
        if(bias < 0.) bias = 0.;
        biasfactor = bias;
    }

    void setminRMS(float rms)
    {
        if(rms > 1.) rms = 1.;
        if(rms < 0.) rms = 0.;
        minrms = rms;
    }

    float getperiod() const
    {
        return(periodlength);
    }

    float getfidelity() const
    {
        return(fidelity);
    }
    
private:
    // procedures
    void analyzeframe()
    {
        float norm = 1. / sqrt(float(framesize * 2));
        
        // copy input to processing buffer
        for(int n=0; n<framesize; n++) processbuf[n] = inputbuf[n] * norm;
        // for(int n=0; n<framesize; n++) processbuf[n] = inputbuf[n];
        
        // copy for normalization function
        for(int n=0; n<framesize; n++) inputbuf2[n] = inputbuf[n];
        
        // zeropadding
        for(int n=framesize; n<(framesize<<1); n++) processbuf[n] = 0.;
        
        // call analysis procedures
        autocorrelation();
        normalize();
        pickpeak();
        periodandfidelity();
    }

    void autocorrelation()
    {
        int n;
        int fftsize = framesize * 2;
        
        
        // REALFFT(fftsize, processbuf);
        qwqdsp::spectral::OourasRealFFT fft;
        fft.Init(fftsize);
        fft.FFT(processbuf.data(), processbuf.data());
        int num_bins = fftsize / 2 + 1;
        for (int i = 0; i < num_bins; ++i) {
            float re = processbuf[2*i];
            float im = processbuf[2*i+1];
            processbuf[2*i]=(re*re+im*im)*fftsize;
            processbuf[2*i+1]=0;
        }
        fft.IFFT(processbuf.data(), processbuf.data());
    }

    void normalize()
    {
        int n, mask = framesize - 1;
        int seek = framesize * SEEK;
        float signal1, signal2;
        
        // minimum RMS implemented as minimum autocorrelation at index 0
        // effectively this means possible white noise addition
        float rms = minrms / sqrt(1. / (float)framesize);
        float minrzero = rms * rms;
        float rzero = processbuf[0];
        if(rzero < minrzero) rzero = minrzero;
        double normintegral = rzero * 2.;
        
        // normalize biased autocorrelation function
        processbuf[0] = 1.;
        for(n=1; n<seek; n++)
        {
            signal1 = inputbuf2[n-1];
            signal2 = inputbuf2[framesize-n];
            normintegral -= (double)(signal1 * signal1 + signal2 * signal2);
            processbuf[n] /= (float)normintegral * 0.5;
        }
        
        // flush instable function tail
        for(n = seek; n<framesize; n++) processbuf[n] = 0.;
    }

    void pickpeak()
    {
        int n, peakindex=0;
        int seek = max_bin_;
        float maxvalue = 0.;
        float bias = biasfactor / (float)framesize;    // user-controlled bias
        float realpeak;
        
        // skip main lobe
        for(n=min_bin_; n<seek; n++)
        {
            if(processbuf[n] < 0.) break;
        }
        
        // find interpolated / biased maximum in specially normalized autocorrelation function
        // interpolation finds the 'real maximum'
        // biasing favours the first candidate
        for(; n<seek-1; n++)
        {
            if(processbuf[n] > processbuf[n-1]) 
            {
                if(processbuf[n] > processbuf[n+1]) // we have a local peak
                {
                    realpeak = interpolate3max(processbuf.data(), n);
                    
                    if((realpeak * (1. - n * bias)) > maxvalue) 
                    {
                        maxvalue = realpeak;
                        peakindex = n;
                    }
                }
            }
        }
        periodindex = peakindex;
    }

    void periodandfidelity()
    {
        if(periodindex) {
            periodlength = periodindex + interpolate3phase(processbuf.data(), periodindex);
            fidelity = interpolate3max(processbuf.data(), periodindex);
            pitch_.pitch_hz = fs_ / periodlength;
            pitch_.non_period_ratio = 1-fidelity;
        }
        else {
            pitch_.non_period_ratio = 1-fidelity;
        }
    }
    
    // functions
    float interpolate3max(float *buf, int peakindex)
    {
        float realpeak;
        
        float a = buf[peakindex-1];
        float b = buf[peakindex];
        float c = buf[peakindex+1];
        
        realpeak = b + 0.5 * (0.5 * ((c - a) * (c - a))) 
                    / (2 * b - a - c);
        
        return(realpeak);
    }

    float interpolate3phase(float *buf, int peakindex)
    {
        float fraction;
        
        float a = buf[peakindex-1];
        float b = buf[peakindex];
        float c = buf[peakindex+1];
        
        fraction = (0.5 * (c - a)) / ( 2. * b - a - c);
        
        return(fraction);
    }
        
    // buffers
    std::vector<float> inputbuf;
    std::vector<float> inputbuf2;
    std::vector<float> processbuf;
    
    // state variables
    int framesize{};
    int periodindex{};
    float fs_{};
    float periodlength{};
    float fidelity{};
    float biasfactor{};
    float minrms{};

    float min_pitch_{50.0f};
    float max_pitch_{500.0f};
    int min_bin_{};
    int max_bin_{};

    Pitch pitch_;
};
}

#endif // #ifndef Helmholtz_H

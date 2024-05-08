#include "daisy_patch_sm.h"
#include "daisysp.h"

/** These are namespaces for the daisy libraries.
 *  These lines allow us to omit the "daisy::" and "daisysp::" before
 * referencing modules, and functions within the daisy libraries.
 */
using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

DaisyPatchSM hw;
VoctCalibration vc;
Switch lever;


Oscillator modOsc;
VariableShapeOscillator leftOsc, rightOsc;

/** Variables for 1v/oct calibration */
bool isCalibrated = false;
float v1 = 0.2f;
float v3 = 0.6f;


/** Callback for processing and synthesizing audio
 *
 *  The audio buffers are arranged as arrays of samples for each channel.
 *  For example, to access the left input you would use:
 *    in[0][n]
 *  where n is the specific sample.
 *  There are "size" samples in each array.
 *
 *  The default size is very small (just 4 samples per channel). This means the
 * callback is being called at 16kHz.
 *
 *  This size is acceptable for many applications, and provides an extremely low
 * latency from input to output. However, you can change this size by calling
 * hw.SetAudioBlockSize(desired_size). When running complex DSP it can be more
 * efficient to do the processing on larger chunks at a time.
 *
 */


void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    /** First we'll tell the hardware to process the 8 analog inputs */
    hw.ProcessAllControls();

    /** We'll get a nice frequency control knob by using midi note numbers */
    float coarse_tune = hw.GetAdcValue(CV_1);

    /** Using adc 4 as 1V/Oct. */
    float cv_tune = hw.GetAdcValue(CV_5);
    float fm_index = hw.GetAdcValue(CV_6);

    /** Convert those values from midi notes to frequency */
    float base_freq = mtof(12.f + vc.ProcessInput(coarse_tune + cv_tune));


    float base_pw = hw.GetAdcValue(CV_2);
    float left_atten = hw.GetAdcValue(CV_3) * 2.f - 1.f;
    float right_atten = hw.GetAdcValue(CV_4) * 2.f - 1.f;
    float left_amt = hw.GetAdcValue(CV_7);
    float right_amt = hw.GetAdcValue(CV_8);

    // Default to left modulation amount
    if ((right_amt == 0.f) & (left_amt != 0.f) ){
        right_amt = left_amt;
    }
    modOsc.SetFreq(base_freq);


    /** This loop will allow us to process the individual samples of audio */
    for(size_t i = 0; i < size; i++)
    {
        /** FM modulation on the carrier wave */
        float fm_multiple = ((modOsc.Process() + 1.f) / 2.f* fm_index * 7.f) + 1.f;
        float modulated_freq = base_freq * fm_multiple ;
        leftOsc.SetSyncFreq(modulated_freq);
        rightOsc.SetSyncFreq(modulated_freq);
        

        leftOsc.SetWaveshape(base_pw);
        rightOsc.SetWaveshape((1.f-base_pw));
        leftOsc.SetPW(left_atten * left_amt);
        rightOsc.SetPW(right_atten * right_amt);

        /** Outputs hard panned left and right. */
        out[0][i] = leftOsc.Process();
        out[1][i] = rightOsc.Process();

    }
}




int main(void)
{
    /** Initialize the hardware */
    hw.Init();
    lever.Init(hw.B8);

    /** Calibrate 1v/oct */
    if (!isCalibrated) {
        isCalibrated = vc.Record(v1, v3);
        float scale, offset;
        vc.GetData(scale, offset);
    }

    modOsc.Init(hw.AudioSampleRate());
    modOsc.SetWaveform(Oscillator::WAVE_SIN);

    /** Initialize the oscillator modules */
    leftOsc.Init(hw.AudioSampleRate());
    rightOsc.Init(hw.AudioSampleRate());

    leftOsc.SetSync(false);
    rightOsc.SetSync(false);

    /** Start Processing the audio */
    hw.StartAudio(AudioCallback);
    while(1) {}
}








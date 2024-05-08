#include "daisy_patch_sm.h"
#include "daisysp.h"

/** These are namespaces for the daisy libraries.
 *  These lines allow us to omit the "daisy::" and "daisysp::" before
 * referencing modules, and functions within the daisy libraries.
 */
using namespace daisy;
using namespace patch_sm;

/** Our hardware board class handles the interface to the actual DaisyPatchSM
 * hardware. */
DaisyPatchSM patch;
Switch lever;

// helper function to ensure a float is between 0 and 1
float clamp(float x)
{
    if(x < 0.f)
        return 0.f;
    if(x > 1.f)
        return 1.f;
    return x;
}

// helper function to get positive decimal part of a float that could be negative, i.e. -0.1 -> 0.9, 0.9 -> 0.9, 1 -> 1, 1.1 -> 0.9
float posDec(float x)
{
    if(x < 0.f)
        return 1.f + x - (int)x;
    return x - (int)x;
}

/** Callback for processing and synthesizing audio
 *
 *  The audio buffers are arranged as arrays of samples for each channel.
 *  For example, to access the left input you would use:
 *    in[0][n]
 *  where n is the specific sample.
 *  There are "size" samples in each array.
 *
 *  The default size is very small (just 4 saqmples per channel). This means the
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

    // Get all CVs, gate ins, and Inputs
    patch.ProcessAllControls();

    // Initialize array of 2 floats called gains, set with ADC CV1-4 inputs
    float gains[2];
    for (size_t i = 0; i < 2; i++)
    {
        gains[i] = clamp(patch.GetAdcValue(i) + patch.GetAdcValue(i+4));
    }

    // Initialize array of 2 floats called pans, set with ADC CV5-8 inputs
    float pans[2];
    for (size_t i = 0; i < 2; i++)
    {
        if (lever.RawState()) {
          pans[i] = posDec(patch.GetAdcValue(i+2) + patch.GetAdcValue(i+6));
        } else {
          pans[i] = 0.5f;
        }
    }

    // calculate the output of the mixer
    float outL = 0.f;
    float outR = 0.f;

    for(size_t i = 0; i < size; i++)
    {
      for(int chn = 0; chn < 2; chn++)
        {
            outL += in[chn][i] * gains[chn] * (1- pans[chn]) ; 
            outR += in[chn][i] * gains[chn] * pans[chn];
        }

        //attenuate by 1/4
        OUT_L[i] = outL * .25f; 
        OUT_R[i] = outR * .25f; 
    }




}

int main(void)
{
    /** Initialize the hardware */
    patch.Init();
    lever.Init(patch.B8);

    /** Start Processing the audio */
    patch.StartAudio(AudioCallback);
    while(1) {}
}

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

    // Initialize array of 4 floats called gains, set with ADC CV1-4 inputs
    float gains[4];
    for (size_t i = 0; i < 4; i++)
    {
        gains[i] = patch.GetAdcValue(i);
    }

    // Initialize array of 4 floats called pans, set with ADC CV5-8 inputs
    float pans[4];
    for (size_t i = 0; i < 4; i++)
    {
        pans[i] = patch.GetAdcValue(i + 4);
    }

    // Initialize array of 4 floats called inputs, set wit gate_in_1-2, INL[i], INR[i]
    float inputs[4];
    inputs[0] = patch.gate_in_1.State();
    inputs[1] = patch.gate_in_2.State();
    inputs[2] = IN_L[0];
    inputs[3] = IN_R[0];

    // calculate the output of the mixer
    float outL = 0.f;
    float outR = 0.f;
    for (size_t i = 0; i < 4; i++)
    {
        outL += inputs[i] * gains[i] * (1.f - pans[i]);
        outR += inputs[i] * gains[i] * pans[i];
    }

    // set the output of the mixer
    OUT_L[0] = outL;
    OUT_R[0] = outR;
}

int main(void)
{
    /** Initialize the hardware */
    patch.Init();

    /** Start Processing the audio */
    patch.StartAudio(AudioCallback);
    while(1) {}
}

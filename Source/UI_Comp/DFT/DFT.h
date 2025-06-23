#pragma once

#include <vector>
#include <string>
#include <unordered_map>

#include "../../../pfft/fftpack.h"
#include "../../../pfft/pffft.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

using namespace juce;

#define INPUT_RING_BUFFER_SIZE 8192 * 4

// pfft wrapper to be used in this project.
// takes audio samples, returns amplitude as a callback when available..
// on changing the fft order this class listens to the changes and automatically 
// apply them from the next process block
class PFFFT
{
public:

    // callback happens when a new frame of data is available.
    // send back the calculated amplitudes, with the number of bins.
    PFFFT(AudioProcessorValueTreeState& apvts_reference);
    ~PFFFT();

    void cleanAllContainers();

    void processBlock(float* input, int numSamples);

    static void calculateAmplitudesFromFFT(float* input, float* output, int numSamples);
   
    void setCallback(std::function<void(float* amplitude_data_pointer, int valid_bins)> tick_callback);

private:

    std::function<void(float* amplitude_data_pointer, int valid_bins)> frame_callback = NULL;

    std::vector<float> ring_buffer;
    int ReadIndex = 0, WriteIndex = 0;

    AudioProcessorValueTreeState& apvts_ref;

    // A tick is one cycle of fft -> amplitude/phase calc.
    // tick rate depends on sample rate, fft order and the overlap amount.
    int tick = 0;
    static std::function<int(int)> powToTwo;

    float overlap_amnt = 0.5;

    // 5 possible orders. here N => order of the fft.
    const int NUM_SUPPORTED_N = 5;

    const std::unordered_map<int, int> SUPPPORTED_N_INDEX
    {
        { 9 ,  0 },
        { 10 , 1 },
        { 11 , 2 },
        { 12 , 3 },
        { 13 , 4 }
    };

    const std::vector<int> SUPPORTED_N_VALUES
    {
        9,  // 512
        10, // 1024
        11, // 2048
        12, // 4096
        13  // 8192
    };

    const std::vector<int> SUPPORTED_FFT_SIZES
    {
        powToTwo(SUPPORTED_N_VALUES[0]),
        powToTwo(SUPPORTED_N_VALUES[1]),
        powToTwo(SUPPORTED_N_VALUES[2]),
        powToTwo(SUPPORTED_N_VALUES[3]),
        powToTwo(SUPPORTED_N_VALUES[4])
    };

    // Setups can be safely used from multiple threads 
    // they are read only.
    const std::vector<PFFFT_Setup*> pffft_setups
    {
        pffft_new_setup(powToTwo(SUPPORTED_N_VALUES[0]), PFFFT_REAL),
        pffft_new_setup(powToTwo(SUPPORTED_N_VALUES[1]), PFFFT_REAL),
        pffft_new_setup(powToTwo(SUPPORTED_N_VALUES[2]), PFFFT_REAL),
        pffft_new_setup(powToTwo(SUPPORTED_N_VALUES[3]), PFFFT_REAL),
        pffft_new_setup(powToTwo(SUPPORTED_N_VALUES[4]), PFFFT_REAL)
    };

    std::vector<std::vector<float>> windows;

    // +2 as N/2 + 1 bin. and each bin is of 2 values.
    int SUPER_SET_SIZE = powToTwo(SUPPORTED_N_VALUES[4]) + 2;

    float* pffft_input  = (float*)pffft_aligned_malloc(sizeof(float) * SUPER_SET_SIZE);
    float* pffft_work   = (float*)pffft_aligned_malloc(sizeof(float) * SUPER_SET_SIZE);
    float* pffft_output = (float*)pffft_aligned_malloc(sizeof(float) * SUPER_SET_SIZE);

    // Once there are enough samples.
    std::vector<float> amplitude_buffer;
    // some assumptions to store the history.
    // These are used to approximate the size of history buffers needed to be 
    // when they are initialised.
    const float MAX_BPM = 240;
    const float MIN_BPM = 60;
    // one bar is the biggest selection.
    // can select upto 4 bars here and with 64 multiple you would be watching 
    // 256 bars of history.
    const float MAX_FRACTION = 2.0;
    const float MAX_MULTIPLE = 64.0;
    const float MAX_SAMPLE_RATE = 96000.0;

    // minimum number of frames at each given FFT size based on the above values.
    const std::vector<int> NUM_FRAMES{
        static_cast<int>((MAX_SAMPLE_RATE / static_cast<float>(SUPPORTED_FFT_SIZES[0]))
                * (1.0f / overlap_amnt) * (MAX_FRACTION * MAX_MULTIPLE * (1.0f / MIN_BPM))),
        static_cast<int>((MAX_SAMPLE_RATE / static_cast<float>(SUPPORTED_FFT_SIZES[1]))
                * (1.0f / overlap_amnt) * (MAX_FRACTION * MAX_MULTIPLE * (1.0f / MIN_BPM))),
        static_cast<int>((MAX_SAMPLE_RATE / static_cast<float>(SUPPORTED_FFT_SIZES[2]))
                * (1.0f / overlap_amnt) * (MAX_FRACTION * MAX_MULTIPLE * (1.0f / MIN_BPM))),
        static_cast<int>((MAX_SAMPLE_RATE / static_cast<float>(SUPPORTED_FFT_SIZES[3]))
                * (1.0f / overlap_amnt) * (MAX_FRACTION * MAX_MULTIPLE * (1.0f / MIN_BPM))),
        static_cast<int>((MAX_SAMPLE_RATE / static_cast<float>(SUPPORTED_FFT_SIZES[4]))
                * (1.0f / overlap_amnt) * (MAX_FRACTION * MAX_MULTIPLE * (1.0f / MIN_BPM)))
    };

};
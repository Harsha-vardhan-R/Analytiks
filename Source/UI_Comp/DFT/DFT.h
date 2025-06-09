#pragma once

#include <vector>
#include <string>
#include <unordered_map>

#include "../../../pfft/fftpack.h"
#include "../../../pfft/pffft.h"

#include <juce_dsp/juce_dsp.h>

// pfft wrapper to be used in this project.
// takes audio samples, returns amplitude and phase.
class PFFFT
{
public:

    // callback happens when a new frame of data is available.
    PFFFT(
        std::function<void(int)> tick_callback
    );
    ~PFFFT();

    void resetOrder(int newOrder);
    void cleanAllContainers();

    void processBlock(float* input, float* output, int FFT_order);

    // takes in the outputs from fft, calculates the amplitude.
    // on outputs given by pfft(uses a compressed representation).
    // inputs and outputs can be same for inplace stuff.
    static void calculateAmplitudesFromFFT(float* input, float* output, int numSamples);
    static void calculatePhasesFromFFT(float* input, float* output, int numSamples);

    

private:

    // A tick is one cycle of fft -> amplitude/phase calc.
    // tick rate depends on sample rate, fft order and the overlap amount.
    int tick = 0;
    std::function<int(int)> powToTwo = [](int power) -> int
        {
            return 2 << (power - 1);
        };

    float overlap_amnt = 0.5;

    // 5 possible orders. here N => order of the fft.
    const int NUM_SUPPORTED_N = 5;

    const int CURRENT_N = 11;

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

    std::vector<float*> pffft_inputs
    {
        (float*)pffft_aligned_malloc(sizeof(float) * powToTwo(SUPPORTED_N_VALUES[0])),
        (float*)pffft_aligned_malloc(sizeof(float) * powToTwo(SUPPORTED_N_VALUES[1])),
        (float*)pffft_aligned_malloc(sizeof(float) * powToTwo(SUPPORTED_N_VALUES[2])),
        (float*)pffft_aligned_malloc(sizeof(float) * powToTwo(SUPPORTED_N_VALUES[3])),
        (float*)pffft_aligned_malloc(sizeof(float) * powToTwo(SUPPORTED_N_VALUES[4])),
    };


    std::vector<float*> pffft_outputs
    {
        (float*)pffft_aligned_malloc(sizeof(float) * powToTwo(SUPPORTED_N_VALUES[0])),
        (float*)pffft_aligned_malloc(sizeof(float) * powToTwo(SUPPORTED_N_VALUES[1])),
        (float*)pffft_aligned_malloc(sizeof(float) * powToTwo(SUPPORTED_N_VALUES[2])),
        (float*)pffft_aligned_malloc(sizeof(float) * powToTwo(SUPPORTED_N_VALUES[3])),
        (float*)pffft_aligned_malloc(sizeof(float) * powToTwo(SUPPORTED_N_VALUES[4])),
    };

    // temp buffers used internally by pffft.
    std::vector<float*> pffft_work
    {
        (float*)pffft_aligned_malloc(sizeof(float) * powToTwo(SUPPORTED_N_VALUES[0])),
        (float*)pffft_aligned_malloc(sizeof(float) * powToTwo(SUPPORTED_N_VALUES[1])),
        (float*)pffft_aligned_malloc(sizeof(float) * powToTwo(SUPPORTED_N_VALUES[2])),
        (float*)pffft_aligned_malloc(sizeof(float) * powToTwo(SUPPORTED_N_VALUES[3])),
        (float*)pffft_aligned_malloc(sizeof(float) * powToTwo(SUPPORTED_N_VALUES[4])),
    };

    // some assumptions.
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

    // This is where the amplitudes, phases are updated.
    // Double buffered.
    // once this is updated, put the latest updated as the fromt buffer
    float* amplitude_dubl_buf_1{ (float*)pffft_aligned_malloc(sizeof(float) * powToTwo(SUPPORTED_N_VALUES[4])) };
    float* amplitude_dubl_buf_2{ (float*)pffft_aligned_malloc(sizeof(float) * powToTwo(SUPPORTED_N_VALUES[4])) };

    float* phase_dubl_buf_1{ (float*)pffft_aligned_malloc(sizeof(float) * powToTwo(SUPPORTED_N_VALUES[4])) };
    float* phase_dubl_buf_2{ (float*)pffft_aligned_malloc(sizeof(float) * powToTwo(SUPPORTED_N_VALUES[4])) };

    std::atomic<float*> amplitude_double_buffer_order[2] = { amplitude_dubl_buf_1, amplitude_dubl_buf_2 };
    std::atomic<float*> phase_double_buffer_order[2] = { phase_dubl_buf_1, phase_dubl_buf_2 };


    // This is where the fft'd values are stored, a vector of circular buffers.
    std::vector<std::vector<std::vector<float>>> fft_history_buffers;

};
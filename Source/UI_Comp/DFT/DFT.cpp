#include "DFT.h"

PFFFT::PFFFT(
    std::function<void(int)> tick_callback
)
{
    jassert(overlap_amnt >= 0.0 && overlap_amnt <= 1.0);

    fft_history_buffers.resize(NUM_SUPPORTED_N);
    for (int i = 0; i < NUM_SUPPORTED_N; ++i)
    {
        fft_history_buffers[i].resize(NUM_FRAMES[i]);
        for (int j = 0; j < NUM_FRAMES[i]; ++j)
        {
            fft_history_buffers[i][j].resize(SUPPORTED_FFT_SIZES[i], 0.0f);
        }
    }
}

PFFFT::~PFFFT()
{
    for (float* allocation : pffft_inputs)
        pffft_aligned_free(allocation);
    
    for (float* allocation : pffft_outputs)
        pffft_aligned_free(allocation);

    for (float* allocation : pffft_work)
        pffft_aligned_free(allocation);

    for (PFFFT_Setup* setup : pffft_setups)
        pffft_destroy_setup(setup);
}

void PFFFT::resetOrder(int newOrder)
{
}

void PFFFT::cleanAllContainers()
{
}

void PFFFT::processBlock(float* input, float* output, int FFT_order)
{
}

void PFFFT::calculateAmplitudesFromFFT(float* input, float* output, int numSamples)
{
    // inputs will be in the form of 
    // N = numSamples
    // [re(0), re(NqF), re(1), im(1), re(2), im(2), ... re((N/2)-1), im((N/2)-1)]
    // where NqF = N/2.
    float nyquist_real = input[1];

    int num_bins = (numSamples / 2) + 1;
    
    // bin 0 is 0 Hz DC signal, 2*k, 2*k+1 are the indexes of real and imaginary
    // parts of bin k where k = [1 ,N/2 - 1]
    for (int k = 1; k < num_bins - 1; ++k)
    {
        int k_2 = 2 * k;
        float real = input[k_2];
        // john lennon.
        float imagine = input[k_2 + 1];

        output[k] = std::sqrtf(real * real + imagine * imagine);
    }

    // DC and Nyquist amplitudes.
    output[0] = input[0];
    output[num_bins - 1] = std::abs(nyquist_real);
}

void PFFFT::calculatePhasesFromFFT(float* input, float* output, int numSamples)
{
    // N = numSamples
    // inputs: [re(0), re(NqF), re(1), im(1), re(2), im(2), ... re((N/2)-1), im((N/2)-1)]
    // NqF = N/2
    int num_bins = (numSamples / 2) + 1;

    // DC and Nyquist bins are purely real, so phase is 0
    output[0] = 0.0f;
    output[num_bins - 1] = 0.0f;

    for (int k = 1; k < num_bins - 1; ++k)
    {
        int k_2 = 2 * k;
        float real = input[k_2];
        float imag = input[k_2 + 1];

        output[k] = std::atan2(imag, real);
    }
}
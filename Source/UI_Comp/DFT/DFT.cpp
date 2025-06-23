#include "DFT.h"

std::function<int(int)> PFFFT::powToTwo = 
    [](int power) -> int { return 2 << (power - 1); };

PFFFT::PFFFT(
    AudioProcessorValueTreeState& apvts_reference)
    :   apvts_ref(apvts_reference)
{

    DBG("FFT Engine SIMD size : " + String(pffft_simd_size()));

    amplitude_buffer.resize(SUPER_SET_SIZE);
    ring_buffer.resize(INPUT_RING_BUFFER_SIZE);

    for (int n = 0; n < NUM_SUPPORTED_N; ++n)
    {
        windows.push_back(std::vector<float>());
        windows[n].resize(SUPPORTED_FFT_SIZES[n]);

        dsp::WindowingFunction<float>::fillWindowingTables(
            windows[n].data(),
            SUPPORTED_FFT_SIZES[n],
            dsp::WindowingFunction<float>::hann,
            true
        );
    }
}

PFFFT::~PFFFT()
{

    for (PFFFT_Setup* setup : pffft_setups)
        pffft_destroy_setup(setup);
    
    pffft_aligned_free(pffft_input);
    pffft_aligned_free(pffft_work);
    pffft_aligned_free(pffft_output);
    
}

void PFFFT::cleanAllContainers()
{
    for (auto& value : ring_buffer)
        value = 0.0;

    for (auto& value : amplitude_buffer)
        value = 0.0;

}

void PFFFT::processBlock(float* input, int numSamples)
{
    int FFT_order = apvts_ref.getRawParameterValue("gb_fft_ord")->load();

    auto it = SUPPPORTED_N_INDEX.find(FFT_order);
    if (it == SUPPPORTED_N_INDEX.end())
        return; // Invalid order

    int fft_index = it->second;
    int fft_size = SUPPORTED_FFT_SIZES[fft_index];
    PFFFT_Setup* setup = pffft_setups[fft_index];
    auto& windowing_array = windows[fft_index];

    int hop_size = static_cast<int>(fft_size * (1.0f - overlap_amnt));

    // write new data to ring buff.
    for (int i = 0; i < numSamples; ++i)
    {
        ring_buffer[WriteIndex] = input[i];
        WriteIndex = (WriteIndex + 1) % ring_buffer.size();
    }

    // Process as many frames as possible
    while (true)
    {
        
        int available = (WriteIndex >= ReadIndex)
            ? (WriteIndex - ReadIndex)
            : (ring_buffer.size() - ReadIndex + WriteIndex);

        if (available < fft_size)
            break;

        // Copy fft_size samples with the window multiplied.
        for (int i = 0; i < fft_size; ++i)
        {
            int idx = (ReadIndex + i) % ring_buffer.size();
            pffft_input[i] = ring_buffer[idx] * windowing_array[i];
        }

        pffft_transform_ordered(setup, pffft_input, pffft_output, NULL, PFFFT_FORWARD);
        calculateAmplitudesFromFFT(pffft_output, amplitude_buffer.data(), fft_size);

        int num_bins = (fft_size / 2) + 1;

        ReadIndex = (ReadIndex + hop_size) % ring_buffer.size();

        if (frame_callback)
            frame_callback(amplitude_buffer.data(), num_bins);
    }
}

void PFFFT::calculateAmplitudesFromFFT(float* input, float* output, int numSamples)
{
    int num_bins = (numSamples / 2) + 1;
    output[0] = std::abs(input[0]);           // DC
    output[num_bins - 1] = std::abs(input[1]); // Nyquist
    for (int k = 1; k < num_bins - 1; ++k) {
        float real = input[2 * k];
        float imag = input[2 * k + 1];
        output[k] = std::sqrt(real * real + imag * imag);
    }
}

void PFFFT::setCallback(std::function<void(float* amplitude_data_pointer, int valid_bins)> tick_callback)
{
    frame_callback = tick_callback;
}

#include "DFT.h"

std::function<int(int)> PFFFT::powToTwo = 
    [](int power) -> int { return 1 << power; };

std::function<void(string)> callback = 
    [](string msg) -> void { DBG(msg); };

PFFFT::PFFFT(
    AudioProcessorValueTreeState& apvts_reference)
    :   spectral_analyser_component(std::make_unique<SpectrumAnalyserComponent>(apvts_reference, callback)),
        spectrogram_component(std::make_unique<SpectrogramComponent>(apvts_reference)),
        apvts_ref(apvts_reference)
{

    cout << "FFT Engine SIMD size : " + String(pffft_simd_size()) << "\n";

    amplitude_buffer.resize(SUPER_SET_SIZE);
    ring_buffer.resize(INPUT_RING_BUFFER_SIZE);

    for (int i = 0; i < MAX_ACCUMULATED; ++i) {
        processed_amplitude_data[i].resize(MAX_BUFFER_SIZE);  // or max bin count
    }

    for (int n = 0; n < NUM_SUPPORTED_N; ++n)
    {
        if (!pffft_setups[n]) {
            std::cerr << "FFT setup creation failed at index " << n << "\n";
            std::exit(EXIT_FAILURE); // kaboooom
        }

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

std::array<Component*, 2> PFFFT::getSpectrogramAndAnalyser()
{
    std::array<Component*, 2> arr = {
        spectrogram_component.get(),
        spectral_analyser_component.get()
    };

    return arr;
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
    // if we can't see both analyser and the spectrogram - 
    // there is not a need to calculate this.
    if (spectral_analyser_component->getHeight() == 0) return;
    // ===================================================

    int FFT_order = apvts_ref.getRawParameterValue("gb_fft_ord")->load();

    auto it = SUPPPORTED_N_INDEX.find(FFT_order + 9);
    if (it == SUPPPORTED_N_INDEX.end()) {
        std::cout << "Unsupported FFT order requested : " << FFT_order << "\n";
        return; // Invalid order
    }

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

    int indx = 0;

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

        auto& vec_reference = processed_amplitude_data[indx];
        for (int bin = 0; bin < num_bins; ++bin) {
            vec_reference[bin] = amplitude_buffer[bin];
        }
        
        indx++;

        if (indx >= MAX_ACCUMULATED) {
            std::cerr << "Processed amplitude data buffer overflow\n";
            break;
        }
    }

    spectrogram_component->newDataBatch(processed_amplitude_data, indx, (fft_size / 2) + 1);
    for (int i = 0; i < indx; ++i) {
        spectral_analyser_component->newData(processed_amplitude_data[i].data(), (fft_size / 2) + 1);
    }
}

void PFFFT::calculateAmplitudesFromFFT(float* input, float* output, int numSamples)
{
    int num_bins = (numSamples / 2) + 1;
    output[0] = std::abs(input[0]);           // DC
    output[num_bins - 1] = std::abs(input[1]); // Nyquist
    for (int bin = 1; bin < num_bins - 1; ++bin) { // DC and nyquist are the bins that are excluded.
        float real = input[2 * bin];
        float imag = input[2 * bin + 1];
        output[bin] = std::sqrt(real * real + imag * imag);
    }

    for (int bin = 0; bin < num_bins; ++bin) {
        output[bin] /= (float)numSamples;

        // output is sent as a dB map that is normalised to 0.0 to 1.0,
        // because both the analyser and the spectrogram expect that.
        output[bin] = 
            jmap<float> ( 
                20.0f * log10f(std::clamp<float>(output[bin], 0.0, 1.0) + 1e-4f),
                -80,
                0,
                0,
                1);

    }
}


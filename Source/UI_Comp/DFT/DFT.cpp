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
        processed_amplitude_data[i].resize(MAX_BUFFER_SIZE);
    }

    // pre-size each result slot in the worker_buffers — nothing else needed,
    // WorkerFFTBuffers allocates its own aligned memory on construction.
    for (int n = 0; n < NUM_SUPPORTED_N; ++n)
    {
        if (!pffft_setups[n]) {
            std::cerr << "FFT setup creation failed at index " << n << "\n";
            std::exit(EXIT_FAILURE);
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

    startTimerHz(FPS);
}

PFFFT::~PFFFT()
{
    // fft_worker_pool destructor joins all workers before we free anything below.
    // Declaration order in the header guarantees fft_worker_pool is destroyed
    // before pffft_setups, so no worker can be mid-transform when we free setups.

    for (PFFFT_Setup* setup : pffft_setups)
        pffft_destroy_setup(setup);
    
    pffft_aligned_free(pffft_input);
    pffft_aligned_free(pffft_work);
    pffft_aligned_free(pffft_output);
}

void PFFFT::play()
{
    spectrogram_component->parameterChanged("", 0.0f);
}

void PFFFT::timerCallback()
{
    // Drain the result queue on the UI/timer thread.
    // Workers push FFTResult objects here; we consume them all each tick.
    // The mutex is held only briefly per result — this never blocks the audio thread
    // because the audio thread only holds the mutex during a quick push.
    while (true)
    {
        FFTResult result;
        {
            std::lock_guard<std::mutex> lock(result_mutex);
            if (result_queue.empty()) break;
            result = std::move(result_queue.front());
            result_queue.pop_front();
        }

        // These calls are safe here — we're on the UI thread i.e. message thread.
        spectrogram_component->newDataBatch(
            result.amplitude_data,
            result.valid_frames,
            result.num_bins,
            result.bpm,
            result.sample_rate,
            result.N,
            result.D
        );

        for (int i = 0; i < result.valid_frames; ++i)
            spectral_analyser_component->newData(result.amplitude_data[i].data(), result.num_bins);
    }

    spectral_analyser_component->timerCallback();
    spectrogram_component->timerCallback();
}

std::array<Component*, 2> PFFFT::getSpectrogramAndAnalyser()
{
    std::array<Component*, 2> arr = {
        spectrogram_component.get(),
        spectral_analyser_component.get()
    };
    return arr;
}

void PFFFT::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    spectral_analyser_component->prepareToPlay(sampleRate, samplesPerBlock);
}

void PFFFT::cleanAllContainers()
{
    for (auto& value : ring_buffer)   value = 0.0;
    for (auto& value : amplitude_buffer) value = 0.0;
}

void PFFFT::processBlock(const float* input, int numSamples, float bpm, float SR, int N, int D)
{
    int FFT_order = apvts_ref.getRawParameterValue("gb_fft_ord")->load();

    auto it = SUPPPORTED_N_INDEX.find(FFT_order + 9);
    if (it == SUPPPORTED_N_INDEX.end()) {
        std::cout << "Unsupported FFT order requested : " << FFT_order << "\n";
        return;
    }

    int          fft_index       = it->second;
    int          fft_size        = SUPPORTED_FFT_SIZES[fft_index];
    PFFFT_Setup* setup           = pffft_setups[fft_index];
    auto&        windowing_array = windows[fft_index];
    int          hop_size        = overlap_samples;
    int          num_bins        = (fft_size / 2) + 1;

    // Write new samples into the ring buffer — audio thread only, no lock needed.
    for (int i = 0; i < numSamples; ++i)
    {
        ring_buffer[WriteIndex] = input[i];
        WriteIndex = (WriteIndex + 1) % ring_buffer.size();
    }

    // Collect all available frames from the ring buffer into a local snapshot.
    // We do the ring buffer reads here on the audio thread (cheap), then hand
    // the raw samples to the worker thread for the actual FFT + amplitude math.
    struct FrameSnapshot {
        std::vector<float> samples; // windowed samples, fft_size long
    };

    std::vector<FrameSnapshot> frames;
    frames.reserve(MAX_ACCUMULATED);

    while ((int)frames.size() < MAX_ACCUMULATED)
    {
        int available = (WriteIndex >= ReadIndex)
            ? (WriteIndex - ReadIndex)
            : ((int)ring_buffer.size() - ReadIndex + WriteIndex);

        if (available < fft_size) break;

        FrameSnapshot snap;
        snap.samples.resize(fft_size);

        for (int i = 0; i < fft_size; ++i)
        {
            int idx = (ReadIndex + i) % ring_buffer.size();
            snap.samples[i] = ring_buffer[idx] * windowing_array[i];
        }

        frames.push_back(std::move(snap));
        ReadIndex = (ReadIndex + hop_size) % ring_buffer.size();
    }

    if (frames.empty()) return;

    // Submit all the FFT work to the pool.
    // The lambda captures by value everything the worker needs:
    //   - frames        : the windowed sample data (moved in)
    //   - setup         : read-only PFFFT_Setup*, safe across threads
    //   - fft_size/bins : plain ints
    //   - bpm/SR/N/D    : plain values for result metadata
    //   - this          : to access worker_buffers and push to result_queue
    //
    // worker_id is received via the task_init_callback to index worker_buffers.
    // We use a shared_ptr<int> to pass the worker_id into the lambda because
    // we can't capture it directly before submission.

    // Round-robin worker buffer selection — audio thread is the only submitter,
    // so this counter has no race.
    static int buffer_index = 0;
    int buf_idx = buffer_index % 2;
    buffer_index++;

    // Capture everything by value. frames is moved in to avoid a copy.
    fft_worker_pool.submit_work([
        this,
        frames      = std::move(frames),
        setup,
        fft_size,
        num_bins,
        buf_idx,
        bpm, SR, N, D
    ]() mutable
    {
        WorkerFFTBuffers& bufs = worker_buffers[buf_idx];

        FFTResult result;
        result.num_bins     = num_bins;
        result.bpm          = bpm;
        result.sample_rate  = SR;
        result.N            = N;
        result.D            = D;

        for (int i = 0; i < MAX_ACCUMULATED; ++i)
            result.amplitude_data[i].resize(num_bins);

        int indx = 0;
        for (auto& frame : frames)
        {
            // copy windowed samples into this worker's own input buffer
            std::memcpy(bufs.input, frame.samples.data(), fft_size * sizeof(float));

            pffft_transform_ordered(setup, bufs.input, bufs.output, bufs.work, PFFFT_FORWARD);

            calculateAmplitudesFromFFT(bufs.output, result.amplitude_data[indx].data(), fft_size);

            indx++;
        }

        result.valid_frames = indx;

        // Push to result queue — timerCallback drains this on the UI thread.
        {
            std::lock_guard<std::mutex> lock(result_mutex);
            result_queue.push_back(std::move(result));
        }
    });
}

void PFFFT::calculateAmplitudesFromFFT(float* input, float* output, int numSamples)
{
    int num_bins = (numSamples / 2) + 1;
    output[0] = std::abs(input[0]);            // DC
    output[num_bins - 1] = std::abs(input[1]); // Nyquist
    for (int bin = 1; bin < num_bins - 1; ++bin) {
        float real = input[2 * bin];
        float imag = input[2 * bin + 1];
        output[bin] = std::sqrt(real * real + imag * imag);
    }

    for (int bin = 0; bin < num_bins; ++bin) {
        output[bin] /= (float)numSamples;
        output[bin] = 
            jmap<float>(
                20.0f * log10f(std::clamp<float>(output[bin], 0.0, 1.0) + 1e-4f),
                -80, 0, 0, 1
            );
    }
}
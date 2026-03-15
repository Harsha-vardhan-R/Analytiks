#pragma once

// acts as the parent of the spectrogram and the spectrum analyser.

#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <deque>

#include "../../../pfft/fftpack.h"
#include "../../../pfft/pffft.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "../Analyser/Analyser.h"
#include "../Spectrogram/Spectrogram.h"

#include "../../ds/dataStructure.h"

#include "../../../rwqueue/readerwritercircularbuffer.h"

#include "../util.h"
#include "workerpool.h"

using namespace juce;

#define FPS 60
#define MAX_BUFFER_SIZE 8192
#define INPUT_RING_BUFFER_SIZE 8192 * 4
#define MAX_ACCUMULATED 32

// One per worker thread — each worker gets its own aligned FFT buffers
// so pffft_transform_ordered is never called with shared memory across threads.
struct WorkerFFTBuffers {
    float* input  = (float*)pffft_aligned_malloc(sizeof(float) * 8192);
    float* work   = (float*)pffft_aligned_malloc(sizeof(float) * 8192);
    float* output = (float*)pffft_aligned_malloc(sizeof(float) * 8192);

    WorkerFFTBuffers() = default;

    // not copyable — these are raw heap allocations
    WorkerFFTBuffers(const WorkerFFTBuffers&)            = delete;
    WorkerFFTBuffers& operator=(const WorkerFFTBuffers&) = delete;

    ~WorkerFFTBuffers() {
        pffft_aligned_free(input);
        pffft_aligned_free(work);
        pffft_aligned_free(output);
    }
};

// Result of one processed FFT batch, passed from worker thread to UI thread.
struct FFTResult {
    std::array<std::vector<float>, MAX_ACCUMULATED> amplitude_data;
    int    valid_frames = 0;
    int    num_bins     = 0;
    float  bpm          = 0.0f;
    float  sample_rate  = 0.0f;
    int    N            = 0;
    int    D            = 0;
};

// pfft wrapper to be used in this project.
// takes audio samples, returns amplitude as a callback when available..
// on changing the fft order this class listens to the changes and automatically 
// apply them from the next process block.
class PFFFT :
      private Timer {
public:

    // callback happens when a new frame of data is available.
    // send back the calculated amplitudes, with the number of bins.
    PFFFT(AudioProcessorValueTreeState& apvts_reference);
    ~PFFFT();

    void play();

    void timerCallback() override;

    std::array<Component*, 2> getSpectrogramAndAnalyser();
    void prepareToPlay(double sampleRate, int samplesPerBlock);

    void cleanAllContainers();

    // automatically triggers the spectogram's and the analysers
    // repaint methods.
    void processBlock(const float* input, int numSamples, float bpm, float SR, int N, int D);

    static void calculateAmplitudesFromFFT(float* input, float* output, int numSamples);

    int getHeight() { return spectrogram_component->getHeight(); }
   
private:

    // ── worker pool ──────────────────────────────────────────────────────────
    // 2 workers: one for spectrogram, one for analyser frames.
    // More than 2 rarely helps since timerCallback is the bottleneck.
    VoidVoidWorkerPool fft_worker_pool { 2 };

    // Per-worker FFT buffers — indexed by worker_id (0 or 1).
    // Constructed once, never resized, so the pointer is stable.
    std::array<WorkerFFTBuffers, 2> worker_buffers;

    // Result queue — worker threads push, timerCallback drains on UI thread.
    // Protected by result_mutex.
    std::deque<FFTResult> result_queue;
    std::mutex            result_mutex;

    // ── original members ─────────────────────────────────────────────────────
    std::array<std::vector<float>, MAX_ACCUMULATED> processed_amplitude_data;

    std::unique_ptr<SpectrogramComponent>     spectrogram_component;
    std::unique_ptr<SpectrumAnalyserComponent> spectral_analyser_component;

    std::vector<float> ring_buffer;
    int ReadIndex = 0, WriteIndex = 0;

    AudioProcessorValueTreeState& apvts_ref;

    int tick = 0;
    static std::function<int(int)> powToTwo;

    float overlap_samples = HOP_SIZE;

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

    // Setups can be safely used from multiple threads — they are read only.
    const std::vector<PFFFT_Setup*> pffft_setups
    {
        pffft_new_setup(powToTwo(SUPPORTED_N_VALUES[0]), PFFFT_REAL),
        pffft_new_setup(powToTwo(SUPPORTED_N_VALUES[1]), PFFFT_REAL),
        pffft_new_setup(powToTwo(SUPPORTED_N_VALUES[2]), PFFFT_REAL),
        pffft_new_setup(powToTwo(SUPPORTED_N_VALUES[3]), PFFFT_REAL),
        pffft_new_setup(powToTwo(SUPPORTED_N_VALUES[4]), PFFFT_REAL)
    };

    std::vector<std::vector<float>> windows;

    int SUPER_SET_SIZE = powToTwo(SUPPORTED_N_VALUES[4]);

    // These are now only used for the ring buffer read on the audio thread.
    // Per-worker transform buffers live in worker_buffers above.
    float* pffft_input  = (float*)pffft_aligned_malloc(sizeof(float) * SUPER_SET_SIZE);
    float* pffft_work   = (float*)pffft_aligned_malloc(sizeof(float) * SUPER_SET_SIZE);
    float* pffft_output = (float*)pffft_aligned_malloc(sizeof(float) * SUPER_SET_SIZE);

    std::vector<float> amplitude_buffer;
};
#pragma once

#include "../../rwqueue/readerwriterqueue.h"

#include <vector>
#include <queue>
#include <atomic>

#include <juce_core/juce_core.h>

using namespace std;

#define POOL_SIZE 1024
#define DATA_SIZE 8192

// This data structure runs on the balance of the consumer and the 
// producer, we consume in bites and we should eat the data faster than
// it accumulates more than `POOL_SIZE`.
// consumer accessing the data after the producer stops producing is a valid case,
// but the other way around is not valid, you cannot accumulate more than
// `POOL_DATA` elements.

// manages the memory and exchange of data required 
// b/w DFT engine and the spectrometer, analyser component.
class linkDS {
public:
    linkDS();

    /// ====================== THREAD 1 ===========================
    // called from the fft engine (PFFT class);
    void addNewData(float* data, int numSamples);
    /// ===========================================================

    /// ====================== THREAD 2 ===========================
    // even though these functions are called from different classes,
    // they all will be called from the same thread.
    // 
    // called from Analyser component.
    bool isLatestDataPresent();
    // buffer and prev can be the same.
    bool fillLatestData(
        float* buffer, 
        float* prev, 
        float alpha, 
        int numSamples);

    // called from Spectrogram component.
    // returns false if no new data is available.
    bool fillFrontData(float* buffer, int numSamples);
    bool empty() const; // returns number of non consumed fft data.
    //// ===========================================================

private:
    // buffer pool.
    vector<vector<float>> pool;

    // stores the indexes of the buffer containers from the pool.
    atomic<int> latest_data;
    atomic<int> last_read_latest_data;

    // cannot be bigger than the number of elements in the queue.
    moodycamel::ReaderWriterQueue<int> newDataQueue{ POOL_SIZE };
    moodycamel::ReaderWriterQueue<int> useQueue{ POOL_SIZE };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(linkDS);
};

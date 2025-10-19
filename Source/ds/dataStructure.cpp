#include "dataStructure.h"

linkDS::linkDS() :
    pool(POOL_SIZE, vector<float>(DATA_SIZE, 0.0)),
    latest_data(POOL_SIZE - 1),
    last_read_latest_data(-1)
{
    for (int ele = 0; ele < POOL_SIZE; ++ele) {
        auto succ = useQueue.try_enqueue(ele);
        if (!succ) jassertfalse;
    }
}

bool linkDS::isLatestDataPresent()
{
    return !(latest_data.load() == last_read_latest_data.load());
}

bool linkDS::fillLatestData(
    float* buffer,
    float* prev,
    float alpha,
    int numSamples
)
{
    // if the latest value has been changed
    if (isLatestDataPresent()) {
        float rem_frac = 1.0f - alpha;

        int front_data = latest_data.load();
        const vector<float>& cont = pool[front_data];

        for (int i = 0; i < numSamples; ++i) 
            buffer[i] = cont[i] * alpha + prev[i] * rem_frac;
        
        last_read_latest_data = front_data;
        return true;  // Data was updated
    }
    
    return false;  // No new data was present
}

bool linkDS::fillFrontData(float* buffer, int numSamples)
{
    if (newDataQueue.peek()) 
    {
        int front_data = *newDataQueue.peek();
        vector<float>& cont = pool[front_data];
        for (int i = 0; i < numSamples; ++i) buffer[i] = cont[i];

        auto succ = useQueue.try_enqueue(front_data);
        if (!succ) jassertfalse;

        // this pop always happens even if the 
        // container is used in the latest_data,
        // but for a data race to happen both the threads should be out
        // of sync by more than 'POOL_SIZE'.

        // pop the first element.
        newDataQueue.try_dequeue(front_data);

        return true;
    }
    else 
    {
        return false;
    }
}

void linkDS::addNewData(float* data, int numSamples)
{
    // if the consumer is not eating.
    if (useQueue.peek() == nullptr) {
        if (!newDataQueue.peek()) DBG("[ERROR] inter thread communication ran out of containers");

        useQueue.try_enqueue(*newDataQueue.peek());
        newDataQueue.try_dequeue(*newDataQueue.peek());
    }

    // get the front empty container.
    int empty_container = *useQueue.peek();
    vector<float>& cont = pool[empty_container];

    for (int sample = 0; sample < numSamples; ++sample) cont[sample] = data[sample];

    // if accumulation still happens oldest data is retired.
    if (!newDataQueue.try_enqueue(empty_container)) {
        newDataQueue.try_dequeue(*newDataQueue.peek());
        if (!newDataQueue.try_enqueue(empty_container)) {
            DBG("[WARNING] failed to update new data in the FFT");
        }
    }

    useQueue.try_dequeue(empty_container);

    latest_data.store(empty_container);
}

bool linkDS::empty() const {
    return newDataQueue.peek() == nullptr;
}

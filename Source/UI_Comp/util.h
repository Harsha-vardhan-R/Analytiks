#include <algorithm>

using namespace std;

// returns from 0->1
float freq_lin_to_log(float val) {
    val = clamp<float>(val, 10.0, 20000.0);


}

int helper_freq_to_index(
    int fft_size, 
    float frac_min, 
    float frac_rng) {
    // 10 -> 0, 20,000 -> fft_size
    const int min_range_indexes = 10;
    const float frac_rng_min = 0.2;


}
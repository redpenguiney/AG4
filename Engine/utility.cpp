#include "utility.hpp"
#include <chrono>

double Time() {
    using namespace std::chrono;
    duration<double, std::milli> time = high_resolution_clock::now().time_since_epoch();
    return time.count() / 1000.0;
}
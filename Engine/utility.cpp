#include "utility.hpp"
#include <chrono>

double Time() {
    using namespace std::chrono;
    duration<double, std::milli> time = high_resolution_clock::now().time_since_epoch();
    return time.count() / 1000.0;
}

IdProvider::IdProvider() {
    largestId = 0;
}

void IdProvider::ReturnId(unsigned int id) {
    freeIds.push_back(id);
}

unsigned int IdProvider::GetId() {
    if (freeIds.size()) {
        return freeIds.back();
        freeIds.pop_back();
    }
    else {
        return largestId++;
    }
}

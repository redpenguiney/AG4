#include "utility.hpp"
#include <chrono>
#include <glm/geometric.hpp>

double Time() {
    using namespace std::chrono;
    duration<double, std::milli> time = steady_clock::now().time_since_epoch();
    return time.count() / 1000.0;
}

glm::dvec3 LookVector(double pitch, double yaw) {
    return glm::dvec3(
        sin(yaw) * cos(pitch),
        -sin(pitch),
        -cos(yaw) * cos(pitch)

    );
}

glm::dvec3 ClosestPointOnLine1ToLine2(glm::dvec3 origin1, glm::dvec3 dir1, glm::dvec3 origin2, glm::dvec3 dir2) {
    glm::dvec3 norm = glm::normalize(glm::cross(dir1, dir2));
    double t1 = glm::dot(glm::cross(dir2, norm), origin2 - origin1);
    return origin1 + dir1 * t1;
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

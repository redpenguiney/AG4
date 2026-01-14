#pragma once
#include <vector>
#include "glm/vec3.hpp"

// returns current time in seconds
double Time();

// Generic class for providing reusable, consecutive object ids.
// Will never return an id of 0.
class IdProvider {
public:
	IdProvider();
	void ReturnId(unsigned int id);
	unsigned int GetId();
private:
	std::vector<unsigned int> freeIds;
	unsigned int largestId;
};

glm::dvec3 LookVector(double pitch, double yaw);
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

glm::dvec3 ClosestPointOnLine1ToLine2(glm::dvec3 origin1, glm::dvec3 dir1, glm::dvec3 origin2, glm::dvec3 dir2);
glm::dvec3 PlaneRayIntersection(glm::dvec3 planePoint, glm::dvec3 planeNormal, glm::dvec3 rayPoint, glm::dvec3 rayDir);
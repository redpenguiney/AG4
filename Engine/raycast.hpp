#pragma once
#include <glm/vec3.hpp>

class Gameobject;

struct RaycastResult {
	glm::dvec3 hitPos;
	glm::dvec3 hitNormal;

	double distance;

	// nullptr if no hit
	Gameobject* object;
};

struct RaycastParams {

};

RaycastResult Raycast(glm::dvec3 origin, glm::dvec3 direction, RaycastParams params);
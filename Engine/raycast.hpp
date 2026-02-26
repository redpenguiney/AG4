#pragma once
#include <glm/vec3.hpp>

class Gameobject;

struct RaycastResult {
	glm::dvec3 hitPos;
	glm::dvec3 hitNormal;

	// nullptr if no hit
	Gameobject* object;
};
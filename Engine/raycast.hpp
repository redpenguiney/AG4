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
	// if true, given the opportunity the raycasting will be done against gameobject's rendered meshes rather than against the physics geometry.
	bool preferMesh = false;
};

RaycastResult Raycast(glm::dvec3 origin, glm::dvec3 direction, RaycastParams params);
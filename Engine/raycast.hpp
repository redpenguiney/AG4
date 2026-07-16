#pragma once
#include <glm/vec3.hpp>
#include <bitset>
#include "collision_layers.hpp"

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
	std::bitset<NUM_COLLISION_LAYERS> collisionLayers = 0xffffffff;
};

RaycastResult Raycast(glm::dvec3 origin, glm::dvec3 direction, RaycastParams params);
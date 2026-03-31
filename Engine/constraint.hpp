#pragma once
#include "glm/vec3.hpp"
class Gameobject;
class Physobject;

// physobject-gameobject collision
struct StaticCollisionConstraint {
	glm::vec3 r1;
	glm::vec3 r2;
	Physobject* a;
	Gameobject* b;

	// B to A
	glm::vec3 collisionNormal;
	float totalLagrange;
};

// physobject-physobject collision
struct DynamicCollisionConstraint {
	glm::vec3 r1;
	glm::vec3 r2;
	Physobject* a;
	Physobject* b;

	// B to A
	glm::vec3 collisionNormal;
	float totalLagrange;
};
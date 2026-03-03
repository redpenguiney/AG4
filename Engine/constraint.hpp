#pragma once
#include "glm/vec3.hpp"
class Gameobject;

class CollisionConstraint {
	Gameobject* a;
	Gameobject* b;

	glm::vec3 r1;
	glm::vec3 r2;
};
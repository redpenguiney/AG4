#pragma once
#include <vector>
#include <optional>
#include "glm/vec3.hpp"

struct Collision {
	// in space of objectA
	std::vector<glm::vec3> collisionPointsA;

	// in space of objectB
	std::vector<glm::vec3> collisionPointsB;

	// A to B
	glm::vec3 collisionNormal;
};
class Gameobject;

std::optional<Collision> NarrowphaseCollisionDetection(Gameobject* a, Gameobject* b);
void ClearCollisionsCache();
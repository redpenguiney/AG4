#pragma once
#include "glm/vec3.hpp"
class Gameobject;

struct Constraint {
	const float inverseStiffness = 0; // in m/N; 0 for infinitely stiff constraints
	float lagrangeMultiplier = 0;

	virtual void Update() = 0;
};

struct CollisionConstraint: public Constraint {
	Gameobject* a;
	Gameobject* b;
	glm::dvec3 collisionNormal;
	glm::vec3 r1;
	glm::vec3 r2;

	void Update() override;
};
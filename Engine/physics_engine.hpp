#pragma once
#include "glm/vec3.hpp"
#include "constraint.hpp"
#include <vector>

class Gameobject;

class PhysicsEngine {
public:
	static PhysicsEngine& Get();

	void StepSimulation(double timestep);

	PhysicsEngine(const PhysicsEngine&) = delete;

	glm::dvec3 gravity = { 0.0, -9.81, 0.0 };


private:
	std::vector<DynamicCollisionConstraint> dynamicCollisions;
	std::vector<StaticCollisionConstraint> staticCollisions;

	PhysicsEngine();
	~PhysicsEngine();
};
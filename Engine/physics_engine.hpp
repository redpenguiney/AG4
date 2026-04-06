#pragma once
#include "glm/vec3.hpp"
#include "constraint.hpp"
#include <vector>
#include <random>

class Gameobject;

class PhysicsEngine {
public:
	static PhysicsEngine& Get();

	void StepSimulation(double timestep);

	PhysicsEngine(const PhysicsEngine&) = delete;

	glm::dvec3 gravity = { 0.0, -4.81, 0.0 };


private:
	unsigned currentShiftAmount;

	std::vector<DynamicCollisionConstraint> dynamicCollisions;
	std::vector<StaticCollisionConstraint> staticCollisions;
	std::default_random_engine rng;

	PhysicsEngine();
	~PhysicsEngine();
};
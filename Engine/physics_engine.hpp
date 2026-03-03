#pragma once
#include "glm/vec3.hpp"

class PhysicsEngine {
public:
	static PhysicsEngine& Get();

	void StepSimulation(double timestep);

	PhysicsEngine(const PhysicsEngine&) = delete;

	glm::dvec3 gravity = { 0.0, -1.81, 0.0 };


private:
	PhysicsEngine();
	~PhysicsEngine();
};
#include "physics_engine.hpp"

PhysicsEngine& PhysicsEngine::Get() {
	static PhysicsEngine PE;
	return PE;
}

void PhysicsEngine::StepSimulation(double timestep) {
}

PhysicsEngine::PhysicsEngine() {

}

PhysicsEngine::~PhysicsEngine() {

}

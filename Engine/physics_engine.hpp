#pragma once
#include "glm/vec3.hpp"
#include "constraint.hpp"
#include <vector>
#include <random>
#include <bitset>
#include <array>
#include "collision_layers.hpp"
#include "let_me_hash_a_tuple.hpp"
#include <plf_colony.h>
#include <unordered_set>

class Gameobject;

class PhysicsEngine {
public:
	static PhysicsEngine& Get();

	void StepSimulation(double timestep);

	PhysicsEngine(const PhysicsEngine&) = delete;

	glm::dvec3 gravity = { 0.0, -4.81, 0.0 };

	bool GetLayerCollisionEnabled(unsigned layer1, unsigned layer2);
	void SetLayerCollisionEnabled(unsigned layer1, unsigned layer2, bool collides);

	// add/remove joints at your discretion. 
	plf::colony<DynamicJoint> dynamicJoints;

	// add/remove joints at your discretion. 
	plf::colony<StaticJoint> staticJoints;

private:
	// always symmetric
	std::array<std::bitset<NUM_COLLISION_LAYERS>, NUM_COLLISION_LAYERS> collisionLayerMatrix;

	std::vector<DynamicCollisionConstraint> dynamicCollisions;
	std::vector<StaticCollisionConstraint> staticCollisions;
	
	std::default_random_engine rng;

	std::unordered_set<std::pair<Gameobject*, Gameobject*>, hash_pair::hash<Gameobject*, Gameobject*>> noCollidePairs;

	PhysicsEngine();
	~PhysicsEngine();
};
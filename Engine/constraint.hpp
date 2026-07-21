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
	glm::vec3 relV;
	float totalNormalLagrange;
	float totalTangentLagrange;
	float nerf;

	void PositionPass(float timestep, unsigned nIter);
	void VelocityPass(float timestep);
};

// physobject-physobject collision
struct DynamicCollisionConstraint {
	glm::vec3 r1;
	glm::vec3 r2;
	Physobject* a;
	Physobject* b;

	// B to A
	glm::vec3 collisionNormal;
	glm::vec3 relV;
	float totalNormalLagrange;
	float totalTangentLagrange;
	float nerf;

	void PositionPass(float timestep, unsigned nIter);
	void VelocityPass(float timestep);
};

struct JointParams {
	glm::vec3 forwardAxis; // in space of a, determines positive z-direction in joint space
	glm::vec3 upAxis; // in space of a, determines positive y-direction in joint space

	//glm::bvec3 translationLimited;
	//glm::vec3 minTranslation, maxTranslation;
	float maxDistance;

	float inverseStiffness;

	//glm::bvec3 rotationLimited;
	//glm::vec3 minRotation, maxRotation; // in radians around xyz axises in joint space
};

// physobject-gameobject highly configurable joint
struct StaticJoint {
	JointParams params;

	glm::vec3 r1 = glm::vec3(0, 0, 0);
	glm::vec3 r2 = glm::vec3(0, 0, 0);;
	Physobject* a;
	Gameobject* b;
	float lagrange = 0.0f;

	void PositionPass(float timestep, unsigned nIter);
	void VelocityPass(float timestep);
	void Reset();
};

// physobject-physobject highly configurable joint
struct DynamicJoint {
	JointParams params;

	glm::vec3 r1 = glm::vec3(0, 0, 0);;
	glm::vec3 r2 = glm::vec3(0, 0, 0);;
	Physobject* a;
	Physobject* b;
	float lagrange = 0.0f;

	void PositionPass(float timestep, unsigned nIter);
	void VelocityPass(float timestep);
	void Reset();
};


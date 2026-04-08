#include "constraint.hpp"
#include "glm/vec3.hpp"
#include "gameobject.hpp"

void StaticCollisionConstraint::VelocityPass(float timestep) {
	glm::vec3 rotatedR1 = a->GetRotSclMatrix() * r1;
	glm::vec3 currentRelV = a->velocity + glm::cross(a->rotVelocity, rotatedR1);
	float currentNormalSpeed = glm::dot(collisionNormal, currentRelV);
	float priorNormalSpeed = glm::dot(collisionNormal, relV);
	//if (priorNormalSpeed > -0.001) priorNormalSpeed = 0.0f; // prevent jitter and backwards restitution

	glm::vec3 tangentVelocity = currentRelV - collisionNormal * currentNormalSpeed;
	float tangentSpeed = glm::length(tangentVelocity);

	//DebugLogInfo("V = ", currentRelV, " was ", collision.relV);

	float restitution = (a->elasticity + b->elasticity) * 0.5f;
	float normalForce = totalLagrange / timestep; // this is actually normalForce * timestep, divide by timestep again for the actual force
	float friction = (a->friction + b->friction) * 0.5f;
	float desiredNormalSpeed = -restitution * priorNormalSpeed;
	float neededDv = desiredNormalSpeed - currentNormalSpeed;


	// std::min prevents funky behavior when the two objects were intersecting before the frame started (TODO NO IT DOESNT)
	glm::vec3 deltaV = /*collision.nerf **/ collisionNormal * (-currentNormalSpeed - std::min(0.0f, restitution * priorNormalSpeed));

	//DebugLogInfo("desired ", desiredNormalSpeed, " solution ", deltaV.y, " currentV = ", collision.a->velocity);

	if (tangentSpeed != 0) {
		//DebugLogInfo("Tangent speed", tangentSpeed, " v = ", collision.a->velocity);
		glm::vec3 frictionDirection = -tangentVelocity / tangentSpeed;
		deltaV += frictionDirection * glm::min(normalForce * friction, tangentSpeed);
	}

	glm::vec3 torqueAxis1 = glm::cross(rotatedR1, glm::normalize(deltaV));
	float inertiaAroundTorqueAxis = 0;
	if (glm::length2(torqueAxis1) != 0) {
		auto localAxis = glm::inverse(a->Rotation()) * glm::normalize(torqueAxis1);
		inertiaAroundTorqueAxis = glm::dot(localAxis, a->inverseInertiaTensor * localAxis);
	}
	float reducedInverseMass1 = a->inverseMass + glm::length2(torqueAxis1) * inertiaAroundTorqueAxis;

	glm::vec3 impulse = deltaV / reducedInverseMass1;
	a->nextVel += impulse * a->inverseMass;
	a->nextRotVel += inertiaAroundTorqueAxis * glm::cross(rotatedR1, impulse);

	//DebugLogInfo("NEXTVEL ", collision.a->nextVel.y);
	//break;
}

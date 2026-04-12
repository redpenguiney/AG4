#include "constraint.hpp"
#include "glm/vec3.hpp"
#include "gameobject.hpp"
#include "assert.hpp"
#include "debug_prefabs.hpp"

void StaticCollisionConstraint::PositionPass(float timestep, unsigned posIter){
	glm::vec3 rotatedR1 = a->GetRotSclMatrix() * r1;
	glm::vec3 rotatedR2 = b->GetRotSclMatrix() * r2; // TODO: for static collisions we should precalculate the rotated r2

	if (posIter == 0) {
		relV = a->velocity + glm::cross(a->rotVelocity, rotatedR1);
	}

	//DebugPoint(glm::dvec3(rotatedR1) + a->position, {1, 0.5, 0});
	//DebugPoint(glm::dvec3(rotatedR2) + b->Position(), {1, 0, 0});

	// todo: could maybe evaluate these in A's object space and then use floats?
	glm::dvec3 dnormal = glm::dvec3(collisionNormal);
	double penetration = glm::dot(glm::dvec3(rotatedR2) + b->Position() - glm::dvec3(rotatedR1) - a->Position(), dnormal);
	if (penetration < 0) { // todo: we're having way too many of these given that N_POS_ITERS == 1, wasting perf
		//DebugLogInfo("Fake news collision, p=", penetration, " n=", dnormal); // expected if N_POS_ITERS > 1
		return;
	}
	glm::vec3 torqueAxis1 = glm::cross(rotatedR1, collisionNormal);

	//DebugLogInfo("N ", dnormal, " R1 ", rotatedR1, " p = ", penetration);

	float inertiaAroundTorqueAxis = 0;
	if (glm::length2(torqueAxis1) != 0) {
		auto localAxis = glm::inverse(a->Rotation()) * glm::normalize(torqueAxis1);
		inertiaAroundTorqueAxis = glm::dot(localAxis, a->inverseInertiaTensor * localAxis);
	}
	//DebugLogInfo("MMOI ", inertiaAroundTorqueAxis, " times ", glm::length2(torqueAxis1));
	float reducedInverseMass1 = a->inverseMass + glm::length2(torqueAxis1) * inertiaAroundTorqueAxis;

	float lagrange = penetration / reducedInverseMass1;
	totalNormalLagrange += lagrange;
	glm::vec3 impulse = collisionNormal * lagrange;
	glm::dvec3 displacement = impulse * a->inverseMass;
	//DebugLogInfo("Displacement strength ", glm::length(displacement), " against penetration ", penetration);
	glm::vec3 torque = glm::cross(rotatedR1, impulse);
	glm::vec3 dRot = inertiaAroundTorqueAxis * torque;

	//glm::vec3 dRot = collision.a->inverseInertiaTensor * torque;
	a->nextPos += displacement;
	a->nextRot = a->nextRot + 0.5f * glm::quat(0, dRot.x, dRot.y, dRot.z) * a->Rotation();

	//DebugLogInfo("IMPULSE ", impulse, " DROT ", dRot)
	//glm::dvec3 newR1 = glm::dvec3(collision.a->GetRotSclMatrix() * collision.r1) + collision.a->Position();
	//double newPenetration = glm::dot(r2 - newR1, dnormal);
	//DebugLogInfo("DIS ", displacement);
	//collision.a->SetPosition(collision.a->Position() + dnormal * penetration);
}

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
	float normalForce = totalNormalLagrange / timestep; // this is actually normalForce * timestep, divide by timestep again for the actual force
	float friction = (a->friction + b->friction) * 0.5f;
	float desiredNormalSpeed = -restitution * priorNormalSpeed;
	float neededDv = desiredNormalSpeed - currentNormalSpeed;


	// std::min prevents funky behavior when the two objects were intersecting before the frame started (TODO NO IT DOESNT)
	glm::vec3 deltaV = nerf * collisionNormal * (-currentNormalSpeed - std::min(0.0f, restitution * priorNormalSpeed));

	if (tangentSpeed != 0) {
		glm::vec3 frictionDirection = -tangentVelocity / tangentSpeed;
		deltaV += frictionDirection * glm::min(normalForce * friction, tangentSpeed);
	}

	float deltaSpeed = glm::length2(deltaV);
	if (deltaSpeed != 0) {
		deltaSpeed = std::sqrtf(deltaSpeed);
		glm::vec3 torqueAxis1 = glm::cross(rotatedR1, deltaV/deltaSpeed);
		float inertiaAroundTorqueAxis = 0;
		if (glm::length2(torqueAxis1) != 0) {
			auto localAxis = glm::inverse(a->Rotation()) * glm::normalize(torqueAxis1);
			inertiaAroundTorqueAxis = glm::dot(localAxis, a->inverseInertiaTensor * localAxis);
		}
		float reducedInverseMass1 = a->inverseMass + glm::length2(torqueAxis1) * inertiaAroundTorqueAxis;

		glm::vec3 impulse = deltaV / reducedInverseMass1;
		a->nextVel += impulse * a->inverseMass;
		a->nextRotVel += inertiaAroundTorqueAxis * glm::cross(rotatedR1, impulse);
		Assert(!std::isnan(a->nextVel.x));
	}
}

void DynamicCollisionConstraint::PositionPass(float timestep, unsigned posIter) {
	glm::vec3 rotatedR1 = a->GetRotSclMatrix() * r1;
	glm::vec3 rotatedR2 = b->GetRotSclMatrix() * r2;

	//DebugPoint(glm::dvec3(rotatedR1) + a->position);
	//DebugPoint(glm::dvec3(rotatedR2) + b->position, {0, 0, 0});

	if (posIter == 0) {
		relV = a->velocity + glm::cross(a->rotVelocity, rotatedR1) - b->velocity - glm::cross(b->rotVelocity, rotatedR2);
	}

	// todo: could maybe evaluate these in A's object space and then use floats?
	glm::dvec3 dnormal = glm::dvec3(collisionNormal);
	double penetration = glm::dot(glm::dvec3(rotatedR2) + b->Position() - glm::dvec3(rotatedR1) - a->Position(), dnormal);
	if (penetration < 0) { // todo: we're having way too many of these given that N_POS_ITERS == 1, wasting perf
		return;
	}

	glm::vec3 torqueAxis1 = glm::cross(rotatedR1, collisionNormal);
	float inertiaAroundTorqueAxis1 = 0;
	if (glm::length2(torqueAxis1) != 0) {
		auto localAxis = glm::inverse(a->Rotation()) * glm::normalize(torqueAxis1);
		inertiaAroundTorqueAxis1 = glm::dot(localAxis, a->inverseInertiaTensor * localAxis);
	}
	glm::vec3 torqueAxis2 = glm::cross(rotatedR2, collisionNormal);
	float inertiaAroundTorqueAxis2 = 0;
	if (glm::length2(torqueAxis2) != 0) {
		auto localAxis = glm::inverse(b->Rotation()) * glm::normalize(torqueAxis2);
		inertiaAroundTorqueAxis2 = glm::dot(localAxis, b->inverseInertiaTensor * localAxis);
	}
	float reducedInverseMass = a->inverseMass + glm::length2(torqueAxis1) * inertiaAroundTorqueAxis1 + b->inverseMass + glm::length2(torqueAxis2) * inertiaAroundTorqueAxis2;

	float lagrange = penetration / reducedInverseMass;
	totalNormalLagrange += lagrange;
	glm::vec3 impulse = collisionNormal * lagrange;
	glm::vec3 dRot1 = inertiaAroundTorqueAxis1 * glm::cross(rotatedR1, impulse);
	glm::vec3 dRot2 = inertiaAroundTorqueAxis2 * glm::cross(impulse, rotatedR2);

	//glm::vec3 dRot = collision.a->inverseInertiaTensor * torque;
	a->nextPos += impulse * a->inverseMass;
	a->nextRot = a->nextRot + 0.5f * glm::quat(0, dRot1.x, dRot1.y, dRot1.z) * a->Rotation();
	b->nextPos -= impulse * b->inverseMass;
	b->nextRot = b->nextRot + 0.5f * glm::quat(0, dRot2.x, dRot2.y, dRot2.z) * b->Rotation();
}

void DynamicCollisionConstraint::VelocityPass(float timestep) {
	glm::vec3 rotatedR1 = a->GetRotSclMatrix() * r1;
	glm::vec3 rotatedR2 = b->GetRotSclMatrix() * r2;
	glm::vec3 currentRelV = a->velocity + glm::cross(a->rotVelocity, rotatedR1) - b->velocity - glm::cross(b->rotVelocity, rotatedR2);
	float currentNormalSpeed = glm::dot(collisionNormal, currentRelV);
	float priorNormalSpeed = glm::dot(collisionNormal, relV);
	//if (priorNormalSpeed > -0.001) priorNormalSpeed = 0.0f; // prevent jitter and backwards restitution

	glm::vec3 tangentVelocity = currentRelV - collisionNormal * currentNormalSpeed;
	float tangentSpeed = glm::length(tangentVelocity);


	float restitution = (a->elasticity + b->elasticity) * 0.5f;
	float normalForce = totalNormalLagrange / timestep; // this is actually normalForce * timestep, divide by timestep again for the actual force
	float friction = (a->friction + b->friction) * 0.5f;
	float desiredNormalSpeed = -restitution * priorNormalSpeed;
	float neededDv = desiredNormalSpeed - currentNormalSpeed;

	// std::min prevents funky behavior when the two objects were intersecting before the frame started (TODO NO IT DOESNT)
	glm::vec3 deltaV = nerf * collisionNormal * (-currentNormalSpeed - std::min(0.0f, restitution * priorNormalSpeed));

	if (tangentSpeed != 0) {
		glm::vec3 frictionDirection = -tangentVelocity / tangentSpeed;
		deltaV += frictionDirection * glm::min(normalForce * friction, tangentSpeed);
	}

	float deltaSpeed = glm::length2(deltaV);
	if (deltaSpeed != 0) {
		deltaSpeed = std::sqrtf(deltaSpeed);
		glm::vec3 torqueAxis1 = glm::cross(rotatedR1, deltaV / deltaSpeed);
		float inertiaAroundTorqueAxis1 = 0;
		if (glm::length2(torqueAxis1) != 0) {
			auto localAxis = glm::inverse(a->Rotation()) * glm::normalize(torqueAxis1);
			inertiaAroundTorqueAxis1 = glm::dot(localAxis, a->inverseInertiaTensor * localAxis);
		}
		glm::vec3 torqueAxis2 = glm::cross(rotatedR2, deltaV / deltaSpeed);
		float inertiaAroundTorqueAxis2 = 0;
		if (glm::length2(torqueAxis2) != 0) {
			auto localAxis = glm::inverse(b->Rotation()) * glm::normalize(torqueAxis2);
			inertiaAroundTorqueAxis2 = glm::dot(localAxis, b->inverseInertiaTensor * localAxis);
		}

		float reducedInverseMass = a->inverseMass + glm::length2(torqueAxis1) * inertiaAroundTorqueAxis1 + b->inverseMass + glm::length2(torqueAxis2) * inertiaAroundTorqueAxis2;
		glm::vec3 impulse = deltaV / reducedInverseMass;
		a->nextVel += impulse * a->inverseMass;
		a->nextRotVel += inertiaAroundTorqueAxis1 * glm::cross(rotatedR1, impulse);
		b->nextVel -= impulse * b->inverseMass;
		b->nextRotVel -= inertiaAroundTorqueAxis2 * glm::cross(rotatedR2, impulse);
		Assert(!std::isnan(a->nextVel.x));
	}
}

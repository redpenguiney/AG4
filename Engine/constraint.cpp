#include "constraint.hpp"
#include "glm/vec3.hpp"
#include "gameobject.hpp"
#include "assert.hpp"
#include "debug_prefabs.hpp"

struct PBDHelpers {

	// for physobject-gameobject interaction, where the gameobject is 2.
	// r1/r2 should be rotated.
	static void ApplyPositionalCorrectionStatic(Physobject* obj1, glm::vec3 rotatedR1, glm::vec3 correctionDirection, float correctionStrength, float& currentLagrange, float compliance, float timestep) {
		glm::vec3 torqueAxis1 = glm::cross(rotatedR1, correctionDirection);

		//DebugLogInfo("N ", dnormal, " R1 ", rotatedR1, " p = ", penetration);

		auto [reducedInverseMass1, inertiaAroundTorqueAxis1] = obj1->GetInverseReducedMass(torqueAxis1);
		Assert(!std::isnan(reducedInverseMass1) && !std::isnan(inertiaAroundTorqueAxis1));

		float adjustedCompliance = compliance / timestep / timestep;
		float deltaLagrange = (correctionStrength - currentLagrange * adjustedCompliance) / (reducedInverseMass1 + adjustedCompliance);

		glm::vec3 impulse = correctionDirection * deltaLagrange;
		glm::dvec3 displacement = impulse * obj1->inverseMass;
		//DebugLogInfo("Displacement strength ", glm::length(displacement), " against penetration ", penetration);
		glm::vec3 torque = glm::cross(rotatedR1, impulse);
		glm::vec3 dRot = inertiaAroundTorqueAxis1 * torque;

		//glm::vec3 dRot = collision.a->inverseInertiaTensor * torque;
		obj1->nextPos += displacement;
		obj1->nextRot = obj1->nextRot + 0.5f * glm::quat(0, dRot.x, dRot.y, dRot.z) * obj1->Rotation();

		currentLagrange += deltaLagrange;
	}

	// for physobject-physobject interaction
	// r1/r2 should be rotated.
	static void ApplyPositionalCorrectionDynamic(Physobject* obj1, Physobject* obj2, glm::vec3 rotatedR1, glm::vec3 rotatedR2, glm::vec3 correctionDirection, float correctionStrength, float& currentLagrange, float compliance, float timestep) {
		glm::vec3 torqueAxis1 = glm::cross(rotatedR1, correctionDirection);
		auto [reducedInverseMass1, inertiaAroundTorqueAxis1] = obj1->GetInverseReducedMass(torqueAxis1);
		glm::vec3 torqueAxis2 = glm::cross(rotatedR2, correctionDirection);
		auto [reducedInverseMass2, inertiaAroundTorqueAxis2] = obj2->GetInverseReducedMass(torqueAxis2);
		float reducedInverseMass = reducedInverseMass1 + reducedInverseMass2;

		float adjustedCompliance = compliance / timestep / timestep;
		float deltaLagrange = (correctionStrength - currentLagrange * adjustedCompliance) / (reducedInverseMass + adjustedCompliance);
		
		glm::vec3 impulse = correctionDirection * deltaLagrange;
		glm::vec3 dRot1 = inertiaAroundTorqueAxis1 * glm::cross(rotatedR1, impulse);
		glm::vec3 dRot2 = inertiaAroundTorqueAxis2 * glm::cross(impulse, rotatedR2);

		//glm::vec3 dRot = collision.a->inverseInertiaTensor * torque;
		obj1->nextPos += impulse * obj1->inverseMass;
		obj1->nextRot = obj1->nextRot + 0.5f * glm::quat(0, dRot1.x, dRot1.y, dRot1.z) * obj1->Rotation();
		obj2->nextPos -= impulse * obj2->inverseMass;
		obj2->nextRot = obj2->nextRot + 0.5f * glm::quat(0, dRot2.x, dRot2.y, dRot2.z) * obj2->Rotation();

		currentLagrange += deltaLagrange;
	}

};

void StaticCollisionConstraint::PositionPass(float timestep, unsigned posIter){
	glm::vec3 rotatedR1 = a->GetRotSclMatrix() * r1;
	glm::vec3 rotatedR2 = b->GetRotSclMatrix() * r2; // todo: for static collisions we should precalculate the rotated r2

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
	
	PBDHelpers::ApplyPositionalCorrectionStatic(a, rotatedR1, dnormal, penetration, totalNormalLagrange, 0, timestep);
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
		deltaSpeed = glm::sqrt(deltaSpeed);
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

	PBDHelpers::ApplyPositionalCorrectionDynamic(a, b, rotatedR1, rotatedR2, collisionNormal, penetration, totalNormalLagrange, 0, timestep);
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
		deltaSpeed = glm::sqrt(deltaSpeed);
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

void DynamicJoint::PositionPass(float timestep, unsigned nIter) {
	glm::vec3 rotatedR1 = a->GetRotSclMatrix() * r1;
	glm::vec3 rotatedR2 = b->GetRotSclMatrix() * r2;

	glm::vec3 dPos = glm::dvec3(rotatedR2) + b->Position() - glm::dvec3(rotatedR1) - a->Position();
	float distance = glm::length(dPos);
	if (distance > params.maxDistance) {
		float error = distance - params.maxDistance;
		PBDHelpers::ApplyPositionalCorrectionDynamic(a, b, rotatedR1, rotatedR2, glm::normalize(dPos), error, lagrange, params.inverseStiffness, timestep);
	}
}

void DynamicJoint::VelocityPass(float timestep) {

}

void DynamicJoint::Reset() {
	lagrange = 0;
}

void StaticJoint::PositionPass(float timestep, unsigned nIter) {
	glm::vec3 rotatedR1 = a->GetRotSclMatrix() * r1;
	glm::vec3 rotatedR2 = b->GetRotSclMatrix() * r2; // todo: for static joints we should precalculate the rotated r2

	glm::vec3 dPos = glm::dvec3(rotatedR2) + b->Position() - glm::dvec3(rotatedR1) - a->Position();
	float distance = glm::length(dPos);
	if (distance > params.maxDistance) {
		float error = distance - params.maxDistance;
		PBDHelpers::ApplyPositionalCorrectionStatic(a, rotatedR1, glm::normalize(dPos), error, lagrange, params.inverseStiffness, timestep);
	}
}

void StaticJoint::VelocityPass(float timestep) {

}

void StaticJoint::Reset() {
	lagrange = 0;
}

#include "ik.hpp"
#include <log.hpp>
#include "gameobject.hpp"

std::vector<EndEffector> IKSkeletonInfo::GetEndEffectors() {
	std::vector<EndEffector> endEffectors;
	hip.AddEndEffectors(endEffectors);
	return endEffectors;
}

void IKSkeletonInfo::Init() {
	hip.currentPosition = glm::vec3(0.0f);
	hip.currentRotation = glm::identity<glm::quat>();
	hip.parent = nullptr;
	hip.PropagatePosition();

}

void IKSkeletonInfo::DoIK(unsigned maxIterations, float tolerance) {
	auto endEffectors = GetEndEffectors(); // todo: what order should this be in? randomized?

	float tolerance2 = tolerance * tolerance;

	for (unsigned i = 0; i < maxIterations; i++) {
		float GREEDINESS = 0.1f;

		for (auto& endEffector : endEffectors) {
			auto diff = endEffector.end->currentPosition - endEffector.end->goalPosition.value();
			if (glm::dot(diff, diff) > tolerance2) { // the dot product  is just distance squared
				goto notDone;
			}
		}
		return; // all joints are within tolerance
	notDone:;

		for (auto& endEffector : endEffectors) {
			auto current = endEffector.end;
			auto goal = *endEffector.end->goalPosition;

			// 1st pass, end to root
			while (current) {
				glm::vec3 currentGoalPos = endEffector.end->CurrentEndPosition();

				glm::vec3 currentDir = current->baseDirection;
				glm::vec3 desiredDir = goal - current->currentPosition;
				// TODO: clamp desiredDir based on limits
				glm::quat rot = glm::rotation(currentDir, desiredDir);
				// TODO: clamp twist
				rot = glm::slerp(glm::identity<glm::quat>(), rot, GREEDINESS); // apply greediness by reducing rotation amount (yes, slerp is apparently how that's done?)

				current->Rotate(rot);
				current = current->parent;
			}

			// 2nd pass, subroot to end
			for (auto& bone : endEffector.subrootToEnd) {
				glm::vec3 currentGoalPos = endEffector.end->CurrentEndPosition();

				glm::vec3 currentDir = bone->baseDirection;
				glm::vec3 desiredDir = goal - bone->currentPosition;
				// TODO: clamp desiredDir based on limits
				glm::quat rot = glm::rotation(currentDir, desiredDir);
				// TODO: clamp twist
				rot = glm::slerp(glm::identity<glm::quat>(), rot, GREEDINESS); // apply greediness
				bone->Rotate(rot);
			}
		}
	}

	DebugLogInfo("DoIK() did not converge within ", maxIterations, " iterations.");
}

void IKSkeletonInfo::Apply(Gameobject* object) {
	hip.ApplyToGameobject(object);
}

glm::vec3 IKBone::CurrentEndPosition() {
	return currentPosition + (currentRotation * baseDirection) * length;
}

void IKBone::AddEndEffectors(std::vector<EndEffector>& endEffectors) {
	if (goalPosition.has_value()) {
		endEffectors.push_back({ this, FindSubroot() });
	}
	for (auto& child : children) {
		child.AddEndEffectors(endEffectors);
	}
}

void IKBone::PropagatePosition() {

	numEndEffectorsInDescendants = 0;

	if (goalPosition.has_value()) {
		auto current = this;
		while (current) {
			current->numEndEffectorsInDescendants++;
			current = current->parent;
		}
	}

	for (auto& child : children) {
		child.parent = this;
		child.currentPosition = CurrentEndPosition();
		child.PropagatePosition();
	}


}

void IKBone::ApplyToGameobject(Gameobject* obj) {
	glm::mat4x4 transform = glm::toMat4(currentRotation);
	transform[3] = glm::vec4(currentPosition, 1.0f);
	obj->SetBoneTransform(boneIndex, transform);
	for (auto& c : children) {
		c.ApplyToGameobject(obj);
	}
}

std::vector<IKBone*> IKBone::FindSubroot() {
	std::vector<IKBone*> result;

	auto current = this;
	while (current) {

		result.insert(result.begin(), current); // insert at beginning, so that the subroot is first and end is last

		if (current->numEndEffectorsInDescendants > 1) {
			return result;
		}
		current = current->parent;
	}


	return result; // hip
}

void IKBone::Rotate(glm::quat dRot) {
	currentRotation = dRot * currentRotation;
	for (auto& child : children) {
		child.currentPosition = CurrentEndPosition();
		child.Rotate(dRot); // TODO SUS
	}
}

#pragma once
#include <vector>
#include <optional>
#include <memory>
#include <glm/vec3.hpp>
#include <glm/ext/quaternion_float.hpp>

struct IKBone;
class Mesh;
class Gameobject;

struct EndEffector {
	IKBone* end;
	std::vector<IKBone*> subrootToEnd; // closest ancestor of end that has multiple descendants with goalPosition set, going down children until end is reached (end is included). needed to simultaneously solve multiple end effectors
};

struct IKBone {
	unsigned boneIndex; // in the Skeleton
	unsigned numEndEffectorsInDescendants;
	float length;
	std::vector<IKBone> children;
	IKBone* parent;

	glm::vec3 baseDirection; // Normalized axis around which the bone is oriented when rotation is identity, and around which rotation limits are defined.
	glm::vec3 secondaryDirection; // perpendicular to baseDirection, the "up" direction which twist changes
	float minTwist;
	float maxTwist;
	float minPitch;
	float maxPitch;
	float minYaw;
	float maxYaw;

	glm::quat currentRotation; // in hip space, not relative to parent
	glm::vec3 currentPosition; // relative to hip
	std::optional<glm::vec3> goalPosition; // relative to hip

	glm::vec3 CurrentEndPosition();

	// helper for GetEndEffectors()
	void AddEndEffectors(std::vector<EndEffector>& endEffectors);

	// helper for GetEndEffectors()
	std::vector<IKBone*> FindSubroot();

	// helper for DoIK()
	void Rotate(glm::quat dRot);

	// helper for InitPositions()
	void PropagatePosition();

	// helper for Apply()
	void ApplyToGameobject(Gameobject* obj);
};


struct IKSkeletonInfo {
	IKBone hip;

	// (the bones with goalPosition set)
	std::vector<EndEffector> GetEndEffectors();

	// don't call every frame, warmstarting helps performance
	void Init();

	void DoIK(unsigned maxIterations, float tolerance);

	// updates object skeleton. The mesh you used to generate the IKSkeletonInfo obviously must match up.
	void Apply(Gameobject* object);
};
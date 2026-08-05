#pragma once
#include "component.hpp"
#include "gameobject.hpp"
#include <vector>
#include <memory>

// transform hierarchy. 
// use the Transform() method to cause children to move with the parent object. Setting individual object positions otherwise will just change their relative offsets.
// Only performance cost is when calling Transform() or modifying children.
// Works recursively.
class Hierarchy : public BaseComponent {
public:
	// if updatePhysics, then Transform() will automatically be called every frame with the current position/rot/scl to ensure that children are always moved with the object. (useful for physics ig)
		// in this case, do NOT have other Physobjects as a child of this Hierarchy.
	Hierarchy(Gameobject* obj, bool updatePhysics = false);

	void AddChild(std::unique_ptr<Gameobject> child);

	// O(n children) complexity, returns nullptr if no such child
	std::unique_ptr<Gameobject> ReleaseChild(Gameobject* whichOne);

	const std::vector<std::unique_ptr<Gameobject>>& GetChildren();

	std::vector<std::unique_ptr<Gameobject>> ReleaseChildren();
	
	void PrePhysics(float dt) override;
	void PostPhysics(float dt) override;

	void Transform(glm::dvec3 newPos, glm::quat newRotation, glm::vec3 newScale);
	void Transform(glm::dvec3 newPos);
	void Transform(glm::dvec3 newPos, glm::quat newRotation);
	void Transform(glm::quat newRotation);

	~Hierarchy();
private:
	void ApplyTransform(glm::dvec3 newPos, glm::quat newRotation, glm::vec3 newScale);

	const bool updatePhysics = false;
	glm::dvec3 lastPos;
	glm::quat lastRot;
	glm::vec3 lastScale;
	std::vector<std::unique_ptr<Gameobject>> children;
};
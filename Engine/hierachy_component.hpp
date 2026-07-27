#pragma once
#include "component.hpp"
#include "gameobject.hpp"
#include <vector>
#include <memory>

// transform hierarchy. 
// Physobjects should never have parents.
class Hierarchy : public BaseComponent {
public:
	Hierarchy(Gameobject* obj);

	void SetParent(Hierarchy* parent);
	
	const std::vector<Hierarchy*>& GetChildren();

	void PostPhysics(float dt) override;

	~Hierarchy();
private:

	Hierarchy* parent;
	std::vector<Hierarchy*> children;
	std::vector<std::unique_ptr<Gameobject>> ownedChildren;
};
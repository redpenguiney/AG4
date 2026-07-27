#pragma once
#include <concepts>
#include <vector>
#include <memory>
#include "event.hpp"
#include <plf_colony.h>
class Gameobject;
class BaseComponent;

class ComponentManager {
public:
	static ComponentManager& Get();

private:
	plf::colony<BaseComponent*> components;
	Connection renderConnection;
	Connection prePhysicsConnection;
	Connection postPhysicsConnection;

	ComponentManager();
	~ComponentManager() = default;

	friend class BaseComponent;
};

// Define and attach arbitrary components to Gameobjects to add custom behavior/storage.
class BaseComponent {
public:
	Gameobject* const object;

	BaseComponent(Gameobject* obj);
	virtual ~BaseComponent();

	virtual void PreRender(float dt); // base does nothing, don't worry about calling base implementation in your derived component classes
	virtual void PrePhysics(float dt); // base does nothing, don't worry about calling base implementation in your derived component classes
	virtual void PostPhysics(float dt); // base does nothing, don't worry about calling base implementation in your derived component classes

private:
	plf::colony<BaseComponent*>::iterator location;
};

template <class T>
concept ComponentType = std::derived_from<T, BaseComponent>;

struct GameobjectComponents {
	std::vector<std::unique_ptr<BaseComponent>> components;
};
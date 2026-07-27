#pragma once
#include <concepts>
#include <unordered_map>

class Gameobject;

// Define and attach arbitrary components to Gameobjects to add custom behavior/storage.
class BaseComponent {
public:
	Gameobject* const object;

protected:
	virtual void PreRender(float dt);
	virtual void PrePhysics(float dt);
};

template <class T>
concept ComponentType = std::derived_from<T, BaseComponent>;
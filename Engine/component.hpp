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

// (curiously recurring template pattern)
template <typename T>
class ComponentCRTP {
	std::unordered_map<Gameobject*, std::unique_ptr<T>>;
};

template <class T>
concept Component = std::derived_from<T, BaseComponent>;
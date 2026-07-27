#include "component.hpp"
#include <mainloop.hpp>

ComponentManager& ComponentManager::Get() {
	static ComponentManager m;
	return m;
}

ComponentManager::ComponentManager() {
	renderConnection = Mainloop::Get().preRender.Connect([this](Mainloop*, float dt) {
		for (auto& comp : components) {
			comp->PreRender(dt);
		}
		});
	prePhysicsConnection = Mainloop::Get().prePhysics.Connect([this](Mainloop*, float dt) {
		for (auto& comp : components) {
			comp->PrePhysics(dt);
		}
		});
	postPhysicsConnection = Mainloop::Get().postPhysics.Connect([this](Mainloop*, float dt) {
		for (auto& comp : components) {
			comp->PostPhysics(dt);
		}
		});
}

BaseComponent::BaseComponent(Gameobject* obj): object(obj) {
	location = ComponentManager::Get().components.insert(this);
}

BaseComponent::~BaseComponent() {
	ComponentManager::Get().components.erase(location);
}

void BaseComponent::PreRender(float dt) {
}

void BaseComponent::PrePhysics(float dt) {
}

void BaseComponent::PostPhysics(float dt) {
}

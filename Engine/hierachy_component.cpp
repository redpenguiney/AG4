#include "hierachy_component.hpp"

Hierarchy::Hierarchy(Gameobject* obj, bool updatePhysics): BaseComponent(obj), updatePhysics(updatePhysics) {
	lastPos = object->Position();
	lastRot = object->Rotation();
	lastScale = object->Scale();
}

void Hierarchy::AddChild(std::unique_ptr<Gameobject> child) {
	//Hierarchy* c = child->GetComponent<Hierarchy>();
	//if (!c) c = child->AddComponent<Hierarchy>();
	//c->parent = this;
	children.push_back(std::move(child));
}

std::unique_ptr<Gameobject> Hierarchy::ReleaseChild(Gameobject* whichOne)
{
	for (size_t i = 0; i < children.size(); i++) {
		if (children[i].get() == whichOne) {
			auto ret = std::move(children[i]);
			children[i] = std::move(children.back());
			children.pop_back();
			return ret;
		}
	}
	return nullptr;
}

const std::vector<std::unique_ptr<Gameobject>>& Hierarchy::GetChildren() {
	return children;
}

std::vector<std::unique_ptr<Gameobject>> Hierarchy::ReleaseChildren()
{
	return std::move(children);
}

Hierarchy::~Hierarchy() {
	
}

void Hierarchy::PrePhysics(float dt)
{
}

void Hierarchy::PostPhysics(float dt) {
	if (updatePhysics) {
		ApplyTransform(object->Position(), object->Rotation(), object->Scale());
	}
}

void Hierarchy::ApplyTransform(glm::dvec3 newPos, glm::quat newRotation, glm::vec3 newScale) {
	glm::dvec3 dPos = newPos - lastPos;
	glm::quat dRot = newRotation * glm::inverse(lastRot);
	glm::dvec3 dScale = newScale / lastScale;

	if (dPos == glm::dvec3(0) && dRot == glm::identity<glm::quat>() && dScale == glm::dvec3(1)) {
		return;
	}



	for (auto& c : children) {
		glm::vec3 cScale = c->Scale() * glm::vec3(dScale);
		glm::quat cRot = dRot * c->Rotation();

		glm::dvec3 priorRelP = c->Position() - lastPos;
		glm::dvec3 cPos = newPos + glm::dvec3(dRot * glm::vec3(dScale * priorRelP));
		auto h = c->GetComponent<Hierarchy>();
		if (h) {
			h->Transform(cPos, cRot, cScale);
		}
		else {
			c->SetPosition(cPos);
			c->SetRotation(cRot);
			c->SetScale(cScale);
		}
	}

	object->SetPosition(newPos);
	object->SetRotation(newRotation);
	object->SetScale(newScale);

	lastPos = object->Position();
	lastRot = object->Rotation();
	lastScale = object->Scale();
}

void Hierarchy::Transform(glm::dvec3 newPos, glm::quat newRotation, glm::vec3 newScale) {
	ApplyTransform(newPos, newRotation, newScale);
}

void Hierarchy::Transform(glm::dvec3 newPos)
{
	ApplyTransform(newPos, object->Rotation(), object->Scale());
}

void Hierarchy::Transform(glm::dvec3 newPos, glm::quat newRotation)
{
	ApplyTransform(newPos, newRotation, object->Scale());
}

void Hierarchy::Transform(glm::quat newRotation)
{
	ApplyTransform(object->Position(), newRotation, object->Scale());
}

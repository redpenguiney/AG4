#include "debug_editing_tools.hpp"
#include "mesh.hpp"
#include "log.hpp"

struct HandlesState {
	Gameobject* gripped = nullptr;
	glm::dvec3 cursorGripOffset;
	glm::dvec3 originalPos;
	glm::quat originalRot;
	std::unique_ptr<Gameobject> posX, posY, posZ, rotX, rotY, rotZ;

	std::vector<Connection> connections;

	~HandlesState() = default;

};

void TransformHandles(Gameobject* target) {
	std::shared_ptr<HandlesState> state = std::shared_ptr<HandlesState>(new HandlesState());

	auto transformAxis = [&](glm::dvec3 dir) -> Gameobject* {
		Gameobject* arrow = DebugArrow(target->Position(), dir, dir);
		arrow->GetCollider()->layer = 1;

		state->connections.push_back(Mainloop::Get().preRender.Connect([arrow, target, dir, state = state.get()](Mainloop*, float) {
			if (state->gripped == arrow) {
				arrow->SetInstanceAttribute(*GetArrowMesh()->format.GetAttribute("color"), {1, 1, 1, 0.2});

				glm::dvec3 currentCursorDir = GraphicsEngine::Get().currentCamera->ProjectToWorld(Window::Get().MOUSE_POS, Window::Get().Size());
				//glm::dvec2 arrowScreenDir = glm::normalize(GraphicsEngine::Get().currentCamera->ProjectPointToScreen(dir, Window::Get().Aspect()));
				//target->SetPosition(state->originalPos + dir * 0.01 * glm::dot(arrowScreenDir, Window::Get().MOUSE_POS - state->cursorGripPosition));
				glm::dvec3 relevantOffset = dir * glm::dot(state->cursorGripOffset, dir);
				target->SetPosition(ClosestPointOnLine1ToLine2(state->originalPos, dir, GraphicsEngine::Get().currentCamera->position, currentCursorDir) - relevantOffset);
			}
			else {
				arrow->SetInstanceAttribute(*GetArrowMesh()->format.GetAttribute("color"), { dir.x, dir.y, dir.z, 0.2 });
			}
			arrow->SetPosition(target->Position() + dir * 0.6);
			}));

		return arrow;
		};

	state->posX = std::unique_ptr<Gameobject>(transformAxis({ 1, 0, 0 }));
	state->posY = std::unique_ptr<Gameobject>(transformAxis({ 0, 1, 0 }));
	state->posZ = std::unique_ptr<Gameobject>(transformAxis({ 0, 0, 1 }));

	auto rotateAxis = [&](glm::dvec3 dir) -> Gameobject* {
		Gameobject* rotateHandle = DebugHulaHoop(target->Position(), dir, dir);
		rotateHandle->GetCollider()->layer = 1;

		state->connections.push_back(Mainloop::Get().preRender.Connect([rotateHandle, target, dir, state = state.get()](Mainloop*, float) {
			if (state->gripped == rotateHandle) {
				rotateHandle->SetInstanceAttribute(*GetHulaHoopMesh()->format.GetAttribute("color"), { 1, 1, 1, 0.2 });

				glm::dvec3 priorDir = state->cursorGripOffset - dir * glm::dot(state->cursorGripOffset, dir);

				glm::dvec3 currentCursorDir = GraphicsEngine::Get().currentCamera->ProjectToWorld(Window::Get().MOUSE_POS, Window::Get().Size());
				
				
				glm::dvec3 currentDir = PlaneRayIntersection(target->Position(), dir, GraphicsEngine::Get().currentCamera->position, currentCursorDir) - target->Position();

				glm::quat dDir = glm::rotation(glm::vec3(glm::normalize(priorDir)), glm::vec3(glm::normalize(currentDir)));
				target->SetRotation(dDir * state->originalRot);
			}
			else {
				rotateHandle->SetInstanceAttribute(*GetHulaHoopMesh()->format.GetAttribute("color"), { dir.x, dir.y, dir.z, 0.2 });
			}
			rotateHandle->SetPosition(target->Position());
			}));
		

		return rotateHandle;
		};

	state->rotX = std::unique_ptr<Gameobject>(rotateAxis({ 1, 0, 0 }));
	state->rotY = std::unique_ptr<Gameobject>(rotateAxis({ 0, 1, 0 }));
	state->rotZ = std::unique_ptr<Gameobject>(rotateAxis({ 0, 0, 1 }));

	state->connections.push_back(Window::Get().inputDown.Connect([state = state.get(), target](Window* w, InputObject input) {

		if (!w->IsMouseLocked() && input.input == InputType::LMB) {
			glm::vec3 rayDir = GraphicsEngine::Get().currentCamera->ProjectToWorld(w->MOUSE_POS, w->Size());
			RaycastParams params;
			params.collisionLayers = 2;
			auto result = Raycast(GraphicsEngine::Get().currentCamera->position, rayDir, params);
			if (result.object) {
				if (result.object == state->posX.get() || result.object == state->posY.get() || result.object == state->posZ.get() ||
					result.object == state->rotX.get() || result.object == state->rotY.get() || result.object == state->rotZ.get()) {
					state->gripped = result.object;
					state->originalPos = target->Position();
					state->originalRot = target->Rotation();
					state->cursorGripOffset = result.hitPos - state->originalPos;
				}
			}
		}

		}));

	state->connections.push_back(Window::Get().inputUp.Connect([state = state.get()](Window* w, InputObject input) {

		if (input.input == InputType::LMB) {
			state->gripped = nullptr;
		}

		}));

	state->connections.push_back(target->onGameobjectDestroyed.Connect(target, [state](Gameobject* t) mutable {
		// when this connection is destroyed, it triggers the deletion of the HandlesState, which could cause a second simultaneous deletion of the connections unless we briefly keep a reference on the stack.
		std::shared_ptr<HandlesState> dontDestroyStateYetPls = state;

		// disconnect everything
		state->connections.clear();

		}));
}

void ReportCollisions(Gameobject* a, Gameobject* b) {
	static std::vector<std::unique_ptr<Gameobject>> objects;
	static auto conn = Mainloop::Get().preRender.Connect([a, b](Mainloop*, float) {
		objects.clear();
		if (auto result = a->TestCollision(b)) {
			//DebugLogInfo("A : ", a->Position(), " & ", a->Rotation());
			a->SetInstanceAttribute(*GetArrowMesh()->format.GetAttribute("color"), { 1, 0, 0, 1 });
			b->SetInstanceAttribute(*GetArrowMesh()->format.GetAttribute("color"), { 0, 0, 1, 1 });

			for (auto& [p1, p2] : result->collisionPoints) {
				glm::dvec3 rp1 = a->Position() + glm::dvec3(a->GetRotSclMatrix() * p1);
				objects.emplace_back(DebugArrow(rp1, -result->collisionNormal, { 1, 1, 0 }));
				objects.emplace_back(DebugPoint(rp1, { 1, 1, 0 }));
				glm::dvec3 rp2 = b->Position() + glm::dvec3(b->GetRotSclMatrix() * p2);
				objects.emplace_back(DebugArrow(rp2, result->collisionNormal, { 1, 0.5, 0 }));
				objects.emplace_back(DebugPoint(rp2, { 1, 0.5, 0 }));
			}
		}
		else {
			a->SetInstanceAttribute(*GetArrowMesh()->format.GetAttribute("color"), { 0.5, 0.3, 0.3, 1 });
			b->SetInstanceAttribute(*GetArrowMesh()->format.GetAttribute("color"), { 0.3, 0.3, 0.5, 1 });
		}
		});
}

#include "body.hpp"
#include "gameobject.hpp"
#include "scene.hpp"
#include "shader_program.hpp"
#include "mesh.hpp"
#include <mainloop.hpp>
#include <hierachy_component.hpp>
#include <debug_prefabs.hpp>
#include <window.hpp>

static std::unique_ptr<Scene>& GetHumanoidMesh() {
	LoadSceneParams sceneParams;
	sceneParams.collapseSceneHierarchy = false;
	sceneParams.filepath = "../models/test_anims.fbx";
	static auto scene = Scene::LoadScene(sceneParams);
	return scene;
}

static std::shared_ptr<DrawPass> GetHumanoidDrawPass() {
	auto drawpass = DrawPass::FromTemplate(*GraphicsEngine::Get().defaultDrawingPasses[0]);
	drawpass->name += "_ANIMATED";
	std::get<FramebufferRenderTargetDescriptor>(drawpass->renderTarget).colorAttachments[0].loadPolicy = AttachmentLoadPolicy::Load;
	std::get<FramebufferRenderTargetDescriptor>(drawpass->renderTarget).depthStencilAttachment->loadPolicy = AttachmentLoadPolicy::Load;
	drawpass->outputs.clear();
	drawpass->outputs.push_back("FINAL_SCENE");
	drawpass->outputs.push_back("FINAL_SCENE_DEPTH");
	drawpass->dependencies.push_back("INITIAL_CLEAR");
	drawpass->params.shader = ShaderProgram::New("../shaders/world_vertex_animation.glsl", "../shaders/world_fragment.glsl");
	return drawpass;
}

Body::Body(std::unique_ptr<BodyController> c, BodyCreateParams bodyParams):
	controller(std::move(c))
{
	controller->body = this;

	conns.push_back(Mainloop::Get().preRender.Connect([this](Mainloop*, float dt) { 
		controller->Update(dt); 
		}));

	conns.push_back(Mainloop::Get().prePhysics.Connect([this](Mainloop*, float dt) { controller->FixedUpdate(dt); }));

	auto& scene = GetHumanoidMesh();
	std::shared_ptr<Mesh> mainBody = scene->meshes[0];

	{
		PhysobjectCreateParams physParams;
		physParams.mesh = nullptr;
		physParams.physicsMesh = CapsulePhysicsGeometry::New(bodyParams.height / bodyParams.radius - bodyParams.radius, glm::mat3x3(INFINITY));
		physParams.renderPasses = {};
		collider = std::unique_ptr<Physobject>(Physobject::New(physParams));
		collider->elasticity = 0.0f;
		collider->friction = 4.0f;
		glm::vec3 scl { bodyParams.radius * 2.0f, bodyParams.height, bodyParams.radius * 2.0f };
		collider->AddComponent<Hierarchy>(true)->Transform(glm::dvec3(0, 0, 0), glm::quat(1, 0, 0, 0), scl);


	}

	{
		GameobjectCreateParams renderParams;
		renderParams.mesh = mainBody;
		static auto pass = GetHumanoidDrawPass();
		renderParams.renderPasses = { pass, };

		std::unique_ptr<Gameobject> obj(Gameobject::New(renderParams));
		obj->SetScale(mainBody->OriginalSize());
		obj->SetRotation(glm::angleAxis(glm::radians(180.0f), glm::vec3(0, 1, 0)));
		obj->SetInstanceAttribute(*mainBody->format.GetAttribute("color"), { 1, 1, 1, 1 });
		obj->SetInstanceAttribute(*mainBody->format.GetAttribute(SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION), -1.0f);
		collider->GetComponent<Hierarchy>()->AddChild(std::move(obj));

		for (auto& b : mainBody->bones) {
			//DebugLogInfo("BONE ", b.name);
		}
	}

	

	/*IKBone hip;
	hip.parent = nullptr;
	hip.baseDirection = { 0, 1, 0 };
	hip.currentPosition = { 0, 0, 0 };
	hip.currentRotation = glm::identity<glm::quat>();
	hip.boneIndex = mainBody->GetBone("mixamorig:Hips")->index;*/



}

void Body::SetPos(glm::dvec3 pos) {
	collider->GetComponent<Hierarchy>()->Transform(pos, collider->Rotation(), collider->Scale());
}

LocalPlayerController::LocalPlayerController() {
	camera = std::make_shared<Camera>();
	lookPitch = 0;
	lookYaw = 0;
}

void LocalPlayerController::Update(float dt) {
	if (GraphicsEngine::Get().currentCamera == camera) {
		if (Window::Get().IsMouseLocked()) {
			lookPitch += 0.01f * Window::Get().MOUSE_DELTA.y;
			lookYaw += 0.01f * Window::Get().MOUSE_DELTA.x;
			lookPitch = std::clamp(lookPitch, -glm::radians(89.0f), glm::radians(89.0f));
			if (lookYaw < 0.0f) lookYaw += glm::radians(360.0f);
			lookYaw = std::fmod(lookYaw, glm::radians(360.0f));

			glm::vec3 forward = LookVector(0, lookYaw);
			glm::vec3 right = LookVector(0, lookYaw + glm::radians(90.0f));
			float forwardVel = glm::dot(body->collider->velocity, forward);
			float rightVel = glm::dot(body->collider->velocity, right);
			if (Window::Get().PRESSED_KEYS.contains(InputType::W)) {
				body->collider->velocity += forward * (5.0f - forwardVel);
			}
			else if (Window::Get().PRESSED_KEYS.contains(InputType::S)) {
				body->collider->velocity += forward * (-5.0f - forwardVel);
			}

			if (Window::Get().PRESSED_KEYS.contains(InputType::D)) {
				body->collider->velocity += right * (5.0f - rightVel);
			}
			else if (Window::Get().PRESSED_KEYS.contains(InputType::A)) {
				body->collider->velocity += right * (-5.0f - rightVel);
			}
		}
		body->collider->GetComponent<Hierarchy>()->Transform(glm::quatLookAt(glm::vec3(LookVector(0.0f, lookYaw)), glm::vec3(0, 1, 0)));

		camera->position = body->collider->Position() + glm::dvec3(0, 0.8, 0);
		//DebugPoint(camera->position);
		//DebugArrow(camera->position, body->collider->Rotation() * glm::vec3{ 0, 0, -1 }, { 1, 0, 0 });
		camera->rotation = glm::rotate(glm::rotate(glm::identity<glm::mat4x4>(), lookPitch, glm::vec3(1, 0, 0)), lookYaw, glm::vec3(0, 1, 0));
	}
}

void LocalPlayerController::FixedUpdate(float dt)
{

}

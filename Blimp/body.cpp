#include "body.hpp"
#include "gameobject.hpp"
#include "scene.hpp"
#include "shader_program.hpp"
#include "mesh.hpp"
#include <mainloop.hpp>

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
		SetPos(collider->Pos());
		controller->Update(dt); 
		}));
	conns.push_back(Mainloop::Get().prePhysics.Connect([this](Mainloop*, float dt) { controller->FixedUpdate(dt); }));

	auto& scene = GetHumanoidMesh();
	std::shared_ptr<Mesh> mainBody = scene->meshes[0];

	{
		GameobjectCreateParams renderParams;
		renderParams.mesh = mainBody;
		static auto pass = GetHumanoidDrawPass();
		renderParams.renderPasses = { pass, };

		std::unique_ptr<Gameobject> obj(Gameobject::New(renderParams));
		obj->SetScale(mainBody->OriginalSize());
		obj->SetInstanceAttribute(*mainBody->format.GetAttribute("color"), { 1, 1, 1, 1 });
		obj->SetInstanceAttribute(*mainBody->format.GetAttribute(SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION), -1.0f);
		gameobjects.push_back(std::move(obj));

		for (auto& b : mainBody->bones) {
			//DebugLogInfo("BONE ", b.name);
		}
	}

	{
		PhysobjectCreateParams physParams;
		physParams.mesh = nullptr;
		physParams.physicsMesh = CapsulePhysicsGeometry::New(bodyParams.height / bodyParams.radius - bodyParams.radius);
		physParams.renderPasses = {};
		collider = Physobject::New(physParams);
		auto obj = std::unique_ptr<Physobject>(collider);
		obj->SetScale({bodyParams.radius * 2.0f, bodyParams.height, bodyParams.radius * 2.0f});
		gameobjects.push_back(std::move(obj));
	}

	/*IKBone hip;
	hip.parent = nullptr;
	hip.baseDirection = { 0, 1, 0 };
	hip.currentPosition = { 0, 0, 0 };
	hip.currentRotation = glm::identity<glm::quat>();
	hip.boneIndex = mainBody->GetBone("mixamorig:Hips")->index;*/



}

void Body::SetPos(glm::dvec3 pos) {
	collider->SetPosition(pos);
}

LocalPlayerController::LocalPlayerController() {
	camera = std::make_shared<Camera>();
}

void LocalPlayerController::Update(float dt) {
	camera->position = body->gameobjects[0]->Position() + glm::dvec3(0, 1, 0);
	camera->rotation = body->gameobjects[0]->Rotation();
}

void LocalPlayerController::FixedUpdate(float dt)
{

}

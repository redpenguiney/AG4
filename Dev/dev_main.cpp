#include "gameobject.hpp"
#include "debug_prefabs.hpp"
#include "gui.hpp"
#include "mainloop.hpp"
#include "window.hpp"
#include "utility.hpp"
#include "clustered_lighting.hpp"
#include "game.hpp"
#include "scene.hpp"
#include "shader_program.hpp"
#include "mesh.hpp"
#include <debug_editing_tools.hpp>

std::shared_ptr<GuiElement> frame;

class GameState {
public:
	std::vector<std::unique_ptr<Gameobject>> objs;
	std::vector<std::shared_ptr<GuiElement>> uiElements;
	std::unique_ptr<Scene> scene;
};

Game::Game(std::vector<const char*> launchArgs) {
	state = std::make_unique<GameState>();

	GameobjectCreateParams p;
	p.mesh = GetCubeMesh();
	p.physicsMesh = GetCubeCollisions();

	state->objs.emplace_back(DebugPoint({ 0, 0, 0 }, { 1, 1, 1 }));
	//state->objs.emplace_back(DebugPoint({ 0.4, 0, 0 }, { 1, 0, 0 }));
	//state->objs.emplace_back(DebugPoint({ 0, 0.4, 0 }, { 0, 1, 0 }));
	//state->objs.emplace_back(DebugPoint({ 0, 0, 0.4 }, { 0, 0, 1 }));
	state->objs.emplace_back(DebugArrow({ 0, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }));
	state->objs.emplace_back(DebugArrow({ 0, 0, 0 }, { 0, 1, 0 }, { 0, 1, 0 }));
	state->objs.emplace_back(DebugArrow({ 0, 0, 0 }, { 0, 0, 1 }, { 0, 0, 1 }));

	/*{
		LoadSceneParams sceneParams;
		sceneParams.collapseSceneHierarchy = false;
		sceneParams.filepath = "../models/test_anims.fbx";
		state->scene = Scene::LoadScene(sceneParams);

		for (auto& obj : state->scene->DebugPresent({2, 0, 0})) {
			state->objs.push_back(std::unique_ptr<Gameobject>(obj));
		}

		state->scene = nullptr;
	}*/

	{
		LoadSceneParams sceneParams;
		sceneParams.collapseSceneHierarchy = false;
		sceneParams.filepath = "../models/test_anims.fbx";
		state->scene = Scene::LoadScene(sceneParams);

		auto rig = state->scene->meshes[0];
		auto gparams = GameobjectCreateParams();
		gparams.mesh = rig;
		auto drawpass = DrawPass::FromTemplate(*GraphicsEngine::Get().defaultDrawingPasses[0]);
		drawpass->name += "_ANIMATED";
		std::get<FramebufferRenderTargetDescriptor>(drawpass->renderTarget).colorAttachments[0].loadPolicy = AttachmentLoadPolicy::Load;
		std::get<FramebufferRenderTargetDescriptor>(drawpass->renderTarget).depthStencilAttachment->loadPolicy = AttachmentLoadPolicy::Load;
		drawpass->outputs.clear();
		drawpass->outputs.push_back("FINAL_SCENE");
		drawpass->outputs.push_back("FINAL_SCENE_DEPTH");
		drawpass->dependencies.push_back("INITIAL_CLEAR");
		drawpass->params.shader = ShaderProgram::New("../shaders/world_vertex_animation.glsl", "../shaders/world_fragment.glsl");
		gparams.renderPasses = { drawpass, };
		auto obj = Gameobject::New(gparams);
		obj->SetScale(gparams.mesh->OriginalSize());
		obj->SetPosition({ 3, 0, 0 });
		obj->SetInstanceAttribute(*gparams.mesh->format.GetAttribute("color"), glm::vec4(1, 1, 1, 1));
		obj->SetInstanceAttribute(*gparams.mesh->format.GetAttribute(SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION), -1.0f);
		state->objs.emplace_back(obj);

		obj->SetBoneTransform(5, glm::translate(glm::identity<glm::mat4x4>(), glm::vec3(0, 0.1, 0)));

		state->scene = nullptr;
	}

	GraphicsEngine::Get().currentCamera = GetFreecam();

	/*for (auto& o : BuildPit({ 1, 0, 0 }, { 80, 6, 80 }, 0.0, 0)) {
		state->objs.emplace_back(o);
	}
	auto objs = BuildCubeArray({ 1, -2, 0 }, { 2, 1, 2 }, { 1, 5, 1 }, true, 0.0, 1.0);
	for (auto& o : objs) {
		state->objs.emplace_back(o);
	}*/

	auto testCollisionSubject = BuildCubeArray({ 1, -2, 0 }, { 2, 2, 2 }, { 1, 1, 1}, true, 0.0, 1.0);
	//testCollisionSubject[0]->SetPosition({ 1.003f, -2.002, 1.2f });
	//testCollisionSubject[0]->SetRotation({ -0.195, 0.328, -0.069, 0.922 });
	//testCollisionSubject[0]->SetRotation(glm::angleAxis(glm::radians(180.0f), glm::vec3{0, 1, 0}));
	testCollisionSubject.push_back(BuildSphere({ -2, 0, 0 }, 1.0f, false));
	//testCollisionSubject.push_back(BuildCapsule({ -2, 0, 0 }, 1.0f, 2.0f, false));
	//std::vector<Gameobject*> testCollisionSubject = { BuildSphere({-2, 0, 0}, 1.0f, false), BuildSphere({2, 0, 0}, 1.0f, false)};
	for (auto& o : testCollisionSubject) {
		state->objs.emplace_back(o);
		TransformHandles(o);
	}
	ReportCollisions(testCollisionSubject[0], testCollisionSubject[1]);

	/*for (auto& o : BuildChain({ 1, 0, 0 }, 1.0f, 15, 0.0001f)) {
		state->objs.emplace_back(o);
	}*/

	//TransformHandles(state->objs.back().get());
	//PhysicsEngine::Get()

	//for (int i = 0; i < 1; i++) {
	//	Physobject* gameObj = Physobject::New(p);
	//	gameObj->friction = 1.0f;
	//	gameObj->elasticity = 0.0f;d
	//	gameObj->SetPosition({ i + 2.5, -1.5f, 0 - i + 3 });
	//	gameObj->SetScale({ 1, 1, 1 });
	//	gameObj->SetRotation(glm::angleAxis(glm::radians(100.0f), glm::normalize(glm::vec3(1, 0, 0))));
	//	//gameObj->velocity = { 5, -5, 0 };
	//	//gameObj->rotVelocity = { 0, 2.0f, 0 };
	//	glm::vec4 color(1, 0.7, 1, 1);
	//	gameObj->SetInstanceAttribute(*p.mesh->format.GetAttribute("color"), color);
	//	gameObj->SetInstanceAttribute(*p.mesh->format.GetAttribute(SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION), -1.0f);

	//	std::shared_ptr<Gameobject> unique(gameObj);
	//	objects.push_back(unique);

	//}		

	auto& CL = ClusteredLighting::Get();
	auto pointLight = std::make_shared<PointLight>();
	pointLight->position = { 2.0, 2.0, 2.0 };
	pointLight->intensity = 4.0f;
	pointLight->color = { 1, 0, 0 };
	auto spotLight = std::make_shared<SpotLight>();
	spotLight->position = { 3, 0, 20 };
	spotLight->direction = glm::normalize(glm::vec3{ 0.0f, 0.0f, -1.0f });
	spotLight->innerAngle = glm::radians(3.0f);
	spotLight->outerAngle = glm::radians(6.0f);
	spotLight->intensity = 80.0f;
	spotLight->color = glm::vec3(0, 1, 1);
	CL.lights.push_back(pointLight);
	CL.lights.push_back(spotLight);

	/*TextureCreateParams arialFontParams({ TextureSource("../fonts/arial.ttf"), });
	arialFontParams.fontHeight = 24;
	arialFontParams.format = Texture::Grayscale_8Bit;
	auto arialFont = std::make_shared<Texture>(arialFontParams, Texture::Texture2D);

	frame = GuiElement::New(*GetScreenGuiContainer(), nullptr, arialFont);
	frame->percentagePosition = { 0.5, 0.5 };
	frame->pixelSize = { 300, 150 };
	frame->backgroundColor = { 1, 1, 1, 0.0 };
	frame->text = "Honey.";
	frame->textColor = { 0, 1, 0, 1.0 };
	frame->vAlign = VerticalAlignMode::Center;
	frame->hAlign = HorizontalAlignMode::Center;
	frame->RefreshGraphics();
	frame->RefreshTransform();
	frame->RefreshText();
	state->uiElements.push_back(frame);*/

	GraphicsEngine::Get().currentCamera->position = { 1, 0, 5 };
	////PhysicsEngine::Get().gravity = { 5, -5, 0 };


	////Mainloop::Get().stepPhysics = true;
	//Mainloop::Get().physicsPaused = true;
	static auto c = Window::Get().inputUp.Connect([this](Window*, InputObject input) {
		if (input.input == InputType::Space) Mainloop::Get().physicsPaused = !Mainloop::Get().physicsPaused;

		if (input.input == InputType::T) {
			if (state->objs.size() > 0) state->objs.pop_back();
		}
		else if (input.input == InputType::Y) {
			state->objs.emplace_back(DebugPoint({ 0, 0, state->objs.size() }, { 1, 1, 1 }));
		}
		if (input.input == InputType::LMB) {
			auto result = Raycast(GraphicsEngine::Get().currentCamera->position, LookVector(freecamPitch, freecamYaw), RaycastParams());
			if (result.object) {
				//DebugLogInfo("Result ", result.distance, " ", result.hitNormal, " ", result.hitPos, " ", result.object);
				if (auto phys = dynamic_cast<Physobject*>(result.object)) {
					phys->velocity += result.hitNormal;
				}
			}
			else {
				//DebugLogInfo("Result missed.");
			}
		}
	});
}

Game::~Game() {
	frame = nullptr;
}
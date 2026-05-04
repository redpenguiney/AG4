#include "gameobject.hpp"
#include "debug_prefabs.hpp"
#include "gui.hpp"
#include "mainloop.hpp"
#include "window.hpp"
#include "utility.hpp"
#include "clustered_lighting.hpp"
#include "game.hpp"

std::shared_ptr<GuiElement> frame;

class GameState {
public:
	std::vector<std::unique_ptr<Gameobject>> objs;
};

Game::Game() {
	state = std::make_unique<GameState>();

	GameobjectCreateParams p;
	p.mesh = GetCubeMesh();
	p.physicsMesh = GetCubeCollisions();

	state->objs.emplace_back(DebugPoint({ 0, 0, 0 }, { 1, 1, 1 }));
	state->objs.emplace_back(DebugPoint({ 0.4, 0, 0 }, { 1, 0, 0 }));
	state->objs.emplace_back(DebugPoint({ 0, 0.4, 0 }, { 0, 1, 0 }));
	state->objs.emplace_back(DebugPoint({ 0, 0, 0.4 }, { 0, 0, 1 }));

	Freecam();
	/*{
		Gameobject* gameObj = Gameobject::New(p);
		gameObj->SetPosition({ 0, -3, 0 });
		gameObj->SetScale({ 1, 1, 1 });
		gameObj->SetRotation(glm::angleAxis(glm::radians(45.0f), glm::normalize(glm::vec3(0, 0, 1))));
		glm::vec4 color(0, 0, 1, 1);
		gameObj->SetInstanceAttribute(*p.mesh->format.GetAttribute("color"), color);
		gameObj->SetInstanceAttribute(*p.mesh->format.GetAttribute(SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION), -1.0f);

		std::shared_ptr<Gameobject> unique(gameObj);
		objects.push_back(unique);
	}*/

	//BuildPit({ 1, 0, 0 }, { 80, 6, 80 }, 0.0, 0);
	auto objs = BuildCubeArray({ 1, -2, 0 }, { 2, 1, 2 }, { 1, 5, 1 }, true, 0.0, 1.0);
	for (auto& o : objs) {
		state->objs.emplace_back(o);
	}
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
	pointLight->intensity = 400.0f;
	pointLight->color = { 1, 0, 0 };
	auto spotLight = std::make_shared<SpotLight>();
	spotLight->position = { 3, 0, 20 };
	spotLight->direction = glm::normalize(glm::vec3{ 0.0f, 0.0f, -1.0f });
	spotLight->innerAngle = glm::radians(3.0f);
	spotLight->outerAngle = glm::radians(6.0f);
	spotLight->intensity = 800.0f;
	spotLight->color = glm::vec3(0, 1, 1);
	//CL.lights.push_back(pointLight);
	//CL.lights.push_back(spotLight);

	//TextureCreateParams arialFontParams({ TextureSource("../fonts/arial.ttf"), });
	//arialFontParams.fontHeight = 8;
	//arialFontParams.format = Texture::Grayscale_8Bit;
	//auto arialFont = std::make_shared<Texture>(arialFontParams, Texture::Texture2D);

	//frame = GuiElement::New(*GetScreenGuiContainer(), nullptr, arialFont);
	//frame->percentagePosition = { 0.5, 0.5 };
	//frame->pixelSize = { 300, 150 };
	//frame->backgroundColor = { 1, 1, 1, 0.0 };
	//frame->text = "Honey is a free browser addon available on AAAAAAGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAGoogle, Oprah, Firefox, Safari - if it's a browser. It has Honey.";
	//frame->textColor = { 0, 1, 0, 1.0 };
	//frame->vAlign = VerticalAlignMode::Center;
	//frame->hAlign = HorizontalAlignMode::Center;
	//frame->RefreshGraphics();
	//frame->RefreshTransform();
	//frame->RefreshText();

	GraphicsEngine::Get().currentCamera.position = { 1, 0, 5 };
	////PhysicsEngine::Get().gravity = { 5, -5, 0 };


	////Mainloop::Get().stepPhysics = true;
	//Mainloop::Get().physicsPaused = true;
	static auto c = Window::Get().inputUp.Connect([this](Window*, InputObject input) {
		if (input.input == InputObject::Space) Mainloop::Get().physicsPaused = !Mainloop::Get().physicsPaused;

		if (input.input == InputObject::T) {
			if (state->objs.size() > 0) state->objs.pop_back();
		}
		else if (input.input == InputObject::Y) {
			state->objs.emplace_back(DebugPoint({ 0, 0, state->objs.size() }, { 1, 1, 1 }));
		}
		if (input.input == InputObject::LMB) {
			auto result = Raycast(GraphicsEngine::Get().currentCamera.position, LookVector(freecamPitch, freecamYaw), RaycastParams());
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
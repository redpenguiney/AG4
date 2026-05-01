#include "gameobject.hpp"
#include "mainloop.hpp"
#include "mesh_provider.hpp"
#include "mesh.hpp"
#include "graphics_engine.hpp"
#include "window.hpp"
#include "utility.hpp"
#include <algorithm>
#include "shader_program.hpp"
#include "aabb_tree.hpp"
#include "debug_prefabs.hpp"
#include "light.hpp"
#include "clustered_lighting.hpp"
#include "physics_engine.hpp"
#include "gui.hpp"
#include "game.hpp"

int main() {
	DebugLogInfo("Reached main() successfully."); // you know it's a bad sign when you need to print this sort of thing

	std::vector<std::shared_ptr<Gameobject>> objects;
	
	//MemoryPool<Gameobject, const GameobjectCreateParams&>::Get();
	//MemoryPool<Physobject, const PhysobjectCreateParams&>::Get();
	Window::Get();
	GraphicsEngine::Get();
	GameobjectSAS();
	//GetScreenGuiContainer();
	GuiElement::InitGuiEvents();

	{
		auto& CL = ClusteredLighting::Get();
		auto sun = std::make_shared<EnvironmentalLight>();
		sun->color = glm::vec3(0.9922, 0.9843, 0.8275);
		sun->direction = glm::normalize(glm::vec3(-1.0f, 3.0f, -1.0f));
		sun->intensity = 10.0f;
		CL.lights.push_back(sun);

		// normal objects
		auto newPass = std::make_shared<DrawPass>();
		newPass->name = "default";
		auto frt = FramebufferRenderTargetDescriptor();
		frt.colorAttachments.push_back(FramebufferAttachmentUsageDescriptor{
			.attachmentName = "FINAL_SCENE",
			.loadPolicy = AttachmentLoadPolicy::Clear,
			//.clearColor = {0, 0, 0, 1}
			});
		frt.depthStencilAttachment.emplace(FramebufferAttachmentUsageDescriptor{
			.attachmentName = "FINAL_SCENE_DEPTH",
			.loadPolicy = AttachmentLoadPolicy::Clear,
			.clearColor = {1, 0, 0, 0}
			});
		newPass->dependencies.push_back("lights");
		newPass->renderTarget = frt;
		newPass->outputs.push_back("FINAL_SCENE");
		newPass->outputs.push_back("FINAL_SCENE_DEPTH");
		newPass->params.depthTestMode = DepthTestMode::LEqual;
		newPass->params.cullMode = FaceCulling::Backface;
		newPass->params.shader = ShaderProgram::New("../shaders/world_vertex.glsl", "../shaders/world_fragment.glsl");
		newPass->params.shader->Uniform("vertexColorEnabled", true, false);
		GraphicsEngine::Get().defaultDrawingPasses.push_back(newPass);

		GraphicsEngine::Get().AddAttachment(FramebufferAttachmentFormatDescriptor{
			.resourceName = "FINAL_SCENE",
			.format = Texture::RGBA_16Float,
			.size = {1024, 1024},
			});
		GraphicsEngine::Get().AddAttachment(FramebufferAttachmentFormatDescriptor{
			.resourceName = "FINAL_SCENE_DEPTH",
			.renderbuffer = false,
			.format = Texture::DEPTH24_STENCIL8,
			.size = {1024, 1024},
			});

		GraphicsEngine::Get().ForceAddDrawPass(newPass);

		// postproc
		auto postprocPass = std::make_shared<DrawPass>();
		postprocPass->name = "postproc";
		auto rt = WindowRenderTargetDescriptor();
		rt.loadPolicy = AttachmentLoadPolicy::DontCare;
		postprocPass->renderTarget = rt;
		postprocPass->dependencies.push_back("FINAL_SCENE");
		postprocPass->boundTextures.push_back(TextureUsageDescriptor{
			.texture = "FINAL_SCENE",
			.textureUsageLocation = "screenTextureColor"
		});
		postprocPass->outputs.push_back(WINDOW_RESOURCE_NAME);
		postprocPass->outputs.push_back("POST_PROC");
		postprocPass->params.depthTestMode = DepthTestMode::Disabled;
		postprocPass->params.cullMode = FaceCulling::Backface;
		postprocPass->params.shader = ShaderProgram::New("../shaders/postproc_vertex.glsl", "../shaders/postproc_fragment.glsl");
		GameobjectCreateParams quadParams;
		quadParams.mesh = Mesh::Quad();
		quadParams.renderPasses = { postprocPass, };
		std::shared_ptr<Gameobject> postProcQuad(Gameobject::New(quadParams));
		objects.push_back(postProcQuad);
	}

	//Mainloop::Get().stepPhysics = true;
	Mainloop::Get().physicsPaused = true;
	Window::Get().inputUp.Connect([](auto, InputObject input) {
		if (input.input == InputObject::Tab) {
			Window::Get().SetMouseLocked(!Window::Get().IsMouseLocked());
		}
		});

	{
		Game g;
		Mainloop::Get().Run();
		// Game::~Game() runs here.
	}
	// TODO cleanup?

	DebugLogInfo("Main function body executed successfully.");
}
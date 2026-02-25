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

int main() {
	std::vector<std::shared_ptr<Gameobject>> objects;

	Window::Get();
	GraphicsEngine::Get();
	GameobjectSAS();

	{
		
	

		// normal objects
		auto newPass = std::make_shared<DrawPass>();
		newPass->name = "default";
		auto frt = FramebufferRenderTargetDescriptor();
		frt.colorAttachments.push_back(FramebufferAttachmentUsageDescriptor{
			.attachmentName = "FINAL_SCENE",
			.loadPolicy = AttachmentLoadPolicy::Clear,
			
			});
		frt.depthStencilAttachment.emplace(FramebufferAttachmentUsageDescriptor{
			.attachmentName = "FINAL_SCENE_DEPTH",
			.loadPolicy = AttachmentLoadPolicy::Clear,
			.clearColor = {1, 0, 0, 0}
			});
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

		// postproc
		auto postprocPass = std::make_shared<DrawPass>();
		postprocPass->name = "postproc";
		auto rt = WindowRenderTargetDescriptor();
		rt.loadPolicy = AttachmentLoadPolicy::DontCare;
		postprocPass->renderTarget = rt;
		postprocPass->dependencies.push_back("FINAL_SCENE");
		postprocPass->boundAttachments.push_back(TextureUsageDescriptor{
			.texture = "FINAL_SCENE",
			.textureUsageLocation = "screenTextureColor"
		});
		postprocPass->outputs.push_back(WINDOW_RESOURCE_NAME);
		postprocPass->params.depthTestMode = DepthTestMode::Disabled;
		postprocPass->params.cullMode = FaceCulling::Backface;
		postprocPass->params.shader = ShaderProgram::New("../shaders/postproc_vertex.glsl", "../shaders/postproc_fragment.glsl");
		GameobjectCreateParams quadParams;
		quadParams.mesh = Mesh::Quad();
		quadParams.renderPasses = { postprocPass, };
		std::shared_ptr<Gameobject> postProcQuad(Gameobject::New(quadParams));
		objects.push_back(postProcQuad);
	}
	

	DebugLogInfo("Main function reached successfully.");
	auto mparams = MeshCreateParams::Default();
	mparams.LoadObj("../models/rainbowcube.obj");

	auto squareMesh = Mesh::New(std::move(mparams));

	GameobjectCreateParams p;
	p.mesh = squareMesh;

	{
		Gameobject* gameObj = Gameobject::New(p);
		gameObj->SetPosition({ 0, -5, 0 });
		gameObj->SetScale({ 10, 1, 10 });
		glm::vec4 color(0, 1, 0, 1);
		gameObj->SetInstanceAttribute(*squareMesh->format.GetAttribute("color"), color);
		gameObj->SetInstanceAttribute(*squareMesh->format.GetAttribute(SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION), -1.0f);

		std::shared_ptr<Gameobject> unique(gameObj);
		objects.push_back(unique);
	}

	for (int i = 0; i < 5; i++) {
		Physobject* gameObj = Physobject::New(p);
		gameObj->SetPosition({ i, 0, -3 - i });
		gameObj->SetScale({ 0.8, 0.8, 0.8 });
		glm::vec4 color(1, 0, 1, 1);
		gameObj->SetInstanceAttribute(*squareMesh->format.GetAttribute("color"), color);
		gameObj->SetInstanceAttribute(*squareMesh->format.GetAttribute(SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION), -1.0f);

		std::shared_ptr<Gameobject> unique(gameObj);
		objects.push_back(unique);

	}
	
	//GraphicsEngine::Get().currentCamera.far

	// Freecam
	float pitch = 0, yaw = 0, speed = 0;
	Mainloop::Get().preRender->Connect([&pitch, &yaw, &speed](float) {
		pitch += 0.01f * Window::Get().MOUSE_DELTA.y;
		yaw += 0.01f * Window::Get().MOUSE_DELTA.x;
		pitch = std::clamp(pitch, -glm::radians(89.0f), glm::radians(89.0f));
		if (yaw < 0.0f) yaw += glm::radians(360.0f);
		yaw = std::fmod(yaw,glm::radians(360.0f));

		auto& cam = GraphicsEngine::Get().currentCamera;
		float forward = (Window::Get().PRESSED_KEYS.contains(InputObject::W) ? 1.0f : 0.0f) - (Window::Get().PRESSED_KEYS.contains(InputObject::S) ? 1.0f : 0.0f);
		float right = (Window::Get().PRESSED_KEYS.contains(InputObject::D) ? 1.0f : 0.0f) - (Window::Get().PRESSED_KEYS.contains(InputObject::A) ? 1.0f : 0.0f);
		float up = (Window::Get().PRESSED_KEYS.contains(InputObject::Q) ? 1.0f : 0.0f) - (Window::Get().PRESSED_KEYS.contains(InputObject::E) ? 1.0f : 0.0f);

		if (forward == 0 && right == 0 && up == 0) speed = 0;
		speed += 0.1;
		cam.rotation = glm::rotate(glm::rotate(glm::identity<glm::mat4x4>(), pitch, glm::vec3(1, 0, 0)), yaw, glm::vec3(0, 1, 0));
		glm::vec3 fDir = LookVector(pitch, yaw);
		glm::vec3 rDir= LookVector(0, yaw + glm::radians(90.0f));
		glm::vec3 upDir = glm::cross(fDir, rDir);
		cam.position += (fDir * forward + rDir * right + upDir * up) * speed;
		});

	Mainloop::Get().physicsPaused = true;
	Window::Get().inputUp->Connect([](InputObject input) {
		if (input.input == InputObject::Space) Mainloop::Get().physicsPaused = !Mainloop::Get().physicsPaused;
		});
	Mainloop::Get().Run();
	// TODO cleanup?

	DebugLogInfo("Main function body executed successfully.");
}
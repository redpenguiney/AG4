#include "body.hpp"
#include "gameobject.hpp"
#include "scene.hpp"
#include "shader_program.hpp"

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

Body::Body() {
	{
		auto& scene = GetHumanoidMesh();
		std::shared_ptr<Mesh> mainBody = scene->meshes[0];

		PhysobjectCreateParams params;
		params.mesh = mainBody;
		static auto pass = GetHumanoidDrawPass();
		params.renderPasses = { pass, };
		
		gameobjects.push_back(std::unique_ptr<Physobject>(Physobject::New(params)));
	}

}

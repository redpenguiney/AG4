#include "debug_prefabs.hpp"
#include "mesh_provider.hpp"
#include "mesh.hpp"
#include "rendergraph_node.hpp"
#include "shader_program.hpp"
#include "physics_mesh.hpp"
#include "gameobject.hpp"

static std::shared_ptr<Mesh> CubeMesh() {
	auto mparams = MeshCreateParams::Default();
	mparams.LoadObj("../models/rainbowcube.obj");
	auto cubeMesh = Mesh::New(std::move(mparams));
	return cubeMesh;
}

std::shared_ptr<Mesh> GetCubeMesh()
{
	static auto cubeMesh = CubeMesh();
	return cubeMesh;
}

static std::shared_ptr<DrawPass> DebugWireframePass() {
	auto pass = std::make_shared<DrawPass>();
	pass->name = "debugWireframe";
	auto rt = WindowRenderTargetDescriptor();
	rt.clearDepth = false;
	rt.loadPolicy = AttachmentLoadPolicy::Load;
	pass->renderTarget = rt;
	pass->dependencies.push_back("POST_PROC");
	pass->outputs.push_back(WINDOW_RESOURCE_NAME);
	pass->params.depthTestMode = DepthTestMode::Disabled;
	pass->params.cullMode = FaceCulling::None;
	pass->params.polygonFillMode = PolygonFillMode::Lines;
	pass->params.shader = ShaderProgram::New("../shaders/debug_simple_vertex.glsl", "../shaders/debug_simple_fragment.glsl");
	return pass;
}

static std::shared_ptr<DrawPass> DebugSolidPass() {
	auto pass = std::make_shared<DrawPass>();
	pass->name = "debugWireframe";
	auto rt = WindowRenderTargetDescriptor();
	rt.clearDepth = false;
	rt.loadPolicy = AttachmentLoadPolicy::Load;
	pass->renderTarget = rt;
	pass->dependencies.push_back("POST_PROC");
	pass->outputs.push_back(WINDOW_RESOURCE_NAME);
	pass->params.depthTestMode = DepthTestMode::Disabled;
	pass->params.cullMode = FaceCulling::None;
	pass->params.polygonFillMode = PolygonFillMode::Fill;
	pass->params.shader = ShaderProgram::New("../shaders/debug_simple_vertex.glsl", "../shaders/debug_simple_fragment.glsl");
	return pass;
}

std::shared_ptr<DrawPass> GetDebugWireframePass() {
	static auto pass = DebugWireframePass();
	return pass;
}

std::shared_ptr<DrawPass> GetDebugSolidPass() {
	static auto pass = DebugSolidPass();
	return pass;
}

std::shared_ptr<ConvexMeshPhysicsGeometry> GetCubeCollisions() {
	static auto collisions = ConvexMeshPhysicsGeometry::FromMesh(GetCubeMesh());
	return collisions;
}

Gameobject* DebugPoint(glm::dvec3 pos, glm::vec3 color) {
	GameobjectCreateParams p;
	p.mesh = GetCubeMesh();
	p.physicsMesh = nullptr;
	p.renderPasses = { GetDebugSolidPass(), };
	Gameobject* gameObj = Gameobject::New(p);
	gameObj->SetPosition(pos);
	gameObj->SetScale({ 0.1, 0.1, 0.1 });
	gameObj->SetInstanceAttribute(*p.mesh->format.GetAttribute("color"), {color.x, color.y, color.z, 1});
	gameObj->SetInstanceAttribute(*p.mesh->format.GetAttribute(SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION), -1.0f);
	return gameObj;
}

#include "debug_prefabs.hpp"
#include "mesh_provider.hpp"
#include "mesh.hpp"
#include "rendergraph_node.hpp"
#include "shader_program.hpp"
#include "physics_mesh.hpp"

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
	rt.clearDepth = true;
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

std::shared_ptr<DrawPass> GetDebugWireframePass() {
	static auto pass = DebugWireframePass();
	return pass;
}

std::shared_ptr<ConvexMeshPhysicsGeometry> GetCubeCollisions() {
	static auto collisions = ConvexMeshPhysicsGeometry::FromMesh(GetCubeMesh());
	return collisions;
}

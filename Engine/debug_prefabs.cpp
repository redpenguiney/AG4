#include "debug_prefabs.hpp"
#include "mesh_provider.hpp"
#include "mesh.hpp"
#include "rendergraph_node.hpp"
#include "shader_program.hpp"
#include "physics_mesh.hpp"
#include "gameobject.hpp"
#include "mainloop.hpp"
#include "window.hpp"
#include "utility.hpp"

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

static std::shared_ptr<ShaderProgram> GetDebugShader() {
	static auto s = ShaderProgram::New("../shaders/debug_simple_vertex.glsl", "../shaders/debug_simple_fragment.glsl");
	return s;
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
	pass->params.shader = GetDebugShader();
	return pass;
}

static std::shared_ptr<DrawPass> DebugSolidPass() {
	auto pass = std::make_shared<DrawPass>();
	pass->name = "debugSolid";
	auto rt = WindowRenderTargetDescriptor();
	rt.clearDepth = false;
	rt.loadPolicy = AttachmentLoadPolicy::Load;
	pass->renderTarget = rt;
	pass->dependencies.push_back("POST_PROC");
	pass->outputs.push_back(WINDOW_RESOURCE_NAME);
	pass->params.depthTestMode = DepthTestMode::Disabled;
	pass->params.cullMode = FaceCulling::None;
	pass->params.polygonFillMode = PolygonFillMode::Fill;
	pass->params.shader = GetDebugShader();
	return pass;
}

std::shared_ptr<DrawPass> DebugLinesPass() {
	auto pass = std::make_shared<DrawPass>();
	pass->name = "debugLines";
	auto rt = WindowRenderTargetDescriptor();
	rt.clearDepth = false;
	rt.loadPolicy = AttachmentLoadPolicy::Load;
	pass->renderTarget = rt;
	pass->dependencies.push_back("POST_PROC");
	pass->outputs.push_back(WINDOW_RESOURCE_NAME);
	pass->params.depthTestMode = DepthTestMode::Disabled;
	pass->params.cullMode = FaceCulling::None;
	pass->params.shader = GetDebugShader();
	return pass;
}

std::shared_ptr<DrawPass> GetDebugLinesPass() {
	static auto pass = DebugLinesPass();
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

void BuildPit(glm::dvec3 pos, glm::vec3 size, float elasticity, float friction) {
	GameobjectCreateParams p;
	p.mesh = GetCubeMesh();
	p.physicsMesh = GetCubeCollisions();

	glm::vec4 floorColor(0, 1, 0, 1);
	glm::vec4 wallColor(0.7, 0.3, 0.3, 1);

	Gameobject* floor = Gameobject::New(p);
	floor->elasticity = elasticity;
	floor->friction = friction;
	floor->SetPosition({ pos - glm::dvec3(0, size.y/2.0f, 0)});
	floor->SetScale({ size.x+3, 1, size.z+3 });
	floor->SetInstanceAttribute(*p.mesh->format.GetAttribute("color"), floorColor);
	floor->SetInstanceAttribute(*p.mesh->format.GetAttribute(SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION), -1.0f);
	//floor->SetRotation(glm::angleAxis(glm::radians(5.0f), glm::normalize(glm::vec3(1, 1, 0))));

	Gameobject* wall1 = Gameobject::New(p);
	wall1->elasticity = elasticity;
	wall1->friction = friction;
	wall1->SetPosition({ pos - glm::dvec3(size.x / 2.0f, 0, 0) });
	wall1->SetScale({ 1, size.y, size.z });
	wall1->SetInstanceAttribute(*p.mesh->format.GetAttribute("color"), wallColor);
	wall1->SetInstanceAttribute(*p.mesh->format.GetAttribute(SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION), -1.0f);

	Gameobject* wall2 = Gameobject::New(p);
	wall2->elasticity = elasticity;
	wall2->friction = friction;
	wall2->SetPosition({ pos + glm::dvec3(size.x / 2.0f, 0, 0) });
	wall2->SetScale({ 1, size.y, size.z });
	wall2->SetInstanceAttribute(*p.mesh->format.GetAttribute("color"), wallColor);
	wall2->SetInstanceAttribute(*p.mesh->format.GetAttribute(SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION), -1.0f);

}

void BuildCubeArray(glm::dvec3 origin, glm::dvec3 stride, glm::uvec3 nCubes, bool physics, float elasticity, float friction) {
	GameobjectCreateParams p;
	p.mesh = GetCubeMesh();
	p.physicsMesh = GetCubeCollisions();
	
	for (unsigned i = 0; i < nCubes.x; i++) {
		for (unsigned j = 0; j < nCubes.y; j++) {
			for (unsigned k = 0; k < nCubes.z; k++) {
				Physobject* gameObj = Physobject::New(p);
				gameObj->friction = friction;
				gameObj->elasticity = elasticity;
				gameObj->SetPosition(origin + stride * glm::dvec3(i, j, k));
				gameObj->SetScale({ 1, 1, 1 });
				glm::vec4 color(1, 0.7, 1, 1);
				if (rand() < RAND_MAX / 2) color.x *= 0.5;
				gameObj->SetInstanceAttribute(*p.mesh->format.GetAttribute("color"), color);
				gameObj->SetInstanceAttribute(*p.mesh->format.GetAttribute(SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION), -1.0f);
			}
		}
	}
}

void Freecam() {
	// Freecam
	Mainloop::Get().preRender->Connect([](float) {
		freecamPitch += 0.01f * Window::Get().MOUSE_DELTA.y;
		freecamYaw += 0.01f * Window::Get().MOUSE_DELTA.x;
		freecamPitch = std::clamp(freecamPitch, -glm::radians(89.0f), glm::radians(89.0f));
		if (freecamYaw < 0.0f) freecamYaw += glm::radians(360.0f);
		freecamYaw = std::fmod(freecamYaw, glm::radians(360.0f));

		auto& cam = GraphicsEngine::Get().currentCamera;
		float forward = (Window::Get().PRESSED_KEYS.contains(InputObject::W) ? 1.0f : 0.0f) - (Window::Get().PRESSED_KEYS.contains(InputObject::S) ? 1.0f : 0.0f);
		float right = (Window::Get().PRESSED_KEYS.contains(InputObject::D) ? 1.0f : 0.0f) - (Window::Get().PRESSED_KEYS.contains(InputObject::A) ? 1.0f : 0.0f);
		float up = (Window::Get().PRESSED_KEYS.contains(InputObject::Q) ? 1.0f : 0.0f) - (Window::Get().PRESSED_KEYS.contains(InputObject::E) ? 1.0f : 0.0f);

		if (forward == 0 && right == 0 && up == 0) freecamSpeed = 0;
		freecamSpeed += 0.1;
		cam.rotation = glm::rotate(glm::rotate(glm::identity<glm::mat4x4>(), freecamPitch, glm::vec3(1, 0, 0)), freecamYaw, glm::vec3(0, 1, 0));
		glm::vec3 fDir = LookVector(freecamPitch, freecamYaw);
		glm::vec3 rDir = LookVector(0, freecamYaw + glm::radians(90.0f));
		glm::vec3 upDir = glm::cross(fDir, rDir);
		cam.position += (fDir * forward + rDir * right + upDir * up) * freecamSpeed;
		});
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

Gameobject* DebugLine(glm::dvec3 a, glm::dvec3 b, glm::vec3 color) {
	MeshCreateParams mparams;
	mparams.vertices.push_back(static_cast<float>(a.x));
	mparams.vertices.push_back(static_cast<float>(a.y));
	mparams.vertices.push_back(static_cast<float>(a.z));
	mparams.vertices.push_back(static_cast<float>(b.x));
	mparams.vertices.push_back(static_cast<float>(b.y));
	mparams.vertices.push_back(static_cast<float>(b.z));
	mparams.indices = { 0, 1, };

	mparams.meshVertexFormat = MeshVertexFormat({
		VertexAttribute {.name = SpecialVertexAttributeNames::VERTEX_POSITION, .nComponents = 3, .instanced = false,.type = VertexScalarType::f32},
		VertexAttribute {.name = "color", .nComponents = 4, .instanced = true,.type = VertexScalarType::f32},
		VertexAttribute {.name = SpecialVertexAttributeNames::MODEL_MATRIX, .nComponents = 16, .instanced = true,.type = VertexScalarType::f32},
		VertexAttribute {.name = SpecialVertexAttributeNames::NORMAL_MATRIX, .nComponents = 9, .instanced = true,.type = VertexScalarType::f32}
		});
	mparams.normalizeSize = false;

	auto lineMesh = Mesh::New(std::move(mparams));

	GameobjectCreateParams goparams;
	goparams.renderPasses = { GetDebugLinesPass(), };
	goparams.mesh = lineMesh;
	goparams.primitiveType = GL_LINES;

	auto gameobject = Gameobject::New(goparams);
	gameobject->SetInstanceAttribute(*lineMesh->format.GetAttribute("color"), { color.x, color.y, color.z, 1 });
	return gameobject;
}

Gameobject* DebugTriangle(glm::dvec3 a, glm::dvec3 b, glm::dvec3 c, glm::vec3 color) {
	MeshCreateParams mparams;
	mparams.vertices.push_back(static_cast<float>(a.x));
	mparams.vertices.push_back(static_cast<float>(a.y));
	mparams.vertices.push_back(static_cast<float>(a.z));
	mparams.vertices.push_back(static_cast<float>(b.x));
	mparams.vertices.push_back(static_cast<float>(b.y));
	mparams.vertices.push_back(static_cast<float>(b.z));
	mparams.vertices.push_back(static_cast<float>(c.x));
	mparams.vertices.push_back(static_cast<float>(c.y));
	mparams.vertices.push_back(static_cast<float>(c.z));
	mparams.indices = { 0, 1, 1, 2, 2, 3 };

	mparams.meshVertexFormat = MeshVertexFormat({
		VertexAttribute {.name = SpecialVertexAttributeNames::VERTEX_POSITION, .nComponents = 3, .instanced = false,.type = VertexScalarType::f32},
		VertexAttribute {.name = "color", .nComponents = 4, .instanced = true,.type = VertexScalarType::f32},
		VertexAttribute {.name = SpecialVertexAttributeNames::MODEL_MATRIX, .nComponents = 16, .instanced = true,.type = VertexScalarType::f32},
		VertexAttribute {.name = SpecialVertexAttributeNames::NORMAL_MATRIX, .nComponents = 9, .instanced = true,.type = VertexScalarType::f32}
		});
	mparams.normalizeSize = false;

	auto lineMesh = Mesh::New(std::move(mparams));

	GameobjectCreateParams goparams;
	goparams.renderPasses = { GetDebugLinesPass(), };
	goparams.mesh = lineMesh;
	goparams.primitiveType = GL_LINES;

	auto gameobject = Gameobject::New(goparams);
	gameobject->SetInstanceAttribute(*lineMesh->format.GetAttribute("color"), { color.x, color.y, color.z, 1 });
	return gameobject;
}

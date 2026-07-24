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
#include <constraint.hpp>
#include <physics_engine.hpp>
#include <debug_editing_tools.hpp>
#include "assert.hpp"

static std::shared_ptr<Mesh> CubeMesh() {
	auto mparams = MeshCreateParams::Default();
	mparams.LoadObj("../models/rainbowcube.obj");
	auto cubeMesh = Mesh::New(std::move(mparams));
	return cubeMesh;
}

static std::shared_ptr<Mesh> SphereMesh() {
	auto mparams = MeshCreateParams::Default();
	mparams.LoadObj("../models/icosphere.obj");
	auto sphereMesh = Mesh::New(std::move(mparams));
	return sphereMesh;
}

//static std::shared_ptr<Mesh> CapsuleMesh() {
//	auto mparams = MeshCreateParams::Default();
//	mparams.LoadObj("../models/capsule.obj");
//	auto capsuleMesh = Mesh::New(std::move(mparams));
//	return capsuleMesh;
//}

static std::shared_ptr<Mesh> ArrowMesh() {
	auto mparams = MeshCreateParams::Default();
	mparams.LoadObj("../models/arrowhandle.obj");
	auto arrowMesh = Mesh::New(std::move(mparams));
	return arrowMesh;
}

static std::shared_ptr<Mesh> HulaHoopMesh() {
	auto mparams = MeshCreateParams::Default();
	mparams.LoadObj("../models/rotatehandle.obj");
	auto hulahoopMesh = Mesh::New(std::move(mparams));
	return hulahoopMesh;
}

std::shared_ptr<Mesh> GetCubeMesh()
{
	static auto cubeMesh = CubeMesh();
	return cubeMesh;
}

std::shared_ptr<Mesh> GetArrowMesh()
{
	static auto arrowMesh = ArrowMesh();
	return arrowMesh;
}

std::shared_ptr<Mesh> GetHulaHoopMesh() 
{
	static auto hulahoopMesh = HulaHoopMesh();
	return hulahoopMesh;
}

std::shared_ptr<Mesh> GetSphereMesh() {
	static auto sphere = SphereMesh();
	return sphere;
}

//std::shared_ptr<Mesh> GetCapsuleMesh() {
//	static auto cap = CapsuleMesh();
//	return cap;
//}

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
	pass->params.blending = true;
	rt.loadPolicy = AttachmentLoadPolicy::Load;
	pass->renderTarget = rt;
	pass->dependencies.push_back("POST_PROC");
	pass->outputs.push_back(WINDOW_RESOURCE_NAME);
	pass->params.depthTestMode = DepthTestMode::Disabled;
	pass->params.writeDepthBuffer = true;
	pass->params.cullMode = FaceCulling::None;
	pass->params.polygonFillMode = PolygonFillMode::Fill;
	pass->params.shader = GetDebugShader();
	return pass;
}

static std::shared_ptr<DrawPass> DebugSolidDepthPass() {
	auto pass = std::make_shared<DrawPass>();
	pass->name = "debugSolidDepth";
	auto rt = WindowRenderTargetDescriptor();
	rt.clearDepth = false;
	pass->params.blending = true;
	rt.loadPolicy = AttachmentLoadPolicy::Load;
	pass->renderTarget = rt;
	pass->dependencies.push_back("POST_PROC");
	pass->outputs.push_back(WINDOW_RESOURCE_NAME);
	pass->params.depthTestMode = DepthTestMode::LEqual;
	pass->params.writeDepthBuffer = true;
	pass->params.cullMode = FaceCulling::Backface;
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

std::shared_ptr<DrawPass> GetDebugSolidDepthPass() {
	static auto pass = DebugSolidDepthPass();
	return pass;
}

std::shared_ptr<ConvexMeshPhysicsGeometry> GetCubeCollisions() {
	static auto collisions = ConvexMeshPhysicsGeometry::FromMesh(GetCubeMesh());
	return collisions;
}

Gameobject* BuildSphere(glm::dvec3 pos, float diameter, bool physics, float elasticity, float friction)
{
	GameobjectCreateParams p;
	p.mesh = GetSphereMesh();
	p.physicsMesh = SpherePhysicsGeometry::Get();
	
	Gameobject* go;
	if (physics) {
		go = Physobject::New(p);
		go->elasticity = elasticity;
		go->friction = friction;
	}
	else {
		go = Gameobject::New(p);
	}

	Assert(go->GetCollider());

	go->SetPosition(pos);
	go->SetScale({ diameter, diameter, diameter });
	go->SetInstanceAttribute(*p.mesh->format.GetAttribute("color"), {1, 1, 1, 1});
	go->SetInstanceAttribute(*p.mesh->format.GetAttribute(SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION), -1.0f);

	return go;
}

Gameobject* BuildCapsule(glm::dvec3 pos, float diameter, float height, bool physics, float elasticity, float friction)
{
	GameobjectCreateParams p;
	MeshCreateParams mp;
	mp.LoadCapsule(12, 6, (height - diameter) / diameter);
	p.mesh = Mesh::New(std::move(mp));
	//p.physicsMesh = CapsulePhysicsGeometry::New((height - diameter) / diameter);
	p.physicsMesh = SpherePhysicsGeometry::Get();
	Gameobject* go;
	if (physics) {
		go = Physobject::New(p);
		go->elasticity = elasticity;
		go->friction = friction;
	}
	else {
		go = Gameobject::New(p);
	}

	Assert(go->GetCollider());

	go->SetPosition(pos);
	go->SetScale({ diameter, height, diameter });
	go->SetInstanceAttribute(*p.mesh->format.GetAttribute("color"), { 1, 1, 1, 1 });
	go->SetInstanceAttribute(*p.mesh->format.GetAttribute(SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION), -1.0f);

	return go;
}

std::shared_ptr<ConvexMeshPhysicsGeometry> GetArrowCollisions() {
	static auto collisions = ConvexMeshPhysicsGeometry::FromMesh(GetArrowMesh());
	return collisions;
}

std::shared_ptr<ConvexMeshPhysicsGeometry> GetHulaHoopCollisions() {
	static auto collisions = ConvexMeshPhysicsGeometry::FromMesh(GetHulaHoopMesh());
	return collisions;
}

std::vector<Gameobject*> BuildPit(glm::dvec3 pos, glm::vec3 size, float elasticity, float friction) {
	GameobjectCreateParams p;
	p.mesh = GetCubeMesh();
	p.physicsMesh = GetCubeCollisions();

	glm::vec4 floorColor(0, 1, 0, 1);
	glm::vec4 wallColor(0.7, 0.3, 0.3, 1);

	std::vector<Gameobject*> ret;

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

	ret.push_back(floor);
	ret.push_back(wall1);
	ret.push_back(wall2);
	return ret;
}

std::vector<Gameobject*> BuildCubeArray(glm::dvec3 origin, glm::dvec3 stride, glm::uvec3 nCubes, bool physics, float elasticity, float friction) {
	GameobjectCreateParams p;
	p.mesh = GetCubeMesh();
	p.physicsMesh = GetCubeCollisions();
	
	std::vector<Gameobject*> ret;

	for (unsigned i = 0; i < nCubes.x; i++) {
		for (unsigned j = 0; j < nCubes.y; j++) {
			for (unsigned k = 0; k < nCubes.z; k++) {
				Gameobject* gameObj;
				if (physics) {
					Physobject* o = Physobject::New(p);
					o->friction = friction;
					o->elasticity = elasticity;
					gameObj = o;
				}
				else {
					gameObj = Gameobject::New(p);
				}				
				gameObj->SetPosition(origin + stride * glm::dvec3(i, j, k));
				gameObj->SetScale({ 1, 1, 1 });
				glm::vec4 color(1, 0.7, 1, 1);
				if (rand() < RAND_MAX / 2) color.x *= 0.5;
				gameObj->SetInstanceAttribute(*p.mesh->format.GetAttribute("color"), color);
				gameObj->SetInstanceAttribute(*p.mesh->format.GetAttribute(SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION), -1.0f);
			
				ret.push_back(gameObj);
			}
		}
	}

	return ret;
}

std::vector<Gameobject*> BuildChain(glm::dvec3 pos, float segmentLength, size_t nLinks, float compliance) {
	auto vec = BuildCubeArray(pos, { 1, 1, 1 }, { 1, 1, 1 }, false, 0.0f, 0.0f);

	PhysobjectCreateParams p;
	p.mesh = GetCubeMesh();
	p.physicsMesh = GetCubeCollisions();

	JointParams jp; 
	jp.maxDistance = 2.0f;
	jp.forwardAxis = { 0, 0, 1 };
	jp.upAxis = { 0, 1, 0 };
	jp.inverseStiffness = compliance;
	for (size_t i = 0; i < nLinks; i++) {
		Physobject* obj = Physobject::New(p);
		obj->SetPosition(pos + glm::dvec3(0, -(segmentLength + 1) * (i + 1), 0));
		obj->SetInstanceAttribute(*p.mesh->format.GetAttribute("color"), {0.8, 0.8, 0.7, 1});
		obj->SetInstanceAttribute(*p.mesh->format.GetAttribute(SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION), -1.0f);

		if (i == 0) {
			StaticJoint joint;
			joint.a = obj;
			joint.b = vec.back();
			joint.r1 = { 0, 0.5, 0 };
			joint.r2 = { 0, -0.5, 0 };
			joint.params = jp;
			PhysicsEngine::Get().staticJoints.insert(joint);
		}
		else {
			DynamicJoint joint;
			joint.a = obj;
			joint.b = dynamic_cast<Physobject*>(vec.back());
			joint.r1 = { 0, 0.5, 0 };
			joint.r2 = { 0, -0.5, 0 };
			joint.params = jp;
			PhysicsEngine::Get().dynamicJoints.insert(joint);
		}

		if (i == nLinks - 1) {
			TransformHandles(obj);
		}

		vec.push_back(obj);
	}

	return vec;
}

std::shared_ptr<Camera> ImplGetFreecam() {
	auto cam = std::make_shared<Camera>();

	static auto c = Window::Get().postInputProccessing.Connect([cam](Window*) {
		if (Window::Get().PRESS_BEGAN_KEYS.contains(InputType::Tab)) Window::Get().SetMouseLocked(!Window::Get().IsMouseLocked());

		if (Window::Get().IsMouseLocked()) {

			freecamPitch += 0.01f * Window::Get().MOUSE_DELTA.y;
			freecamYaw += 0.01f * Window::Get().MOUSE_DELTA.x;
			freecamPitch = std::clamp(freecamPitch, -glm::radians(89.0f), glm::radians(89.0f));
			if (freecamYaw < 0.0f) freecamYaw += glm::radians(360.0f);
			freecamYaw = std::fmod(freecamYaw, glm::radians(360.0f));

		}

		if (GraphicsEngine::Get().currentCamera == cam) {
			float forward = (Window::Get().PRESSED_KEYS.contains(InputType::W) ? 1.0f : 0.0f) - (Window::Get().PRESSED_KEYS.contains(InputType::S) ? 1.0f : 0.0f);
			float right = (Window::Get().PRESSED_KEYS.contains(InputType::D) ? 1.0f : 0.0f) - (Window::Get().PRESSED_KEYS.contains(InputType::A) ? 1.0f : 0.0f);
			float up = (Window::Get().PRESSED_KEYS.contains(InputType::Q) ? 1.0f : 0.0f) - (Window::Get().PRESSED_KEYS.contains(InputType::E) ? 1.0f : 0.0f);

			if (forward == 0 && right == 0 && up == 0) freecamSpeed = 0;
			freecamSpeed += 0.1;
			cam->rotation = glm::rotate(glm::rotate(glm::identity<glm::mat4x4>(), freecamPitch, glm::vec3(1, 0, 0)), freecamYaw, glm::vec3(0, 1, 0));
			glm::vec3 fDir = LookVector(freecamPitch, freecamYaw);
			glm::vec3 rDir = LookVector(0, freecamYaw + glm::radians(90.0f));
			glm::vec3 upDir = glm::cross(fDir, rDir);
			cam->position += (fDir * forward + rDir * right + upDir * up) * freecamSpeed;
		}
	});

	return cam;
}

std::shared_ptr<Camera> GetFreecam() {
	Window::Get().SetMouseLocked(true);

	static std::shared_ptr<Camera> freecamCamera = ImplGetFreecam();

	return freecamCamera;
}

Gameobject* DebugArrow(glm::vec3 pos, glm::vec3 direction, glm::vec3 color) {
	Assert(glm::length(direction) != 0);
	direction = glm::normalize(direction);

	GameobjectCreateParams p;
	p.mesh = GetArrowMesh();
	p.physicsMesh = GetArrowCollisions();
	p.renderPasses = { GetDebugSolidPass(), };
	Gameobject* gameObj = Gameobject::New(p);
	gameObj->SetScale(GetArrowMesh()->OriginalSize() * 0.25f);
	glm::vec3 currentDir(0, 1, 0);
	gameObj->GetCollider()->canCollide = false;
	gameObj->SetRotation(glm::rotation(currentDir, direction));
	gameObj->SetPosition(pos + direction * 0.25f);
	gameObj->SetInstanceAttribute(*p.mesh->format.GetAttribute("color"), { color.x, color.y, color.z, 0.5f });
	gameObj->SetInstanceAttribute(*p.mesh->format.GetAttribute(SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION), -1.0f);
	return gameObj;
}

Gameobject* DebugHulaHoop(glm::vec3 pos, glm::vec3 direction, glm::vec3 color)
{
	Assert(glm::length(direction) != 0);
	direction = glm::normalize(direction);

	GameobjectCreateParams p;
	p.mesh = GetHulaHoopMesh();
	p.physicsMesh = GetHulaHoopCollisions();
	p.renderPasses = { GetDebugSolidPass(), };
	Gameobject* gameObj = Gameobject::New(p);
	gameObj->SetScale(glm::vec3(0.05f, 2.0f, 2.0f));
	glm::vec3 currentDir(1, 0, 0);
	gameObj->GetCollider()->canCollide = false;
	gameObj->SetRotation(glm::rotation(currentDir, direction));
	gameObj->SetPosition(pos);
	gameObj->SetInstanceAttribute(*p.mesh->format.GetAttribute("color"), { color.x, color.y, color.z, 0.5f });
	gameObj->SetInstanceAttribute(*p.mesh->format.GetAttribute(SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION), -1.0f);
	return gameObj;
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
	mparams.indices = { 0, 1, 2 };

	mparams.meshVertexFormat = MeshVertexFormat({
		VertexAttribute {.name = SpecialVertexAttributeNames::VERTEX_POSITION, .nComponents = 3, .instanced = false,.type = VertexScalarType::f32},
		VertexAttribute {.name = "color", .nComponents = 4, .instanced = true,.type = VertexScalarType::f32},
		VertexAttribute {.name = SpecialVertexAttributeNames::MODEL_MATRIX, .nComponents = 16, .instanced = true,.type = VertexScalarType::f32},
		VertexAttribute {.name = SpecialVertexAttributeNames::NORMAL_MATRIX, .nComponents = 9, .instanced = true,.type = VertexScalarType::f32}
		});
	mparams.normalizeSize = false;

	auto lineMesh = Mesh::New(std::move(mparams));

	GameobjectCreateParams goparams;
	goparams.renderPasses = { GetDebugSolidDepthPass(), };
	goparams.mesh = lineMesh;
	goparams.primitiveType = GL_TRIANGLES;

	auto gameobject = Gameobject::New(goparams);
	gameobject->SetInstanceAttribute(*lineMesh->format.GetAttribute("color"), { color.x, color.y, color.z, 1 });
	return gameobject;
}

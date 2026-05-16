#pragma once
#include <string>
#include <vector>
#include <memory>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/quaternion_float.hpp>

class Mesh;
class Gameobject;
class Texture;

struct LoadSceneParams {
	std::string filepath;
	bool collapseSceneHierarchy;
	bool combineMeshes;
};

struct SceneNode {
	//SceneNode(const SceneNode&) = delete;

	std::string name;
	const glm::mat4 transform; // relative to parent
	const glm::vec3 position; // global
	const glm::quat rotation; // global
	const glm::vec3 scale; // global but before applying Mesh::originalSize

	std::vector<std::unique_ptr<SceneNode>> children;
	std::vector<std::shared_ptr<Mesh>> meshes;
};


// A scene loaded from a file (.fbx, .obj, etc.), containing an object hierarchy, meshes, animations, etc.
// Uses ASSIMP under the hood.
class Scene {
public:
	static std::unique_ptr<Scene> LoadScene(LoadSceneParams p);

	std::unique_ptr<SceneNode> root; // never nullptr

	std::vector<std::shared_ptr<Mesh>> meshes;
	std::vector<std::shared_ptr<Texture>> textures;

	~Scene();

	// Creates and returns gameobjects to render the entire scene.
	// 2nd parameter is implementation detail, ignore
	std::vector<Gameobject*> DebugPresent(glm::dvec3 posOffset, SceneNode* = nullptr) const;
private:
	Scene(LoadSceneParams params);
};
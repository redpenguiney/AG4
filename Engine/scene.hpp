#pragma once
#include <string>
#include <vector>
#include <memory>

struct LoadSceneParams {
	std::string filepath;
	bool collapseSceneHierarchy;
	bool combineMeshes;
};

class Mesh;

// A scene loaded from a file (.fbx, .obj, etc.), containing an object hierarchy, meshes, animations, etc.
// Uses ASSIMP under the hood.
class Scene {
public:
	static std::unique_ptr<Scene> LoadScene(LoadSceneParams p);

	std::vector<std::shared_ptr<Mesh>> meshes;
	~Scene();
private:
	Scene(LoadSceneParams params);
};
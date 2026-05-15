#include "scene.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/cimport.h"
#include <log.hpp>
#include <mesh.hpp>

std::unique_ptr<Scene> Scene::LoadScene(LoadSceneParams p) {
	return std::unique_ptr<Scene>(new Scene(p));
}

Scene::~Scene() {
		
}

Scene::Scene(LoadSceneParams params) {
	auto flags = aiProcess_GlobalScale | aiProcess_FlipUVs | aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_ImproveCacheLocality | aiProcess_PopulateArmatureData;
	if (params.collapseSceneHierarchy) flags |= aiProcess_OptimizeGraph;
	if (params.combineMeshes) flags |= aiProcess_OptimizeMeshes;

	const aiScene* scene = aiImportFile(params.filepath.c_str(), flags);
	if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode || !scene->HasMeshes()) {
		aiReleaseImport(scene);
		std::string err = "Mesh::MultiFromFile() failed to load " + params.filepath + " because " + aiGetErrorString();
		DebugLogError(err);
		throw std::runtime_error(err);
	}

	for (unsigned meshI = 0; meshI < scene->mNumMeshes; meshI++) {
		aiMesh* assimpMesh = scene->mMeshes[meshI];

		MeshCreateParams params = MeshCreateParams::Default();

		for (unsigned faceI = 0; faceI < assimpMesh->mNumFaces; faceI++) {
			const aiFace& face = assimpMesh->mFaces[faceI];
			for (unsigned i = 0; i < face.mNumIndices; i++) {
				params.indices.push_back(face.mIndices[i]);
			}
		}


	}

	aiReleaseImport(scene);
}

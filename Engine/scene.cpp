#include "scene.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/cimport.h"
#include <log.hpp>
#include <mesh.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "gameobject.hpp"

std::unique_ptr<Scene> Scene::LoadScene(LoadSceneParams p) {
	return std::unique_ptr<Scene>(new Scene(p));
}

Scene::~Scene() {
		
}

glm::mat4x4 AssimpMatrixToGLM(const aiMatrix4x4& m) {
	return glm::mat4x4( // (this also transposes it for us since assimp is row major and glm is column major)
		m.a1, m.b1, m.c1, m.d1,
		m.a2, m.b2, m.c2, m.d2,
		m.a3, m.b3, m.c3, m.d3,
		m.a4, m.b4, m.c4, m.d4
	);
}

//static void HandleNodeSkeletons(const aiScene* scene, aiNode* node, glm::mat4x4 parentTransform, std::unordered_map<aiMesh*, MeshCreateParams>& meshConversion) {
//	auto localTransform = AssimpMatrixToGLM(node->mTransformation);
//	auto globalTransform = parentTransform * localTransform;
//
//	for (unsigned i = 0; i < node->mNumChildren; i++) {
//		HandleNodeSkeletons(scene, node->mChildren[i], globalTransform, meshConversion);
//	}
//
//
//}

static std::unique_ptr<SceneNode> HandleAssimpNode(const aiScene* scene, aiNode* node, glm::mat4x4 parentTransform, const std::unordered_map<aiMesh*, std::shared_ptr<Mesh>>& meshMap) {
	auto localTransform = AssimpMatrixToGLM(node->mTransformation);
	auto globalTransform = parentTransform * localTransform;

	std::vector<std::unique_ptr<SceneNode>> children;
	for (unsigned i = 0; i < node->mNumChildren; i++) {
		children.push_back(std::move(HandleAssimpNode(scene, node->mChildren[i], globalTransform, meshMap)));
	}

	glm::vec3 scale, translation, skew;
	glm::vec4 perspective;
	glm::quat rotation;
	glm::decompose(globalTransform, scale, rotation, translation, skew, perspective);

	std::vector<std::shared_ptr<Mesh>> meshes;
	for (unsigned i = 0; i < node->mNumMeshes; i++) {
		aiMesh* assimpMesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(meshMap.at(assimpMesh));
	}

	SceneNode n = SceneNode {
		.name = node->mName.C_Str(),
		.transform = localTransform,
		.position = translation,
		.rotation = rotation,
		.scale = scale,
		.children = std::move(children),
		.meshes = meshes
	};

	return std::unique_ptr<SceneNode>(new SceneNode(std::move(n)));
}

std::vector<Gameobject*> Scene::DebugPresent(glm::dvec3 posOffset, SceneNode* n) const {
	if (!n) n = root.get();
	std::vector<Gameobject*> result;
	for (auto& c : n->children) {
		auto childObjects = DebugPresent(posOffset, c.get());
		result.insert(result.end(), childObjects.begin(), childObjects.end());
	}
	for (auto& m : n->meshes) {
		GameobjectCreateParams params;
		params.mesh = m;
		Gameobject* obj = Gameobject::New(params);
		obj->SetPosition(glm::dvec3(n->position) + posOffset + glm::dvec3(m->OriginalOffset()));
		obj->SetRotation(n->rotation);
		obj->SetScale(n->scale * m->OriginalSize());
		obj->SetInstanceAttribute(*m->format.GetAttribute("color"), glm::vec4(1, 1, 1, 1));
		obj->SetInstanceAttribute(*m->format.GetAttribute(SpecialVertexAttributeNames::AUTOMATIC_TEXTURE_ARRAY_SELECTION), -1.0f);
		result.push_back(obj);
	}
	return result;
}

Scene::Scene(LoadSceneParams loadParams) {
	auto flags = aiProcess_GlobalScale | aiProcess_FlipUVs | aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_CalcTangentSpace | aiProcess_ImproveCacheLocality | aiProcess_PopulateArmatureData;
	if (loadParams.collapseSceneHierarchy) flags |= aiProcess_OptimizeGraph;
	if (loadParams.combineMeshes) flags |= aiProcess_OptimizeMeshes;

	const aiScene* scene = aiImportFile(loadParams.filepath.c_str(), flags);
	if (scene == nullptr || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode || !scene->HasMeshes()) {
		aiReleaseImport(scene);
		std::string err = "Mesh::MultiFromFile() failed to load " + loadParams.filepath + " because " + aiGetErrorString();
		DebugLogError(err);
		throw std::runtime_error(err);
	}

	std::unordered_map<aiMesh*, MeshCreateParams> meshConversion;
	for (unsigned meshI = 0; meshI < scene->mNumMeshes; meshI++) {
		aiMesh* assimpMesh = scene->mMeshes[meshI];

		MeshCreateParams params = MeshCreateParams::Default();
		params.meshVertexFormat = MeshVertexFormat::Default(assimpMesh->HasBones());
		params.generateNormals = false;
		params.generateTangents = false;
		params.normalizeSize = true;
		unsigned vertStride = params.meshVertexFormat.ScalarsPerVertex(); // in scalars, not bytes

		for (unsigned faceI = 0; faceI < assimpMesh->mNumFaces; faceI++) {
			const aiFace& face = assimpMesh->mFaces[faceI];
			for (unsigned i = 0; i < face.mNumIndices; i++) {
				params.indices.push_back(face.mIndices[i]);
			}
		}

		params.vertices.resize(assimpMesh->mNumVertices * params.meshVertexFormat.ScalarsPerVertex());

		Assert(assimpMesh->HasPositions());
		Assert(params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_POSITION) != nullptr);
		Assert(params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_POSITION)->nComponents >= 3);
		Assert(params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_POSITION)->instanced == false);
		Assert(params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_POSITION)->type == VertexScalarType::f32);

		unsigned posOffset = params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_POSITION)->ScalarOffset();
		for (unsigned vertI = 0; vertI < assimpMesh->mNumVertices; vertI++) {
			params.vertices[vertI * vertStride + posOffset + 0] = assimpMesh->mVertices[vertI].x;
			params.vertices[vertI * vertStride + posOffset + 1] = assimpMesh->mVertices[vertI].y;
			params.vertices[vertI * vertStride + posOffset + 2] = assimpMesh->mVertices[vertI].z;
		}

		Assert(assimpMesh->HasNormals());
		Assert(params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_NORMAL) != nullptr);
		Assert(params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_NORMAL)->nComponents >= 3);
		Assert(params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_NORMAL)->instanced == false);
		Assert(params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_NORMAL)->type == VertexScalarType::f32);

		unsigned normalOffset = params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_NORMAL)->ScalarOffset();
		for (unsigned vertI = 0; vertI < assimpMesh->mNumVertices; vertI++) {
			params.vertices[vertI * vertStride + normalOffset + 0] = assimpMesh->mNormals[vertI].x;
			params.vertices[vertI * vertStride + normalOffset + 1] = assimpMesh->mNormals[vertI].y;
			params.vertices[vertI * vertStride + normalOffset + 2] = assimpMesh->mNormals[vertI].z;
		}

		if (assimpMesh->GetNumUVChannels() > 0) {
			// there won't be tangents if there aren't UVs
			Assert(assimpMesh->HasTangentsAndBitangents());
			Assert(params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_TANGENT) != nullptr);
			Assert(params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_TANGENT)->nComponents >= 3);
			Assert(params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_TANGENT)->instanced == false);
			Assert(params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_TANGENT)->type == VertexScalarType::f32);
			
			unsigned tangentOffset = params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_TANGENT)->ScalarOffset();
			for (unsigned vertI = 0; vertI < assimpMesh->mNumVertices; vertI++) {
				params.vertices[vertI * vertStride + tangentOffset + 0] = assimpMesh->mTangents[vertI].x;
				params.vertices[vertI * vertStride + tangentOffset + 1] = assimpMesh->mTangents[vertI].y;
				params.vertices[vertI * vertStride + tangentOffset + 2] = assimpMesh->mTangents[vertI].z;
			}

			Assert(params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_UV) != nullptr);
			Assert(params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_UV)->nComponents >= 2);
			Assert(params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_UV)->instanced == false);
			Assert(params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_UV)->type == VertexScalarType::f32);

			unsigned uvOffset = params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_UV)->ScalarOffset();
			for (unsigned vertI = 0; vertI < assimpMesh->mNumVertices; vertI++) {
				params.vertices[vertI * vertStride + uvOffset + 0] = assimpMesh->mTextureCoords[0][vertI].x;
				params.vertices[vertI * vertStride + uvOffset + 1] = assimpMesh->mTextureCoords[0][vertI].y;
			}
		}

		if (assimpMesh->HasBones()) {
			bool tooManyWeights = false;

			std::vector<uint8_t> boneCounts;
			boneCounts.resize(params.vertices.size() / vertStride, 0);

			// todo: we should use shorts for bone ids.
			// todo: assuming we do that, can OpenGL do half precision floats and then we fit in 8 bone influences?
			Assert(params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_BONE_IDS) != nullptr);
			Assert(params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_BONE_IDS)->nComponents == 4);
			Assert(params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_BONE_IDS)->instanced == false);
			Assert(params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_BONE_IDS)->type == VertexScalarType::i32);
			Assert(params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_BONE_WEIGHTS) != nullptr);
			Assert(params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_BONE_WEIGHTS)->nComponents == 4);
			Assert(params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_BONE_WEIGHTS)->instanced == false);
			Assert(params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_BONE_WEIGHTS)->type == VertexScalarType::f32);

			unsigned weightOffset = params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_BONE_WEIGHTS)->ScalarOffset();
			unsigned idOffset = params.meshVertexFormat.GetAttribute(SpecialVertexAttributeNames::VERTEX_BONE_IDS)->ScalarOffset();

			for (unsigned boneI = 0; boneI < assimpMesh->mNumBones; boneI++) {
				aiBone* bone = assimpMesh->mBones[boneI];

				for (unsigned weightI = 0; weightI < bone->mNumWeights; weightI++) {
					unsigned vertI = bone->mWeights[weightI].mVertexId;
					unsigned i = boneCounts[vertI];
					if (i == 4) {
						tooManyWeights = true;
						float smallestInfluence = params.vertices[vertI * vertStride + weightOffset + 0].f;
						for (unsigned j = 1; j < 4; j++) {
							if (params.vertices[vertI * vertStride + weightOffset + j].f < smallestInfluence) {
								smallestInfluence = params.vertices[vertI * vertStride + weightOffset + j].f;
								i = j;
							}
						}
					}
					else {
						boneCounts[vertI]++;
					}
					params.vertices[vertI * vertStride + idOffset + i].i = boneI;
					params.vertices[vertI * vertStride + weightOffset + i].f = bone->mWeights[weightI].mWeight;
				}
			}

			// normalize bone weights and handle vertices with less than 4 bone influences by setting remaining ones to 0
			for (unsigned vertI = 0; vertI < assimpMesh->mNumVertices; vertI++) {
				float totalWeight = 0.0f;
				for (unsigned i = 0; i < boneCounts[vertI]; i++) {
					totalWeight += params.vertices[vertI * vertStride + weightOffset + i].f;
				}
				if (totalWeight > 0.0f) {
					for (unsigned i = 0; i < boneCounts[vertI]; i++) {
						params.vertices[vertI * vertStride + weightOffset + i].f /= totalWeight;
					}
				}
				for (unsigned i = boneCounts[vertI]; i < 4; i++) {
					params.vertices[vertI * vertStride + weightOffset + i].f = 0.0f;
					params.vertices[vertI * vertStride + idOffset + i].i = -1;
				}
			}

			if (tooManyWeights) {
				DebugLogInfo("Mesh ", assimpMesh->mName.C_Str(), + " from scene ", loadParams.filepath, " has vertices with more than 4 bone influences. They have been adjusted to only consider 4 bones.");
			}
		}

		meshConversion.insert({assimpMesh, std::move(params)});
	}

	for (unsigned int animI = 0; animI < scene->mNumAnimations; animI++) {
		aiAnimation* anim = scene->mAnimations[animI];
		
		// todo: support non skeletal animation

		unsigned int numAffectedBones = 0;
		for (unsigned int channelIndex = 0; channelIndex < anim->mNumChannels; channelIndex++) {
			aiNodeAnim* channel = anim->mChannels[channelIndex];

			if (boneNamesToIds.count(channel->mNodeName.C_Str())) {
				numAffectedBones++;
			}
		}
	}

	std::unordered_map<aiMesh*, std::shared_ptr<Mesh>> actualMeshes;
	for (auto& [assimpMesh, params] : meshConversion) {
		meshes.push_back(Mesh::New(std::move(params)));
		meshes.back()->name = assimpMesh->mName.C_Str();
		actualMeshes.insert({ assimpMesh, meshes.back() });
	}

	root = HandleAssimpNode(scene, scene->mRootNode, glm::identity<glm::mat4x4>(), actualMeshes);

	aiReleaseImport(scene);
}

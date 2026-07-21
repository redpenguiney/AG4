#pragma once
#include <memory>
#include <vector>
#include "mesh_provider.hpp"
#include "animation.hpp"

class StaticMeshpool;
class Meshpool;

// You do not need to hold onto a shared_ptr of a Mesh to keep the gameobjects using it rendered; RenderGroup does that for you.
class Mesh {
public:
	static std::shared_ptr<Mesh> New(MeshCreateParams params);
	// returns a UV-mapped quad for postprocessing purposes.
	static std::shared_ptr<Mesh> Quad();

	// Like Quad() but with the gui mesh format and normalized vertex positions.
	static std::shared_ptr<Mesh> GuiQuad();

	const MeshVertexFormat format;
	
	const std::vector<VertexScalar>& GetVertices();
	const std::vector<unsigned>& GetIndices();

	// note: numVertices != vertices.size(); this is number of actual vertices, while vertices stores many numbers per vertex
	const unsigned numVertices;
	const unsigned numIndices;

	// for development/debugging purposes only. Does not affect anything.
	std::string name;

	// returns vec3(1) if the mesh vertex positions were not normalized.
	glm::vec3 OriginalSize() const;
	// returns vec3(0) if the mesh vertex positions were not normalized. offset is scaled by OriginalSize()
	glm::vec3 OriginalOffset() const;

	~Mesh();

	// Gets bone with given name, or nullptr if not found
	const Bone* GetBone(std::string name) const;

	const std::vector<Bone> bones;
	const std::vector<Animation> animations;

	unsigned GetBaseVertex() const;
	unsigned GetFirstIndex() const;
private:
	 std::vector<VertexScalar> vertices;
	const std::vector<unsigned> indices;

	glm::vec3 originalSize = { -1, -1, -1 };
	glm::vec3 originalOffset = { 0, 0, 0 };

	Mesh(MeshCreateParams params);
	Mesh(const Mesh&) = delete;

	unsigned baseVertex;
	unsigned firstIndex;
	std::shared_ptr<Meshpool> pool;

	friend class RenderGroup;

	static inline std::vector<std::shared_ptr<StaticMeshpool>> staticMeshpools;
};
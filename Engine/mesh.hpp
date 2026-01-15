#pragma once
#include <memory>
#include <vector>
#include "mesh_provider.hpp"

class StaticMeshpool;
class Meshpool;

class Mesh {
public:
	static std::shared_ptr<Mesh> New(MeshCreateParams params);

	const MeshVertexFormat format;
	
	const std::vector<VertexScalar>& GetVertices();
	const std::vector<unsigned>& GetIndices();

	// note: numVertices != vertices.size(); this is number of actual vertices, while vertices stores many numbers per vertex
	const unsigned numVertices;
	const unsigned numIndices;

	~Mesh();

private:
	 std::vector<VertexScalar> vertices;
	const std::vector<unsigned> indices;

	// -1 if the mesh vertex positions were not normalized.
	glm::vec3 originalSize = { -1, -1, -1 };

	Mesh(MeshCreateParams params);
	Mesh(const Mesh&) = delete;

	unsigned baseVertex;
	unsigned firstIndex;
	std::shared_ptr<Meshpool> pool;

	friend class RenderGroup;

	static inline std::vector<std::shared_ptr<StaticMeshpool>> staticMeshpools;
};
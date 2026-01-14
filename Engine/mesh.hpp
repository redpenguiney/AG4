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
	const std::vector<VertexScalar> vertices;
	const std::vector<unsigned> indices;

	const unsigned numVertices;
	const unsigned numIndices;

	~Mesh();

private:
	Mesh(MeshCreateParams params);
	Mesh(const Mesh&) = delete;

	unsigned baseVertex;
	unsigned firstIndex;
	std::shared_ptr<Meshpool> pool;

	friend class RenderGroup;

	static inline std::vector<std::shared_ptr<StaticMeshpool>> staticMeshpools;
};
#pragma once
#include <memory>
#include <vector>
#include "mesh_provider.hpp"

class Meshpool;

class Mesh {
public:
	static std::shared_ptr<Mesh> New(MeshCreateParams params);

	const unsigned numVertices;
	const unsigned numIndices;

	const MeshVertexFormat format;
	const std::vector<VertexScalar> vertices;
	const std::vector<unsigned> indices;

private:
	Mesh(MeshCreateParams params);
	~Mesh();

	unsigned baseVertex;
	unsigned firstIndex;
	Meshpool* pool;

	friend class RenderGroup;
};
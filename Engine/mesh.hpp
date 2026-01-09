#pragma once
#include <memory>
#include <vector>
#include "mesh_provider.hpp"

class Meshpool;

class Mesh {
public:
	static std::shared_ptr<Mesh> New(MeshProvider&& provider);

	const MeshVertexFormat format;
	const std::vector<VertexScalar> vertices;
	const std::vector<unsigned> indices;

private:
	Mesh(MeshProvider&& provider);
	~Mesh();

	unsigned baseVertex;
	unsigned firstIndex;
	unsigned numIndices;
	Meshpool* pool;

	friend class RenderGroup;
};
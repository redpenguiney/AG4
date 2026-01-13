#include "mesh.hpp"
#include "static_meshpool.hpp"
#include "mesh_provider.hpp"

std::shared_ptr<Mesh> Mesh::New(MeshCreateParams params)
{
	auto ptr = std::shared_ptr<Mesh>(new Mesh(std::move(params)));
	auto loc = ptr->pool->AddMesh(ptr);
	ptr->baseVertex = loc.baseVertex;
	ptr->firstIndex = loc.firstIndex;
	return ptr;
}

Mesh::Mesh(MeshCreateParams params):
vertices(std::move(params.vertices)),
indices(std::move(params.indices)),
format(params.meshVertexFormat),
numVertices(vertices.size() / format.ScalarsPerVertex()),
numIndices(indices.size()) {

}

Mesh::~Mesh() {
	pool->RemoveMesh(this);
}

#include "mesh.hpp"
#include "static_meshpool.hpp"
#include "mesh_provider.hpp"

std::shared_ptr<Mesh> Mesh::New(MeshCreateParams params)
{
	Mesh* m = new Mesh(std::move(params));
	std::shared_ptr<Mesh> ptr(m);
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

#include "mesh.hpp"
#include "static_meshpool.hpp"
#include "mesh_provider.hpp"

std::shared_ptr<Mesh> Mesh::New(MeshCreateParams params)
{
	std::shared_ptr<Mesh> ptr(new Mesh(provider));
	auto loc = ptr->pool->AddMesh(ptr);
	ptr->baseVertex = loc.baseVertex;
	ptr->firstIndex = loc.firstIndex;
	ptr->numIndices = loc.nIndices;
	return ptr;
}

Mesh::Mesh(MeshCreateParams params):
vertices(provider.GetVertices()),
format(provider.meshParams.meshVertexFormat) {

}

Mesh::~Mesh() {
	pool->RemoveMesh(this);
}

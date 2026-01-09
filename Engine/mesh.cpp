#include "mesh.hpp"
#include "static_meshpool.hpp"

std::shared_ptr<Mesh> Mesh::New(MeshProvider&& provider)
{
	std::shared_ptr<Mesh> ptr(new Mesh(provider));
	auto loc = ptr->pool->AddMesh(ptr);
	ptr->baseVertex = loc.baseVertex;
	ptr->firstIndex = loc.firstIndex;
	ptr->numIndices = loc.nIndices;
	return ptr;
}

Mesh::Mesh(MeshProvider&& provider):
Mesh(provider.GetMesh()),
format(provider.meshParams.meshVertexFormat) {

}

Mesh::~Mesh() {
	pool->RemoveMesh(this);
}

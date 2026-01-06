#include "mesh.hpp"
#include "static_meshpool.hpp"

Mesh::Mesh(const MeshProvider& provider):
Mesh(provider.GetMesh()),
format(provider.meshParams.meshVertexFormat),
{

}

Mesh::~Mesh() {
	pool->RemoveMesh(this);
}

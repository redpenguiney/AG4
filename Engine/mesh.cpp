#include "mesh.hpp"
#include "static_meshpool.hpp"
#include "mesh_provider.hpp"

std::shared_ptr<Mesh> Mesh::New(MeshCreateParams params)
{
	Mesh* m = new Mesh(std::move(params));
	auto ptr = std::shared_ptr<Mesh>(m);

	auto loc = ptr->pool->AddMesh(ptr);
	ptr->baseVertex = loc.baseVertex;
	ptr->firstIndex = loc.firstIndex;
	return ptr;
}

const std::vector<VertexScalar>& Mesh::GetVertices() {
	return vertices;
}

const std::vector<unsigned>& Mesh::GetIndices() {
	return indices;
}

Mesh::Mesh(MeshCreateParams params):
numVertices(params.vertices.size() / params.meshVertexFormat.ScalarsPerVertex()),
numIndices(params.indices.size()),
vertices(std::move(params.vertices)),
indices(std::move(params.indices)),
format(params.meshVertexFormat)
{
	Assert(params.isStatic);
	Assert(numVertices > 0);
	Assert(numIndices > 0);
	if (params.isStatic) {
		for (auto& p : staticMeshpools) {
			if (p->format == format) {
				pool = p;
			}
		}
		if (!pool) pool = staticMeshpools.emplace_back(std::make_shared<StaticMeshpool>(format));
	}
	// baseVertex and firstIndex are set in New().

	// Preprocess mesh
	// Mesh normalization
	if (params.normalizeSize) {
		const auto posAttribute = format.GetAttribute(SpecialVertexAttributeNames::VERTEX_POSITION);
		unsigned stride = format.ScalarsPerVertex();
		Assert(posAttribute);
		Assert(posAttribute->nComponents <= 3);
		Assert(posAttribute->type == VertexScalarType::f32);
		glm::vec3 min = { 0, 0, 0 }, max = { 0, 0, 0 };
		for (unsigned i = 0; i < numVertices; i++) {
			for (unsigned j = 0; j < posAttribute->nComponents; j++) {
				min[j] = std::min(min[j], vertices[i * stride + j].f);
				max[j] = std::max(max[j], vertices[i * stride + j].f);
			}
		}

		for (unsigned i = 0; i < numVertices; i++) {
			for (unsigned j = 0; j < posAttribute->nComponents; j++) {
				vertices[i * stride + j].f = (vertices[i * stride + j].f - min[j]) / (max[j] - min[j]) - 0.5f;
			}
		}

		for (unsigned j = 0; j < posAttribute->nComponents; j++) originalSize[j] = max[j] - min[j];
	}
	else {
		originalSize = { -1, -1, -1 };
	}

}

Mesh::~Mesh() {
	pool->RemoveMesh(this);
}

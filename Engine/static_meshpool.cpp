#include "static_meshpool.hpp"
#include "mesh.hpp"
#include "shader_program.hpp"

void Meshpool::StreamModelMatrix(unsigned instance, glm::mat4x4 modelMatrix) {
	memcpy(instances.Data() + instance * format.GetInstancedVertexSize() + modelMatrixOffset, &modelMatrix, sizeof(modelMatrix));
}

void Meshpool::StreamNormalMatrix(unsigned instance, glm::mat3x3 normalMatrix) {
	memcpy(instances.Data() + instance * format.GetInstancedVertexSize() + normalMatrixOffset, &normalMatrix, sizeof(normalMatrix));
}

void Meshpool::CommitWrites() {
	vertices.Commit();
	instances.Commit();
	indices.Commit();
}

void Meshpool::FlipBuffers() {
	vertices.Flip(); // TODO: for static meshpools we shouldn't always need to do this.
	instances.Flip();
	indices.Flip();
}

Meshpool::~Meshpool() {
	for (unsigned i = 0; i < pools.size(); i++) {
		if (pools[i] == this) {
			pools[i] = pools.back();
			pools.pop_back();
			break;
		}
	}

	glDeleteVertexArrays(1, &vao);
	idProvider.ReturnId(id);
}

void Meshpool::PrepareDraw() {
	for (auto& p : pools) {
		p->CommitWrites();
	}
}

void Meshpool::PrepareWrite() {
	for (auto& p : pools) {
		p->FlipBuffers();
	}
}

void Meshpool::BindVAO(const std::shared_ptr<ShaderProgram>& shader) {
	if (!vaos.contains(shader.get())) {
		unsigned vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		
		vertices.Bind();
		for (const auto& attributeRequested : shader->GetInputVertexAttributes()) {
			for (auto& attribute : format.GetAttributes())	 {
				Assert(false); // TODO
				/*if (!attribute.instanced && attribute.name == attributeRequested.name) {
					unsigned numArraysRequired = 0;
					attributeRequested.t;
					for (unsigned j = attributeRequested.index; j < attributeRequested.index + numArraysRequired; j++) {
						glEnableVertexAttribArray(attributeRequested.index+j);

						if (attribute.type == VertexScalarType::f32) {
							glVertexAttribPointer(attributeRequested.index + j, attribute.nComponents, GL_FLOAT, GL_FALSE, format.GetNonInstancedVertexSize(), (void*)(attribute.offset));
						}
						else {
							GLenum itype;
							switch (attribute.type) {
							case VertexScalarType::i32:
								itype = GL_INT;
								break;
							case VertexScalarType::u32:
								itype = GL_UNSIGNED_INT;
								break;
							default:
								Assert(false);
							}
							glVertexAttribIPointer(attributeRequested.index + j, attribute.nComponents, itype, format.GetNonInstancedVertexSize(), (void*)(attribute.offset));
						}
					}
				}*/
			}
		}
		
		instances.Bind();
	}

	glBindVertexArray(vaos[shader.get()]);
}

Meshpool::Meshpool(MeshVertexFormat f):
format(f),
id(idProvider.GetId()),
vertices(GL_ARRAY_BUFFER, 1, (1<<20) * f.GetNonInstancedVertexSize()),
indices(GL_ELEMENT_ARRAY_BUFFER, 1, (1 << 20) * sizeof(GLuint)),
instances(GL_ARRAY_BUFFER, 3, (1<<20) * f.GetInstancedVertexSize())
{
	pools.push_back(this);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	for (unsigned i = 0; i < format.GetAttributes().size(); i++) {
		glEnableVertexAttribArray(i);
		glVertexAttribDivisor(0, format.GetAttributes()[i].instanced ? 1 : 0);
	}

	for (auto& a : format.GetAttributes()) {
		if (a.name == SpecialVertexAttributeNames::MODEL_MATRIX) {
			modelMatrixOffset = a.offset;
			break;
		}
		else if (a.name == SpecialVertexAttributeNames::NORMAL_MATRIX) {
			normalMatrixOffset = a.offset;
			break;
		}
	}

}

void Meshpool::UpdateVertexCapacity() {
	vertices.Reallocate(currentVertexCapacity * format.GetNonInstancedVertexSize());
	glBindVertexArray(vao);
	vertices.Bind();
	unsigned i = 0;
	for (auto& attribute : format.GetAttributes()) {
		if (!attribute.instanced) {
			
		}
		if (attribute.nComponents <= 4) i++;
		else if (attribute.nComponents == 9) i += 3;
		else if (attribute.nComponents == 16) i += 4;
		else Assert(false);
	}
}

void Meshpool::UpdateIndicesCapacity() {
	indices.Reallocate(currentIndicesCapacity);
}

void Meshpool::UpdateInstanceCapacity() {
	instances.Reallocate(currentInstanceCapacity * format.GetInstancedVertexSize());
	instances.Bind();
	glBindVertexArray(vao);
	unsigned i = 0;
	for (auto& attribute : format.GetAttributes()) {
		if (!attribute.instanced) {
			if (attribute.type == VertexScalarType::f32) {
				glVertexAttribPointer(i, attribute.nComponents, GL_FLOAT, GL_FALSE, format.GetInstancedVertexSize(), (void*)(attribute.offset));
			}
			else {
				GLenum itype;
				switch (attribute.type) {
				case VertexScalarType::i32:
					itype = GL_INT;
					break;
				case VertexScalarType::u32:
					itype = GL_UNSIGNED_INT;
					break;
				default:
					Assert(false);
				}
				glVertexAttribIPointer(i, attribute.nComponents, itype, format.GetInstancedVertexSize(), (void*)(attribute.offset));

			}
		}
		i++;
	}
}

StaticMeshpool::StaticMeshpool(MeshVertexFormat f): Meshpool(f) {

}

StaticMeshpool::~StaticMeshpool() {
}

MeshpoolMeshStorageLocation StaticMeshpool::AddMesh(std::shared_ptr<Mesh> m) {
	unsigned firstVertex = nextMeshFirstVertexLocation;
	nextMeshFirstVertexLocation += m->numVertices;
	if (firstVertex >= currentVertexCapacity) {
		while (firstVertex >= currentVertexCapacity)
			currentVertexCapacity *= 2;
		UpdateVertexCapacity();
	}
	unsigned firstIndex = nextMeshFirstIndexLocation;
	if (firstIndex >= currentIndicesCapacity) {
		while (firstIndex >= currentIndicesCapacity)
			currentIndicesCapacity *= 2;
		UpdateIndicesCapacity();
	}
	nextMeshFirstIndexLocation += m->numIndices;

	return MeshpoolMeshStorageLocation{
		.baseVertex = firstVertex,
		.firstIndex = firstIndex * (unsigned)sizeof(unsigned int),
		.nIndices = m->numIndices
	};
}

void StaticMeshpool::RemoveMesh(Mesh*) {

}

unsigned StaticMeshpool::AddInstance() {
	unsigned instanceSlot;
	if (availableInstanceSlots.size() > 0) {
		instanceSlot = availableInstanceSlots.back();
		availableInstanceSlots.pop_back();
	}
	else {
		instanceSlot = nextInstanceLocation++;
		if (instanceSlot >= currentInstanceCapacity) {
			currentInstanceCapacity *= 2;
			UpdateInstanceCapacity();
		}
	}
	return instanceSlot;
}

void StaticMeshpool::RemoveInstance(unsigned instance) {
	availableInstanceSlots.push_back(instance);
}

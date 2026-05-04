#include "static_meshpool.hpp"
#include "mesh.hpp"
#include "shader_program.hpp"

void Meshpool::StreamModelMatrix(unsigned instance, glm::mat4x4 modelMatrix) {
	memcpy(instances.Data() + instance * format.GetInstancedVertexSize() + modelMatrixOffset, &modelMatrix, sizeof(modelMatrix));
}

void Meshpool::StreamNormalMatrix(unsigned instance, glm::mat3x3 normalMatrix) {
	memcpy(instances.Data() + instance * format.GetInstancedVertexSize() + normalMatrixOffset, &normalMatrix, sizeof(normalMatrix));
}

void StaticMeshpool::CommitWrites() {
	pendingInstanceWrites.ApplyWrites(instances.Data());
	vertices.Commit();
	instances.Commit();
	indices.Commit();
}

void StaticMeshpool::FlipBuffers() {
	vertices.Flip(); // TODO: for static meshpools we shouldn't always need to do this.
	instances.Flip();
	indices.Flip();
}

void Meshpool::DestroyVAOs() {
	for (auto& [s, vao] : vaos) {
		glDeleteVertexArrays(1, &vao);
	}
}

Meshpool::~Meshpool() {
	for (unsigned i = 0; i < pools.size(); i++) {
		if (pools[i] == this) {
			pools[i] = pools.back();
			pools.pop_back();
			break;
		}
	}

	DestroyVAOs();
	
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

void Meshpool::SetupVAOAttributes(const std::shared_ptr<ShaderProgram>& shader, bool instanced) {
	if (instanced) instances.Bind(GL_ARRAY_BUFFER); else vertices.Bind(GL_ARRAY_BUFFER);
	
	for (const auto& attributeRequested : shader->GetInputVertexAttributes()) {
		for (auto& attribute : format.GetAttributes()) {
			if (attribute.name == attributeRequested.name) {
				if (attribute.instanced != instanced) {
					goto attributeFound; // the other pass will or already did take care of it
				}

				unsigned totalNComponents = attributeRequested.nComponentsPerArray * attributeRequested.nArrays;
				// check format
				if (attribute.type != attributeRequested.scalarType || attribute.nComponents != totalNComponents) {
					DebugLogError("While binding VAO for vertex shader ", shader->GetVertexSourcePath(), " we found an inconsistency in formats for the vertex attribute ", attribute.name,
						". requested format was ", totalNComponents, " scalars of type ", (unsigned)attributeRequested.scalarType, " got ", attribute.nComponents, " of type ", (unsigned)attribute.type);
				}
				for (unsigned j = 0; j < attributeRequested.nArrays; j++) {
					unsigned attribIdx = attributeRequested.index + j;
					unsigned stride = attribute.instanced ? format.GetInstancedVertexSize() : format.GetNonInstancedVertexSize();
					glEnableVertexAttribArray(attribIdx);
					glVertexAttribDivisor(attribIdx, attribute.instanced ? 1 : 0);
					if (attribute.type == VertexScalarType::f32) {
						glVertexAttribPointer(attribIdx, attributeRequested.nComponentsPerArray, GL_FLOAT, GL_FALSE, stride, (void*)(attribute.offset + j * attributeRequested.nComponentsPerArray * sizeof(VertexScalar)));
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
						glVertexAttribIPointer(attribIdx, attributeRequested.nComponentsPerArray, itype, stride, (void*)(attribute.offset + j * attributeRequested.nComponentsPerArray * sizeof(VertexScalar)));
					}
				}

			}
		}

	attributeFound:;
	}
}

void Meshpool::BindVAO(const std::shared_ptr<ShaderProgram>& shader) {
	if (!vaos.contains(shader.get())) {
		unsigned vao;
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		SetupVAOAttributes(shader, false);
		SetupVAOAttributes(shader, true);

		for (const auto& attributeRequested : shader->GetInputVertexAttributes()) {
			for (auto& attribute : format.GetAttributes()) {
				if (attribute.name == attributeRequested.name) {
					goto attributeFound;
				}
			}
			DebugLogError("While binding VAO for vertex shader ", shader->GetVertexSourcePath(), " the mesh format did not contain an attribute named ", attributeRequested.name, ".");

		attributeFound:;
		}

		vaos[shader.get()] = vao;
	}

	glBindVertexArray(vaos[shader.get()]);
}

Meshpool::Meshpool(MeshVertexFormat f):
format(f),
id(idProvider.GetId()),
currentVertexCapacity(1 << 16),
currentIndicesCapacity(1 << 16),
currentInstanceCapacity(1 << 16),
vertices(GL_ARRAY_BUFFER, 1, currentVertexCapacity * f.GetNonInstancedVertexSize()),
indices(GL_ELEMENT_ARRAY_BUFFER, 1, currentIndicesCapacity * sizeof(GLuint)),
instances(GL_ARRAY_BUFFER, 3, currentInstanceCapacity* f.GetInstancedVertexSize())
{
	pools.push_back(this);

	for (auto& a : format.GetAttributes()) {
		if (a.name == SpecialVertexAttributeNames::MODEL_MATRIX) {
			modelMatrixOffset = a.offset;
		}
		else if (a.name == SpecialVertexAttributeNames::NORMAL_MATRIX) {
			normalMatrixOffset = a.offset;
		}
	}

}

void Meshpool::UpdateVertexCapacity() {
	vertices.Reallocate(currentVertexCapacity * format.GetNonInstancedVertexSize());
	DestroyVAOs();
	vertices.Bind(GL_ARRAY_BUFFER);
}

void Meshpool::UpdateIndicesCapacity() {
	indices.Reallocate(currentIndicesCapacity);
}

void Meshpool::UpdateInstanceCapacity() {
	instances.Reallocate(currentInstanceCapacity * format.GetInstancedVertexSize());
	DestroyVAOs();
	instances.Bind(GL_ARRAY_BUFFER);
}

StaticMeshpool::StaticMeshpool(MeshVertexFormat f): Meshpool(f) {

}

StaticMeshpool::~StaticMeshpool() {
	//DebugLogInfo("Destroying static pool at ", this);
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

	memcpy(vertices.Data() + firstVertex * format.GetNonInstancedVertexSize(), m->GetVertices().data(), m->GetVertices().size() * sizeof(VertexScalar));
	memcpy(indices.Data() + firstIndex * sizeof(GLuint), m->GetIndices().data(), m->numIndices * sizeof(unsigned int));

	return MeshpoolMeshStorageLocation{
		.baseVertex = firstVertex,
		.firstIndex = firstIndex * (unsigned)sizeof(unsigned int),
		.nIndices = m->numIndices
	};
}

void StaticMeshpool::RemoveMesh(Mesh*) {
	// TODO 
}

void StaticMeshpool::SetInstancedVertexAttribute(unsigned instance, const VertexAttribute& attribute, VertexScalar* value) {
	pendingInstanceWrites.AddWrite(attribute.nComponents, attribute.offset + instance * format.GetInstancedVertexSize(), instances.numBuffers, value);
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

void PendingWritesManager::AddWrite(unsigned nComponents, unsigned writeLocation, unsigned nWrites, VertexScalar* valueToWrite) {
	
	// we have to check if we're already writing to the same location and if so replace that PendingWrite.
	// otherwise, since ApplyWrites() doesn't preserve the order of the writes vector we could end up with the older data written.
	for (auto& w : writes[nComponents]) {
		if (w.writeLocation == writeLocation) {
			w.writesLeft = nWrites;
			// update data
			memcpy(w.data, valueToWrite, nComponents * sizeof(VertexScalar));
		}
	}

	// if that wasn't the case we just add the write
	void* data = malloc(nComponents * sizeof(VertexScalar));
	memcpy(data, valueToWrite, nComponents * sizeof(VertexScalar));
	writes[nComponents].push_back(PendingWrite{
		.data = data,
		.writesLeft = nWrites,
		.writeLocation = writeLocation,
	});
}

void PendingWritesManager::ApplyWrites(char* buffer) {
	for (unsigned nComponents = 0; nComponents < writes.size(); nComponents++) {
		auto& vec = writes[nComponents];
		for (unsigned i = 0; i < vec.size(); i++) {
			memcpy(buffer + vec[i].writeLocation, vec[i].data, nComponents * sizeof(VertexScalar));
			vec[i].writesLeft--;
			if (vec[i].writesLeft == 0) {
				free(vec[i].data);
				vec[i] = vec.back();
				vec.pop_back();
				i--;
			}
		}
	}
}

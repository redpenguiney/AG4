#pragma once
#include "mesh_provider.hpp"
#include "utility.hpp"
#include "buffered_buffer.hpp"
#include <unordered_map>
#include <array>
#include <set>
#include <memory>

class Mesh;
class ShaderProgram;

// Describes where a Mesh's data has been stored within a Meshpool.
struct MeshpoolMeshStorageLocation {
	unsigned baseVertex;
	unsigned firstIndex; // offset IN BYTES
	unsigned nIndices;
};

// We use multiple buffering for instances and bone transforms (TODO: and sometimes vertices).
	// OR TODO: maybe use ring buffer for vertices instead?
// This is totally fine for vertex attributes that are updated every frame like object model matrices.
// But when a user wants to just set something once and forget about it (like for object color), this is no good 
//	  since they would have to write it once on each frame until all buffers are written.
// This class handles that issue.
class PendingWritesManager {
public:
	// doesn't take ownership of data, justs read it.
	void AddWrite(unsigned nComponents, unsigned writeLocation, unsigned nWrites, VertexScalar* data);
	void ApplyWrites(char* buffer);
	void ResetWrites(unsigned nWrites);
private:
	struct PendingWrite {
		void* data;
		unsigned writesLeft;
		unsigned writeLocation;
	};
	std::array<std::vector<PendingWrite>, 16> writes; // nth index is for writes consisting of n-1 scalars
};

class Meshpool {
public:
	constexpr static inline unsigned BONE_SSBO_BINDING_INDEX = 0;

	virtual unsigned AddInstance() = 0; // returns base instance. You can combine calls with adjacent base instance values by increasing instance count.
	virtual void RemoveInstance(unsigned instance) = 0;

	virtual MeshpoolMeshStorageLocation AddMesh(std::shared_ptr<Mesh> m) = 0;
	virtual void RemoveMesh(Mesh*) = 0;

	const MeshVertexFormat format;
	const unsigned id;

	void StreamModelMatrix(unsigned instance, glm::mat4x4);
	void StreamNormalMatrix(unsigned instance, glm::mat3x3);

	// attribute must be part of this meshpool's format, and value must refer to an array with the correct number of scalar values.
	// value will be copied immediately, so don't worry about invalidating the pointer after calling this.
	virtual void SetInstancedVertexAttribute(unsigned instance, const VertexAttribute& attribute, VertexScalar* value) = 0;
	// Like SetInstancedVertexAttribute(), but faster and provided data will only be valid for the next frame (due to multiple buffering). 
	// If you're calling this, call it every frame.
	virtual void StreamInstancedVertexAttribute(unsigned instance, const VertexAttribute& attribute, VertexScalar* value) = 0;

	// boneIndex must be < format.GetBoneCapacity()
	virtual void SetBoneTransform(unsigned instance, unsigned boneIndex, const glm::mat4x4 transform) = 0;
	// Like SetBoneTransform(), but faster and provided data will only be valid for the next frame (due to multiple buffering).
	virtual void StreamBoneTransform(unsigned instance, unsigned boneIndex, const glm::mat4x4 transform) = 0;

	virtual ~Meshpool();
	Meshpool(const Meshpool&) = delete;

	// call every frame AFTER writing vertex/instance data and BEFORE dispatching GPU draws/commands
	static void PrepareDraw();
	// call every frame BEFORE writing vertex/instance data and AFTER dispatching GPU draws/commands
	static void PrepareWrite();

	// Binds the VAO for the shader program, lazily creating it if needed. 
	void BindVAO(const std::shared_ptr<ShaderProgram>& shader);

protected:
	PendingWritesManager pendingInstanceWrites;
	PendingWritesManager pendingBoneWrites; // some slight wastefulness here, don't really care tho

	unsigned modelMatrixOffset;
	unsigned normalMatrixOffset;
	// We store a different vao for every shader program to accomodate different shaders not using every vertex attributes or specifying them in a different order.
	std::unordered_map<ShaderProgram*, unsigned> vaos;

	Meshpool(MeshVertexFormat f);

	unsigned currentVertexCapacity = 0;
	unsigned currentIndicesCapacity = 0;
	unsigned currentInstanceCapacity = 0; // how many instances it can currently hold.

	BufferedBuffer vertices; // stores noninstanced vertex attributes of meshes (the per-mesh data)
	BufferedBuffer indices; // stores mesh vertex indices (triangle definitions)
	BufferedBuffer instances; // stores instanced vertex attributes (the per-object data)
	std::optional<BufferedBuffer> boneTransforms; // per instance, nullopt if format.boneCapacity == 0.

	void UpdateVertexCapacity(); // after changing currentVertexCapacity, call to update vertices to the correct size.
	void UpdateIndicesCapacity(); // after changing currentIndicesCapacity, call to update indices to the correct size.
	void UpdateInstanceCapacity(); // after changing currentInstanceCapacity, call to update instances to the correct size.
	
	// needed for BufferedBuffer's double/triple buffering, call every frame AFTER writing vertex/instance data and BEFORE dispatching GPU draws/commands which use this meshpool.
	virtual void CommitWrites() = 0;

	// needed for BufferedBuffer's double/triple buffering, call every frame BEFORE writing vertex/instance data and AFTER dispatching GPU draws/commands which use this meshpool.
	// Might yield if GPU isn't ready for us to write the data, so call at the last possible second.
	virtual void FlipBuffers() = 0;

	// Used by destructor and when a buffer is resized.
	void DestroyVAOs();

private:
	// helper function for BindVAO
	void SetupVAOAttributes(const std::shared_ptr<ShaderProgram>& shader, bool instanced);

	static inline IdProvider idProvider;
	static inline std::vector<Meshpool*> pools;

	friend class RenderGraph;

};

struct SlotSpace {
	unsigned first;
	unsigned count;

	bool operator<(const SlotSpace other) const;
};

class StaticMeshpool : public Meshpool {
public:
	StaticMeshpool(MeshVertexFormat f);
	~StaticMeshpool();
	StaticMeshpool(const StaticMeshpool&) = delete;

	unsigned AddInstance() override; // returns instance ID
	void RemoveInstance(unsigned instance) override;

	MeshpoolMeshStorageLocation AddMesh(std::shared_ptr<Mesh> m) override;
	void RemoveMesh(Mesh*) override;

	void SetInstancedVertexAttribute(unsigned instance, const VertexAttribute& attribute, VertexScalar* value) override;
	void StreamInstancedVertexAttribute(unsigned instance, const VertexAttribute& attribute, VertexScalar* value) override; // TODO untested

	void SetBoneTransform(unsigned instance, unsigned boneIndex, const glm::mat4x4 transform) override;
	void StreamBoneTransform(unsigned instance, unsigned boneIndex, const glm::mat4x4 transform) override;

	void CommitWrites() override;
	void FlipBuffers() override;

private:
	unsigned GetVertexLocationForNewMesh(unsigned nVerts); // retvalue in terms of vertices
	unsigned GetIndexLocationForNewMesh(unsigned nIndices); // retvalue in terms of indices

	std::vector<unsigned> availableInstanceSlots;
	unsigned nextInstanceLocation = 0; // use if availableInstanceSlots is empty.

	std::multiset<SlotSpace> availableVertexSpaces; // in terms of vertices. sorted by count.
	std::multiset<SlotSpace> availableIndexSpaces; // in terms of indices. sorted by count.

	std::unordered_map<Mesh*, unsigned> meshFirstVertexLocations; // index of the first vertex of each mesh 
	std::unordered_map<Mesh*, unsigned> meshFirstIndexLocations; // index of the first index of each mesh 

	unsigned nextMeshFirstVertexLocation = 0;
	unsigned nextMeshFirstIndexLocation = 0;
};
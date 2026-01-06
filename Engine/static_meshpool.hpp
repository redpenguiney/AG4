#pragma once
#include "mesh_provider.hpp"
#include "utility.hpp"
#include "buffered_buffer.hpp"
#include <unordered_map>

class Mesh;

// Describes where a Mesh's data has been stored within a Meshpool.
struct MeshpoolMeshStorageLocation {
	unsigned baseVertex;
	unsigned firstIndex;
	unsigned nIndices;
};

class Meshpool {
public:
	virtual unsigned AddInstance() = 0; // returns base instance. You can combine calls with adjacent base instance values by increasing instance count.
	virtual void RemoveInstance(unsigned instance) = 0;

	virtual MeshpoolMeshStorageLocation AddMesh(std::shared_ptr<Mesh> m) = 0;
	virtual void RemoveMesh(Mesh*) = 0;

	const MeshVertexFormat format;
	const unsigned id;

	void StreamModelMatrix(unsigned instance, glm::mat4x4);

	

	virtual ~Meshpool();
	Meshpool(const Meshpool&) = delete;

	// call every frame AFTER writing vertex/instance data and BEFORE dispatching GPU draws/commands
	static void PrepareDraw();
	// call every frame BEFORE writing vertex/instance data and AFTER dispatching GPU draws/commands
	static void PrepareWrite();

protected:
	unsigned modelMatrixOffset;
	unsigned vao;

	Meshpool(MeshVertexFormat f);

	BufferedBuffer vertices; // stores noninstanced vertex attributes of meshes (the per-mesh data)
	BufferedBuffer indices; // stores mesh vertex indices (triangle definitions)
	BufferedBuffer instances; // stores instanced vertex attributes (the per-object data)

	unsigned currentVertexCapacity; 
	unsigned currentIndicesCapacity;
	unsigned currentInstanceCapacity; // how many instances it can currently hold.

	void UpdateVertexCapacity(); // after changing currentVertexCapacity, call to update vertices to the correct size.
	void UpdateIndicesCapacity(); // after changing currentIndicesCapacity, call to update indices to the correct size.
	void UpdateInstanceCapacity(); // after changing currentInstanceCapacity, call to update instances to the correct size.
	
	// needed for BufferedBuffer's double/triple buffering, call every frame AFTER writing vertex/instance data and BEFORE dispatching GPU draws/commands which use this meshpool.
	void CommitWrites();

	// needed for BufferedBuffer's double/triple buffering, call every frame BEFORE writing vertex/instance data and AFTER dispatching GPU draws/commands which use this meshpool.
	// Might yield if GPU isn't ready for us to write the data, so call at the last possible second.
	void FlipBuffers();

private:
	static IdProvider idProvider;
	static std::vector<Meshpool*> pools;
};

struct SlotSpace {
	unsigned first;
	unsigned count;
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

private:
	std::vector<unsigned> availableInstanceSlots;
	unsigned nextInstanceLocation = 0; // use if availableInstanceSlots is empty.

	//std::vector<SlotSpace> availableVertexSpace; // in terms of vertices

	std::unordered_map<Mesh*, unsigned> meshFirstVertexLocations; // index of the first vertex of each mesh 
	std::unordered_map<Mesh*, unsigned> meshFirstIndexLocations; // index of the first index of each mesh 

	unsigned nextMeshFirstVertexLocation = 0;
	unsigned nextMeshFirstIndexLocation = 0;
};
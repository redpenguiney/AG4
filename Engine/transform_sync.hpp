#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include "component.hpp"

using TransformSyncId = uint32_t;
class Player;
class Hierarchy;

// the actual data being synced.
struct TransformSyncData {
	glm::dvec3 pos;
	glm::quat rot; 
	glm::vec3 vel;
	glm::vec3 rotVel;
	TransformSyncId id;

	bool operator==(const TransformSyncData& other) const = default;
};

class TransformSync;

// Call the corresponding methods every frame to make TransformSync actually do something.
// Ensure you call TransformSyncManager::Get() before creating any TransformSyncs.
class TransformSyncManager {
public:
	static constexpr unsigned int PHYS_SYNC_DATA_CHANNEL = 30300;
	static constexpr unsigned int PHYS_SYNC_OWNERSHIP_CHANNEL = 30301;

	float SYNC_RATE = 0.05f; // send out physics transforms 20 times a second 
	bool QUANTIZE_STATE = false; // TODO UNIMPLEMENTED
	//float JITTER_BUFFER_DELAY = SYNC_RATE * 4; 
	size_t ALLOWED_SEND_RATE = 50000; // in BYTES (not bits) per second per connected player. Probably make higher for server.

	static TransformSyncManager& Get();

	void PrePhysics();
	//void PostPhysics();

private:
	friend class TransformSync;

	TransformSyncManager();

	std::vector<Connection> connections;

	// todo: best data structure? same probably goes for GuiElement and a few other systems iirc lol
	std::unordered_map<TransformSyncId, TransformSync*> synced;
public:
	~TransformSyncManager();
};

// intended to be used on dynamic Physobjects.
// Giving each Physobject its own TransformSyncId, and ensuring it is the same id as the one the remote client/host uses, is your responsibility. 
// By default the host authoritatively reports transforms to all players.
// With SetNetworkOwner() you can make some gameobject transforms owned by a client, and the server will relay whatever it sends to all other clients.
// Also see TransformSyncManager, whose methods you must call somewhere to make this component actually do something.
// TODO: client event for on network owner change
class TransformSync : public BaseComponent {
public:
	friend class TransformSyncManager;

	// higher = synced more often.
	// works if not network owner, though nothing will happen until ownership is transferred of course.
	void SetSyncPriority(float priority);

	// boosts the priority accumulator just once, so that the object will be higher priority for syncing until it is synced once. 
	// works if not network owner, though nothing will happen until ownership is transferred of course.
	void PrioritizeSyncOnce(float priorityBoost = INFINITY);

	// if a Hierarchy is provided, Transform() will be called to respond to sync changes. 
		// this will NOT sync the Hierarchy's children if their offsets change; in that case you would also need to add TransformSyncs to the children.
	TransformSync(Gameobject* obj, TransformSyncId id, Hierarchy* h = nullptr);
	~TransformSync();

	//void PrePhysics(float dt) override;
	//void PostPhysics(float dt) override;

	Player* GetNetworkOwner() const;

	// Server-only.
	void SetNetworkOwner(Player* to);

	struct SenderInfo {
		
		TransformSyncData lastSent;
	};

	struct RecieverInfo {

	};

	
private:
	// not inside SenderInfo so SetSyncPriority() and related functions can be called before ownership transfer message arrives over the network
	float priority = 1.0f; // higher means it will be synced more often, if we are the network owner.
	float priorityAccumulator = 1000000000.0f; // initially very high priority so owner ensures everyone else gets initial position at the very least

	Hierarchy* hierarchy;

	// solely changes gameobject transform/physobject velocities
	void ApplySyncData(const TransformSyncData data);

	TransformSyncData GetTransformSyncData();

	const TransformSyncId syncId;

	// always valid
	// on client, this will refer to the server if another client owns the object, since clients don't actually know about other clients and the server relays whatever the actual owner sent to this client.
	Player* owner; 

	// sender if owner, reciever if not
	union {
		SenderInfo sender;
		RecieverInfo reciever;
	};
	

	//float CalcCurrentPriority();
};
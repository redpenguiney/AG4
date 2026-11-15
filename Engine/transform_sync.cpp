#include "transform_sync.hpp"
#include "networking_engine.hpp"
#include "gameobject.hpp"
#include "serialization.hpp"
#include "hierachy_component.hpp"

template <>
inline size_t SerializedSize(const TransformSyncData& v) {
	return sizeof(v);
}

template <>
inline void Serialize(const TransformSyncData& val, uint8_t*& currentLocation) {
	Serialize(val.id, currentLocation);
	Serialize(val.pos, currentLocation);
	Serialize(val.rot, currentLocation);
	Serialize(val.rotVel, currentLocation);
	Serialize(val.vel, currentLocation);
}

template <>
inline TransformSyncData Deserialize(uint8_t*& currentLocation, size_t& bytesRemaining) {
	TransformSyncData out;
	out.id = Deserialize<TransformSyncId>(currentLocation, bytesRemaining);
	out.pos = Deserialize<glm::dvec3>(currentLocation, bytesRemaining);
	out.rot = Deserialize<glm::quat>(currentLocation, bytesRemaining);
	out.rotVel = Deserialize<glm::vec3>(currentLocation, bytesRemaining);
	out.vel = Deserialize<glm::vec3>(currentLocation, bytesRemaining);
	return out;
}

void TransformSync::SetSyncPriority(float p) {
	//if (NetworkingEngine::Get().GetLocalMachinePlayer().get() != owner) return;

	priority = p;
}

void TransformSync::PrioritizeSyncOnce(float priorityBoost) {
	//if (NetworkingEngine::Get().GetLocalMachinePlayer().get() != owner) return;
	
	priorityAccumulator += priorityBoost;
}

TransformSync::TransformSync(Gameobject* obj, TransformSyncId id, Hierarchy* h):
	BaseComponent(obj),
	syncId(id),
	hierarchy(h)
{
	Assert(!TransformSyncManager::Get().synced.contains(id));
	TransformSyncManager::Get().synced.emplace(id, this);
	owner = NetworkingEngine::Get().GetServerPlayer().get();
}

TransformSync::~TransformSync() {
	auto& map = TransformSyncManager::Get().synced;
	map.erase(syncId);
}

Player* TransformSync::GetNetworkOwner() const {
	return owner;
}

void TransformSync::SetNetworkOwner(Player* to) {
	Assert(NetworkingEngine::Get().IsHost());
	if (owner == to) return;
	bool serverOwns = to == NetworkingEngine::Get().GetServerPlayer().get();
	NetworkingEngine::Get().SendUserMessageToClient(to, TransformSyncManager::PHYS_SYNC_OWNERSHIP_CHANNEL, SendMessageParams(), serverOwns, syncId);

	owner = to;
	DebugLogInfo("Transferring ownership of ", syncId);
	// TODO: other action required? maybe fire event even though that'd be mostly for client anyways?
}

void TransformSync::ApplySyncData(const TransformSyncData data) {
	if (hierarchy) {
		hierarchy->Transform(data.pos, data.rot);
	}
	else {
		object->SetPosition(data.pos);
		object->SetRotation(data.rot);	
	}

	auto physobject = dynamic_cast<Physobject*>(object);
	if (physobject) {
		physobject->velocity = data.vel;
		physobject->rotVelocity = data.rotVel;
	}
}

TransformSyncData TransformSync::GetTransformSyncData() {
	auto physobject = dynamic_cast<Physobject*>(object);
	if (physobject) {
		return TransformSyncData{
			.pos = physobject->Position(),
			.rot = physobject->Rotation(),
			.vel = physobject->velocity,
			.rotVel = physobject->rotVelocity,
			.id = syncId
		};
	}
	else {
		return TransformSyncData{ // TODO: pointlessly sending vel/rot vel for regular gameobjects
			.pos = object->Position(),
			.rot = object->Rotation(),
			.vel = glm::vec3(0, 0, 0),
			.rotVel = glm::vec3(0, 0, 0),
			.id = syncId
		};
	}
}

//float TransformSync::CalcCurrentPriority() {
//	return priority;
//}

TransformSyncManager& TransformSyncManager::Get() {
	static TransformSyncManager tsm;
	return tsm;
}

void TransformSyncManager::PrePhysics() {
	auto serverPlayer = NetworkingEngine::Get().GetServerPlayer().get();
	auto localPlayer = NetworkingEngine::Get().GetLocalMachinePlayer().get();
	if (!localPlayer || !serverPlayer) return; 

	std::vector<TransformSync*> objectsToSend;
	for (auto& [id, sync] : synced) {
		if (sync->owner == nullptr) {
			sync->owner = serverPlayer;
		}

		if (sync->owner == localPlayer) { 
			// see if object has actually moved
			TransformSyncData toSend = sync->GetTransformSyncData();
			if (sync->sender.lastSent != toSend) {
				sync->priorityAccumulator += sync->priority;
				//sync->sender.lastSent = toSend; // actually sent after the partial sort
				objectsToSend.push_back(sync);
			}
		}
	}

	size_t maxObjectsWeCouldSync = static_cast<size_t>((ALLOWED_SEND_RATE / sizeof(TransformSyncData)) * SYNC_RATE);
	if (maxObjectsWeCouldSync > objectsToSend.size()) maxObjectsWeCouldSync = objectsToSend.size();
	std::partial_sort(objectsToSend.begin(), objectsToSend.begin() + maxObjectsWeCouldSync, objectsToSend.end(), [](const TransformSync* a, const TransformSync* b) -> bool {
		return a->priority > b->priority;
		});

	SendMessageParams sendParams;
	sendParams.reliable = false;
	sendParams.sendImmediately = false;
	for (auto& sync : objectsToSend) {
		TransformSyncData toSend = sync->GetTransformSyncData();
		if (localPlayer == serverPlayer) {
			NetworkingEngine::Get().SendUserMessageToAllClients(PHYS_SYNC_DATA_CHANNEL, sendParams, toSend);
			//DebugLogInfo("Synced transform.");
		}
		else {
			NetworkingEngine::Get().SendUserMessageToHost(PHYS_SYNC_DATA_CHANNEL, sendParams, toSend);
			//DebugLogInfo("ooooh we sending!");
		}
		sync->sender.lastSent = toSend;
	}

	//NetworkingEngine::Get().FlushUnsentPackets(); sad this function doesn't exist in gamenetworkingsockets (doesn't rly matter)
}

TransformSyncManager::TransformSyncManager() {
	connections.push_back(NetworkingEngine::Get().onNetworkStateChange.Connect([](NetworkingEngine*, NetworkState prior, NetworkState now) {
		
	}));

	connections.push_back(NetworkingEngine::Get().ConnectUserMessage<TransformSyncData>(PHYS_SYNC_DATA_CHANNEL, std::function<void(Player*, TransformSyncData)>([this](Player* fromWho, TransformSyncData data) {
		if (!synced.contains(data.id)) {
			DebugLogInfo("Remote attempt to sync id ", data.id, " which doesn't exist");
			return;
		}
		
		if (synced.at(data.id)->owner != fromWho) {
			DebugLogInfo("Remote attempt to illegally sync id ", data.id, " without ownership");
			return; // not a real object, or the player doesn't own the object they're sending data for
		}
		
		TransformSync* sync = synced.at(data.id);

		if (NetworkingEngine::Get().IsHost()) {
			//DebugLogInfo("Recieved sync data from client?");

			// sync server's transform 
			//sync->reciever.;  
			sync->ApplySyncData(data);
			// forward sync to other clients 

			for (auto& p : NetworkingEngine::Get().GetPlayers()) {
				if (!p->IsLocalMachine() && p.get() != fromWho) {
					SendMessageParams sendParams;
					sendParams.reliable = false;
					sendParams.sendImmediately = false;
					NetworkingEngine::Get().SendUserMessageToClient<TransformSyncData>(p.get(), PHYS_SYNC_DATA_CHANNEL, sendParams, data);
				}
			}
		}
		else {
			// sync client's transform
			//sync->reciever.;
			sync->ApplySyncData(data);
		}
	})));

	connections.push_back(NetworkingEngine::Get().ConnectUserMessage<bool, TransformSyncId>(PHYS_SYNC_OWNERSHIP_CHANNEL, std::function([this](Player* fromWho, bool serverOwnsNow, TransformSyncId id) {
		auto& NE = NetworkingEngine::Get();
		
		if (NE.IsHost()) return; // client doesn't get to say whether they own something!
		if (fromWho != NE.GetServerPlayer().get()) { // should be impossible
			DebugLogError("????"); 
			return;
		}
		if (!synced.contains(id)) { // we don't have whatever they wanted us to sync.
			DebugLogError("Unable to transfer ownership for id ", id, " because no such TransformSync exists. This is probably a user error.");
			return; 
		}
		TransformSync* sync = synced[id];
		Player* newOwner = serverOwnsNow ? fromWho : NE.GetLocalMachinePlayer().get();
		if (newOwner != sync->owner) {
			sync->owner = newOwner;
			DebugLogInfo("Recieved ownership transfer.");
			// TODO: fire event
			// TODO: any other action required?
		}
	})));
}

TransformSyncManager::~TransformSyncManager() {

}
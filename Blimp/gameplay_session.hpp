#pragma once
#include <vector>
#include <memory>
#include "body.hpp"
#include <event.hpp>
#include <utility.hpp>
#include <unordered_map>
#include <string>
#include <transform_sync.hpp>

class Body;
class Gameobject;
class Player;
class GameState;

struct PlayerInfo {
	int id = -1;
	TransformSyncId bodySyncId = 0;
	Player* player = nullptr; // will be nullptr if we're a client and this player isn't us or the server

	// only used on server; client gets player ids from server
	static inline int lastId = 0;

	std::unique_ptr<Body> body;

	std::string GetUsername();
};

// Stores all the gameplay stuff whose lifetime goes from when you hit host/join to when you shutdown/disconnect.
class GameplaySession {
public:
	GameplaySession(GameState* state);
	~GameplaySession();

	std::vector<std::shared_ptr<Gameobject>> staticObjects;
	std::vector<std::shared_ptr<Gameobject>> dynamicObjects;
	//std::vector<std::unique_ptr<Body>> bodies;

	std::shared_ptr<Camera> worldcam;

	// any connections you want to last for only the duration of the gameplay session should be stored here.
	std::vector<Connection> connections;

	std::unordered_map<int, PlayerInfo> players;

	// server only
	IdProvider transformSyncIdProvider;

	GameState* const state;

private:
	std::unique_ptr<Body> MakeLocalBody(TransformSyncId syncId);
	std::unique_ptr<Body> MakeRemoteBody(TransformSyncId syncId, Player* forWho);
};
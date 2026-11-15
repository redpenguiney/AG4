#include "game_state.hpp"
#include "debug_prefabs.hpp"
#include <window.hpp>
#include <mainloop.hpp>
#include "chat.hpp"
#include <transform_sync.hpp>

GameState::GameState() : chat()
{
}

void GameState::MakeGameplaySession() {
	currentSession = std::make_unique<GameplaySession>(this);

	chat.SetVisible(true);

	Mainloop::Get().physicsPaused = false;

	for (auto& obj : BuildPit({ 0, -10, 0 }, { 100, 10, 100 }, 0.2f, 1.0f)) {
		currentSession->staticObjects.emplace_back(obj);
	}

}

GameplaySession::GameplaySession(GameState* s) : state(s) {

	auto freecam = GetFreecam();	

	connections.push_back(Window::Get().postInputProccessing.Connect([this, freecam](Window*) {
		if (Window::Get().PRESS_BEGAN_KEYS.contains(InputType::F)) {
			if (!worldcam || GraphicsEngine::Get().currentCamera == worldcam) {
				GraphicsEngine::Get().currentCamera = freecam;
			}
			else {
				GraphicsEngine::Get().currentCamera = worldcam;
			}
		}
		else if (Window::Get().PRESS_BEGAN_KEYS.contains(InputType::Space)) {
			Mainloop::Get().physicsPaused = !Mainloop::Get().physicsPaused;
		}
		else if (Window::Get().PRESS_BEGAN_KEYS.contains(InputType::P)) {
			//DebugLogInfo("Sending " + std::to_string(NetworkingEngine::Get().IsClient()));
			//chat.SendMessage("Hello, world! client=" + std::to_string(NetworkingEngine::Get().IsClient()));
		}
		}));

	connections.push_back(Mainloop::Get().prePhysics.Connect([this](Mainloop*, float dt) {
		TransformSyncManager::Get().PrePhysics();
		}));

	constexpr unsigned DECLARE_PLAYER_CHANNEL = 1000000;
	auto& NE = NetworkingEngine::Get();

	// 
	if (NE.IsHost()) {
		// declare PlayerInfo for the local machine
		int localPlayerId = PlayerInfo::lastId++;
		PlayerInfo localPlayerInfo;
		localPlayerInfo.id = localPlayerId;
		localPlayerInfo.bodySyncId = transformSyncIdProvider.GetId();
		localPlayerInfo.player = NE.GetLocalMachinePlayer().get();
		localPlayerInfo.body = MakeLocalBody(localPlayerInfo.bodySyncId);
		players.emplace(localPlayerId, std::move(localPlayerInfo));


		auto declarePlayers = [&NE, localPlayerId, this]() {
			// get all players on the same page regarding who is online
			auto sendParams = SendMessageParams();

			for (auto& [_, playerInfo] : players) {
				if (playerInfo.player == NE.GetLocalMachinePlayer().get()) continue;

				std::vector<int> playerIds;
				std::vector<TransformSyncId> playerBodySyncIds;
				playerIds.push_back(playerInfo.id); // tell the client their id
				playerBodySyncIds.push_back(playerInfo.bodySyncId);
				playerIds.push_back(localPlayerId); // tell the client the server's id
				playerBodySyncIds.push_back(players[localPlayerId].bodySyncId);

				// tell the client about all other clients
				for (auto& [_, otherPlayerInfo] : players) {
					if (playerInfo.id != otherPlayerInfo.id && otherPlayerInfo.id != localPlayerId) {
						playerIds.push_back(otherPlayerInfo.id);
						playerBodySyncIds.push_back(otherPlayerInfo.bodySyncId);
					}
				}

				NE.SendUserMessageToClient(playerInfo.player, DECLARE_PLAYER_CHANNEL, sendParams, playerIds, playerBodySyncIds);
			}
		};
		
		connections.push_back(NE.onPlayerAdded.Connect([this, declarePlayers](NetworkingEngine*, Player* newPlayer) {
			int playerId = PlayerInfo::lastId++;
			PlayerInfo newInfo;
			newInfo.id = playerId;
			newInfo.bodySyncId = transformSyncIdProvider.GetId();
			newInfo.player = newPlayer;
			players.emplace(playerId, std::move(newInfo));
			state->chat.LogMessage(players[playerId].GetUsername() + " has connected.");

			declarePlayers();
			players[playerId].body = MakeRemoteBody(players[playerId].bodySyncId, newPlayer);
		}));
		connections.push_back(NE.onPlayerRemoved.Connect([this, declarePlayers](NetworkingEngine*, Player* playerThatLeft) {
			int leftId = -1;
			for (auto& [id, playerInfo] : players) {
				if (playerInfo.player == playerThatLeft) {
					leftId = id;
					break;
				}
			}
			if (leftId == -1) {
				DebugLogError("Goofy player left???!??!");
				return;
			}

			state->chat.LogMessage(players[leftId].GetUsername() + " has disconnected.");
			transformSyncIdProvider.ReturnId(players[leftId].bodySyncId);
			players.erase(leftId);
			declarePlayers();
		}));
	}
	else if (NE.IsClient()) {
		connections.push_back(NE.ConnectUserMessage<std::vector<int>, std::vector<TransformSyncId>>(DECLARE_PLAYER_CHANNEL, std::function([&NE, this](Player* sender, std::vector<int> ids, std::vector<TransformSyncId> syncIds) {
			std::unordered_set<int>  playersThatAreGoneNow;
			for (auto& [i, _] : players) playersThatAreGoneNow.insert(i);

			//players[ids.at(0)] = PlayerInfo{ .id = ids.at(0), .player = NE.GetLocalMachinePlayer().get() };
			//players[ids.at(1)] = PlayerInfo{ .id = ids.at(1), .player = NE.GetServerPlayer().get() };
			for (int i = 0; i < ids.size(); i++) {
				playersThatAreGoneNow.erase(i);
				if (!players.contains(ids[i])) {
					PlayerInfo info;
					info.id = ids[i];
					info.player = nullptr;
					if (i == 0) {
						info.player = NE.GetLocalMachinePlayer().get();
						info.body = MakeLocalBody(syncIds[i]);
					}
					else if (i == 1) info.player = NE.GetServerPlayer().get();

					if (i != 0) {
						info.body = MakeRemoteBody(syncIds[i], players[ids[i]].player);
					}

					players[info.id] = std::move(info);
					state->chat.LogMessage(players[ids[i]].GetUsername() + " has connected.");
				}
			}

			for (int id : playersThatAreGoneNow) {
				state->chat.LogMessage(players[id].GetUsername() + " has disconnected.");
				players.erase(id);
			}
		})));
	}
}

GameplaySession::~GameplaySession() {
	DebugLogInfo("Killing game session.");
}

std::unique_ptr<Body> GameplaySession::MakeLocalBody(TransformSyncId syncId) {
	DebugLogInfo("Local body ", syncId, ": server = ", NetworkingEngine::Get().IsHost());
	BodyCreateParams bp;
	bp.syncId = syncId;
	auto localController = std::make_unique<LocalPlayerController>();
	worldcam = localController->camera;
	GraphicsEngine::Get().currentCamera = worldcam;
	auto b = new Body(std::move(localController), bp);
	//b->SetPos({ 0, 8, 0 });
	return std::unique_ptr<Body>(b);
}

std::unique_ptr<Body> GameplaySession::MakeRemoteBody(TransformSyncId syncId, Player* forWho) {
	DebugLogInfo("Remote body ", syncId, ": server = ", NetworkingEngine::Get().IsHost());
	BodyCreateParams bp;
	bp.syncId = syncId;
	auto remoteController = std::make_unique<RemotePlayerController>(forWho);
	auto b = new Body(std::move(remoteController), bp);
	//b->SetPos({ 5, 8, 0 });
	return std::unique_ptr<Body>(b);
}

std::string PlayerInfo::GetUsername() {
	return "Player_" + std::to_string(id);
}

#include "networking_engine.hpp"
#include <GameNetworkingSockets/steam/isteamnetworkingutils.h>

struct ConnectionInfo {
	HSteamNetConnection connection;
};

void SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo) {

}

void SteamDebugCallback(ESteamNetworkingSocketsDebugOutputType eDebugOutputType, const char* pszMsg) {
    DebugLogInfo("STEAM DEBUG: ", pszMsg);
}

const NetworkState NetworkingEngine::GetState() const {
    return state;
}

const std::vector<Player>& NetworkingEngine::GetPlayers() const {
    return players;
}

NetworkingEngine& NetworkingEngine::Get() {
    static NetworkingEngine instance;
    return instance;
}

void NetworkingEngine::Update() {
    if (state == NetworkState::Offline) return;
    if (std::holds_alternative<std::unique_ptr<ServerNetworkInfo>>(stateData)) {
        SteamNetworkingSockets()->RunCallbacks();

		auto& serverInfo = std::get<std::unique_ptr<ServerNetworkInfo>>(stateData);
		SteamNetworkingMessage_t* message = nullptr;
        for (Player& p : players) {
            if (!p.connectionInfo) {
				continue; // we aren't connected to this player (either because its the local machine or because we're the client and they're a fellow non-server client)
            }
            while (true) {
                int result = SteamNetworkingSockets()->ReceiveMessagesOnConnection(p.connectionInfo->connection, &message, 1);
                if (result == 0) break;
                if (result == -1) {
                    DebugLogError("YO WHAT WHY -1");
                    break;
                }
				HandleRecievedMessage(p, message);
            }
        }
    }
    else if (std::holds_alternative<std::unique_ptr<ClientNetworkInfo>>(stateData)) {
        SteamNetworkingSockets()->RunCallbacks();
    }
    else {
        Assert(false);
        std::unreachable();
    }
}

void NetworkingEngine::Host(unsigned port) {
    Assert(state == NetworkState::Offline);
    stateData = std::make_unique<ServerNetworkInfo>(port);


    SetState(NetworkState::Server);
}

void NetworkingEngine::TryJoin() {
    Assert(state == NetworkState::Offline);
	stateData = std::make_unique<ClientNetworkInfo>();

    SetState(NetworkState::ClientConnecting);
}

NetworkingEngine::~NetworkingEngine() {
    stateData = std::monostate{};
    GameNetworkingSockets_Kill();
}

void NetworkingEngine::SetState(NetworkState newState) {
    if (state != newState) {
        auto priorState = state;
        state = newState;
		onNetworkStateChange.Fire(this, priorState, newState);
    }
}

NetworkingEngine::NetworkingEngine() {
    SteamDatagramErrMsg err;
    if (!GameNetworkingSockets_Init(nullptr, err)) {
		DebugLogError("Failed to initialize GameNetworkingSockets: ", err);
        Assert(false);
        std::unreachable();
    }

    SteamNetworkingUtils()->SetDebugOutputFunction(k_ESteamNetworkingSocketsDebugOutputType_Debug, SteamDebugCallback);
	SteamNetworkingUtils()->SetGlobalCallback_SteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback);

    players.push_back(Player());
}

void NetworkingEngine::HandleRecievedMessage(Player& sender, SteamNetworkingMessage_t* message) {
    uint8_t* currentPacketPosition = reinterpret_cast<uint8_t*>(message->m_pData);
	int packetSize = message->m_cbSize;

    Assert(packetSize > 0);


    message->Release();
}

ServerNetworkInfo::ServerNetworkInfo(unsigned port) {
    SteamNetworkingIPAddr localaddr;
    localaddr.Clear();
    localaddr.m_port = port;
    listenSocket = SteamNetworkingSockets()->CreateListenSocketIP(localaddr, 0, nullptr);
}

ServerNetworkInfo::~ServerNetworkInfo() {
	SteamNetworkingSockets()->CloseListenSocket(listenSocket);
}

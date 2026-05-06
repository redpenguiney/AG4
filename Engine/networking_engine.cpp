#include "networking_engine.hpp"
#include <GameNetworkingSockets/steam/isteamnetworkingutils.h>

void SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo) {

}

void SteamDebugCallback(ESteamNetworkingSocketsDebugOutputType eDebugOutputType, const char* pszMsg) {
    DebugLogInfo("STEAM DEBUG: ", pszMsg);
}

const NetworkState NetworkingEngine::GetState() const {
    return state;
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
        for (auto& connection : TODO) {
            while (true) {
                int result = SteamNetworkingSockets()->ReceiveMessagesOnConnection(connection, &message, 1);
                if (result == 0) break;
                if (result == -1) {
                    DebugLogError("YO WHAT WHY -1");
                    break;
                }

                
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

NetworkingEngine::~NetworkingEngine() {
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

    SteamNetworkingUtils()->SetDebugOutputFunction(k_ESteamNetworkingSocketsDebugOutputType_Warning, SteamDebugCallback);
	SteamNetworkingUtils()->SetGlobalCallback_SteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback);

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

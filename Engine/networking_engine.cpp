#include "networking_engine.hpp"



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

    }
    else if (std::holds_alternative<std::unique_ptr<ClientNetworkInfo>>(stateData)) {

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

}

void NetworkingEngine::SetState(NetworkState newState) {
    if (state != newState) {
        auto priorState = state;
        state = newState;
		onNetworkStateChange.Fire(this, priorState, newState);
    }
}

NetworkingEngine::NetworkingEngine() {

}

ServerNetworkInfo::ServerNetworkInfo(unsigned port) {
    SteamNetworkingIPAddr localaddr;
    localaddr.Clear();
    localaddr.m_port = port;
    listenSocket = SteamNetworkingSockets()->CreateListenSocketIP(localaddr, 0, nullptr);
}

ServerNetworkInfo::~ServerNetworkInfo() {

}

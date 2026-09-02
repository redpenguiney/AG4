#include "networking_engine.hpp"
#include <GameNetworkingSockets/steam/isteamnetworkingutils.h>

enum SERVER_CLOSE_REASONS {
    UNKNOWN = 0,
    SERVER_SHUTDOWN = 1,

};

void Client::HandleRecievedMessage(SteamNetworkingMessage_t* message) {
    uint8_t* currentPacketPosition = reinterpret_cast<uint8_t*>(message->m_pData);
    int bytesRemaining = message->m_cbSize;
    Assert(bytesRemaining > 0);

    PacketType pktType = (PacketType)Deserialize<uint8_t>(currentPacketPosition, bytesRemaining);

    switch (pktType) {
    case PacketType::UserMessage:
        NetworkingEngine::Get().HandleUserMessage(currentPacketPosition, bytesRemaining);
        break;
    }

    message->Release();
}

void Client::SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo) {
    Assert(NetworkingEngine::Get().IsClient());
    auto& client = *std::get<std::unique_ptr<Client>>(NetworkingEngine::Get().stateData);
    if (pInfo->m_info.m_eState == ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_Connected) {
        DebugLogInfo("Connected!");
        NetworkingEngine::Get().players.clear();
        NetworkingEngine::Get().players.emplace_back(new Player());
        NetworkingEngine::Get().players.emplace_back(new Player(client.connection->connection));
        NetworkingEngine::Get().SetState(NetworkState::Client);
    }
    else if (pInfo->m_info.m_eState == ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_ProblemDetectedLocally) {
		//DebugLogError("Local connection problem: ", pInfo->m_info.m_szEndDebug);
		if (NetworkingEngine::Get().state == NetworkState::ClientConnecting) {
            NetworkingEngine::Get().onConnectionAttemptFailure.Fire(&NetworkingEngine::Get(), ConnectionFailureReason::Unknown, pInfo->m_info.m_szEndDebug);
        }
        NetworkingEngine::Get().stateData = std::monostate();
        NetworkingEngine::Get().SetState(NetworkState::Offline);
    }
    else if (pInfo->m_info.m_eState == ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_ClosedByPeer) {
        DebugLogInfo("Server kicked us out?");
        if (NetworkingEngine::Get().state == NetworkState::ClientConnecting) {
            NetworkingEngine::Get().onConnectionAttemptFailure.Fire(&NetworkingEngine::Get(), ConnectionFailureReason::Unknown, pInfo->m_info.m_szEndDebug);
        }
        NetworkingEngine::Get().stateData = std::monostate();
        NetworkingEngine::Get().SetState(NetworkState::Offline);
    }
    else {
        DebugLogError("Client: Anomalous connection state ", pInfo->m_info.m_eState, " was ", pInfo->m_eOldState);
    }
}

void Server::SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo) {
    DebugLogInfo("We're in.");

    Assert(NetworkingEngine::Get().IsHost());
    auto& server = *std::get<std::unique_ptr<Server>>(NetworkingEngine::Get().stateData);
    if (pInfo->m_info.m_eState == ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_Connecting) {
        SteamNetworkingSockets()->AcceptConnection(pInfo->m_hConn);
        auto info = new ConnectionInfo(pInfo->m_hConn);
        NetworkingEngine::Get().players.emplace_back(new Player(info->connection));
        server.connections.emplace_back(info);

        NetworkingEngine::Get().onPlayerAdded.FireNow(&NetworkingEngine::Get(), NetworkingEngine::Get().players.back().get());
    }
    else if (pInfo->m_info.m_eState == ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_Connected) {
        return; // we're connected, no action required
    }
    else if (pInfo->m_info.m_eState == ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_ClosedByPeer) {
        DebugLogError("Client connection closed by client.");

        Player* playerThatLeft = nullptr;
        unsigned connectionI = 0;
        for (auto& c : server.connections) {
            if (c->connection == pInfo->m_hConn) {
                playerThatLeft = 
                break;
            }
            connectionI++;
        }

        NetworkingEngine::Get().onPlayerRemoved.FireNow(&NetworkingEngine::Get(), playerThatLeft);

        Assert(false);
    }
    else if (pInfo->m_info.m_eState == ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_Dead) {
        DebugLogError("DEAD CONNECTOIN TO CLIENT");
        Assert(false);
    }
    else {
        DebugLogError("Server: Anomalous connection state ", pInfo->m_info.m_eState, " was ", pInfo->m_eOldState);
    }
}

void SteamDebugCallback(ESteamNetworkingSocketsDebugOutputType eDebugOutputType, const char* pszMsg) {
    DebugLogInfo("STEAM DEBUG: ", pszMsg);
}

const NetworkState NetworkingEngine::GetState() const {
    return state;
}

const std::vector<std::shared_ptr<Player>>& NetworkingEngine::GetPlayers() const
{
    Assert(state == NetworkState::Client || state == NetworkState::Server || state == NetworkState::ServerShuttingDown);
    return players;
}

const std::shared_ptr<Player> NetworkingEngine::GetLocalMachinePlayer() const
{
    Assert(state == NetworkState::Client || state == NetworkState::Server || state == NetworkState::ServerShuttingDown);
    Assert(players.size() > 0);
    Assert(playerConnections.contains(players[0].get()) == false);
    return players[0];
}

NetworkingEngine& NetworkingEngine::Get() {
    static NetworkingEngine instance;
    return instance;
}

void NetworkingEngine::Update() {
    if (state == NetworkState::Offline) return;
    if (std::holds_alternative<std::unique_ptr<Server>>(stateData)) {

		auto& serverInfo = std::get<std::unique_ptr<Server>>(stateData);
        serverInfo->UpdateServer();
		
    }
    else if (std::holds_alternative<std::unique_ptr<Client>>(stateData)) {
    
        auto& clientInfo = std::get<std::unique_ptr<Client>>(stateData);
        clientInfo->UpdateClient();

    }
    else {
        Assert(false);
        //std::unreachable();
    }
}

bool NetworkingEngine::IsHost() {
    return state == NetworkState::Server || state == NetworkState::ServerShuttingDown;
}

bool NetworkingEngine::IsClient() {
    return state == NetworkState::Client || state == NetworkState::ClientConnecting || state == NetworkState::ClientDisconnecting;
}

bool NetworkingEngine::IsReadyClient()
{
    return state == NetworkState::Client;
}

void NetworkingEngine::Host(HostServerParams params) {
    Assert(state == NetworkState::Offline);
    stateData = std::make_unique<Server>(params);

    players.clear();
    players.emplace_back(new Player());

    SetState(NetworkState::Server);
}

void NetworkingEngine::ShutdownServer() {
    if (state == NetworkState::ServerShuttingDown) return;
    Assert(state == NetworkState::Server);
    
    auto& serverInfo = std::get<std::unique_ptr<Server>>(stateData);
    serverInfo->ShutdownServer();

	SetState(NetworkState::ServerShuttingDown);
}

void NetworkingEngine::Disconnect() {
    if (state == NetworkState::ClientDisconnecting) return;
    Assert(state == NetworkState::Client);

    auto& clientInfo = std::get<std::unique_ptr<Client>>(stateData);
    clientInfo->Disconnect();

    SetState(NetworkState::ClientDisconnecting);
}

void NetworkingEngine::TryJoin(ConnectionAttemptParams params) {
    Assert(state == NetworkState::Offline);
	stateData = std::make_unique<Client>(params);


    SetState(NetworkState::ClientConnecting);
}

void NetworkingEngine::CancelJoin() {
    Assert(state == NetworkState::ClientConnecting);
    stateData = std::monostate{};
	SetState(NetworkState::Offline);
}

NetworkingEngine::~NetworkingEngine() {
    stateData = std::monostate{};
    GameNetworkingSockets_Kill();
}

void NetworkingEngine::SetState(NetworkState newState) {
    if (state != newState) {
        auto priorState = state;
        state = newState;

        if (state == NetworkState::Offline) {
            players.clear();
        }
		onNetworkStateChange.Fire(this, priorState, newState);
    }
}

NetworkingEngine::NetworkingEngine() {
    SteamDatagramErrMsg err;
    if (!GameNetworkingSockets_Init(nullptr, err)) {
		DebugLogError("Failed to initialize GameNetworkingSockets: ", err);
        Assert(false);
        //std::unreachable();
    }

    SteamNetworkingUtils()->SetDebugOutputFunction(k_ESteamNetworkingSocketsDebugOutputType_Debug, SteamDebugCallback);
    SteamNetworkingUtils()->SetGlobalCallback_SteamNetConnectionStatusChanged(NetworkingEngine::SteamNetConnectionStatusChangedCallback);
}

void NetworkingEngine::SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo) {
    if (std::holds_alternative<std::unique_ptr<Client>>(Get().stateData)) {
        Client::SteamNetConnectionStatusChangedCallback(pInfo);
    }
    else if (std::holds_alternative<std::unique_ptr<Server>>(Get().stateData)) {
        Server::SteamNetConnectionStatusChangedCallback(pInfo);
    } else {
        DebugLogError("Missing state yet connection callback fired???");
        Assert(false);
    }
}

void NetworkingEngine::HandleUserMessage(uint8_t* data, int size) {
    if (size < sizeof(unsigned int)) return;

    unsigned int channel = Deserialize<unsigned int>(data, size);
    if (userEventChannels.contains(channel)) {
        for (auto& f : userEventChannels[channel].connected) { // TODO: deserialization of entire message should happen here 
            f(data, size);
        }
    }
}

Server::Server(HostServerParams params) {
    SteamNetworkingIPAddr localaddr;
    localaddr.Clear();
    localaddr.m_port = params.port;

    //SteamNetworkingUtils()->Set

    std::vector<SteamNetworkingConfigValue_t> options;
    //SteamNetworkingConfigValue_t connectionStatusCallback;
    //connectionStatusCallback.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)Server::SteamNetConnectionStatusChangedCallback);
    //options.push_back(connectionStatusCallback);
    listenSocket = SteamNetworkingSockets()->CreateListenSocketIP(localaddr, options.size(), options.data());
}

Server::~Server() {
	SteamNetworkingSockets()->CloseListenSocket(listenSocket);
}

void Server::UpdateServer() {
    SteamNetworkingSockets()->RunCallbacks();
    auto& NE = NetworkingEngine::Get();

    // recieve messages
    SteamNetworkingMessage_t* message = nullptr;
    for (auto& p : connections) {
        Assert(p);
        while (true) {
            int result = SteamNetworkingSockets()->ReceiveMessagesOnConnection(p->connection, &message, 1);
            if (result == 0) break;
            if (result == -1) {
                DebugLogError("YO WHAT WHY -1");
                break;
            }
            
            HandleRecievedMessage(message);
        }
    }
}

void Server::ShutdownServer() {
    for (auto& conn : connections) {
        SteamNetworkingSockets()->CloseConnection(conn->connection, SERVER_SHUTDOWN, nullptr, false);
    }
}

void Server::HandleRecievedMessage(SteamNetworkingMessage_t* message) {
    uint8_t* currentPacketPosition = reinterpret_cast<uint8_t*>(message->m_pData);
    int bytesRemaining = message->m_cbSize;
    Assert(bytesRemaining > 0);

    PacketType pktType = (PacketType)Deserialize<uint8_t>(currentPacketPosition, bytesRemaining);

    switch (pktType) {
    case PacketType::UserMessage:
        NetworkingEngine::Get().HandleUserMessage(currentPacketPosition, bytesRemaining);
        break;
    }

    message->Release();
}

Client::Client(ConnectionAttemptParams params) {
    SteamNetworkingIPAddr address;
    address.ParseString(params.ip.c_str());

    std::vector<SteamNetworkingConfigValue_t> options;
    //SteamNetworkingConfigValue_t connectionStatusCallback;
	//connectionStatusCallback.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)Client::SteamNetConnectionStatusChangedCallback);
	//options.push_back(connectionStatusCallback);
    HSteamNetConnection conn = SteamNetworkingSockets()->ConnectByIPAddress(address, options.size(), options.data());
    connection = std::unique_ptr<ConnectionInfo>(new ConnectionInfo(conn));
}

Client::~Client() {

}

void Client::UpdateClient() {
    SteamNetworkingSockets()->RunCallbacks();


}

void Client::Disconnect() {
    SteamNetworkingSockets()->CloseConnection(connection->connection, 0, nullptr, false);
}

ConnectionInfo::ConnectionInfo(HSteamNetConnection conn): connection(conn) { 

}

ConnectionInfo::~ConnectionInfo() {
    SteamNetworkingSockets()->CloseConnection(connection, 0, nullptr, false);
}

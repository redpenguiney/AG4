#include "networking_engine.hpp"
#include <GameNetworkingSockets/steam/isteamnetworkingutils.h>

enum SERVER_CLOSE_REASONS {
    UNKNOWN = 0,
    SERVER_SHUTDOWN = 1,

};

void Client::HandleRecievedMessage(SteamNetworkingMessage_t* message) {
    uint8_t* currentPacketPosition = reinterpret_cast<uint8_t*>(message->m_pData);
    size_t bytesRemaining = message->m_cbSize;
    Assert(bytesRemaining > 0);

    PacketType pktType = (PacketType)Deserialize<uint8_t>(currentPacketPosition, bytesRemaining);
    //DebugLogInfo("RECIEVED MESSAGE from server: pkt type ", (uint8_t)pktType);

	Player* sender = connection->connectee;
    Assert(sender);

    switch (pktType) {
    case PacketType::UserMessage:
        NetworkingEngine::Get().HandleUserMessage(sender, currentPacketPosition, bytesRemaining);
        break;
    }

    message->Release();
}

void Client::SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo) {
    Assert(NetworkingEngine::Get().IsClient());
    auto& client = *std::get<std::unique_ptr<Client>>(NetworkingEngine::Get().stateData);

    if (pInfo->m_info.m_eState == ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_Connected) {
        DebugLogInfo("Connected!");
        Assert(NetworkingEngine::Get().players.empty());
        NetworkingEngine::Get().players.clear();
        auto local = NetworkingEngine::Get().players.emplace_back(new Player());
        auto server = NetworkingEngine::Get().players.emplace_back(new Player());
		NetworkingEngine::Get().playerConnections[server.get()] = client.connection.get();
        client.connection->connectee = server.get();
        NetworkingEngine::Get().SetState(NetworkState::Client);

        
    }
    else if (pInfo->m_info.m_eState == ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_ProblemDetectedLocally) {
		DebugLogError("Client: Local connection problem: ", pInfo->m_info.m_szEndDebug);

        // TODO: retrieve remaining messages from connection before destroying

		if (NetworkingEngine::Get().state == NetworkState::ClientConnecting) {
            NetworkingEngine::Get().onConnectionAttemptFailure.FireNow(&NetworkingEngine::Get(), ConnectionFailureReason::Unknown, pInfo->m_info.m_szEndDebug);
        }

        // either server died or we died, so kill the client; there are no other connections so unlike server we just go offline
        NetworkingEngine::Get().stateData = std::monostate();

        NetworkingEngine::Get().SetState(NetworkState::Offline);
        NetworkingEngine::Get().players.clear();
        NetworkingEngine::Get().playerConnections.clear();
    }
    else if (pInfo->m_info.m_eState == ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_ClosedByPeer) {
        DebugLogInfo("Server kicked us out?");
        if (NetworkingEngine::Get().state == NetworkState::ClientConnecting) {
            NetworkingEngine::Get().onConnectionAttemptFailure.FireNow(&NetworkingEngine::Get(), ConnectionFailureReason::Unknown, pInfo->m_info.m_szEndDebug);
        }

        // kill client
        NetworkingEngine::Get().stateData = std::monostate();

        NetworkingEngine::Get().SetState(NetworkState::Offline);
        NetworkingEngine::Get().players.clear();
        NetworkingEngine::Get().playerConnections.clear();
    }
    else if (pInfo->m_info.m_eState == ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_Connecting) {
		DebugLogInfo("Client: CONNECTION STATE CHANGE: Connecting to server...");
        // we don't actually care, we already knew we were trying to connect
    }
    else if (pInfo->m_info.m_eState == ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_FindingRoute) {
        DebugLogInfo("Client: connection state is finding route??");
        // keep waiting 
    }
    else {
        DebugLogError("Client: Anomalous connection state ", pInfo->m_info.m_eState, " was ", pInfo->m_eOldState);
    }
}

void Server::SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo) {

    Assert(NetworkingEngine::Get().IsHost());
    auto& server = *std::get<std::unique_ptr<Server>>(NetworkingEngine::Get().stateData);

    if (pInfo->m_info.m_eState == ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_Connecting) {
        // then a player has connected to us.
        SteamNetworkingSockets()->AcceptConnection(pInfo->m_hConn);
        auto info = new ConnectionInfo(pInfo->m_hConn);
        auto newPlr = NetworkingEngine::Get().players.emplace_back(new Player());
        info->connectee = newPlr.get();
        NetworkingEngine::Get().playerConnections[newPlr.get()] = info;
        server.connections.emplace_back(info);
        NetworkingEngine::Get().onPlayerAdded.FireNow(&NetworkingEngine::Get(), NetworkingEngine::Get().players.back().get());
    }
    else if (pInfo->m_info.m_eState == ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_Connected) {
        DebugLogInfo("Server: remote client has fully connected to us.");

        return; // we already connected to the client successfully, no action required
        // todo: are we sure?
    }
    else if (pInfo->m_info.m_eState == ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_ClosedByPeer) {
        DebugLogError("Client connection closed by client.");

        Player* playerThatLeft = nullptr;
        unsigned connectionI = 0;
        for (auto& c : server.connections) {
            if (c->connection == pInfo->m_hConn) {
                playerThatLeft = c->connectee;
                break;
            }
            connectionI++;
        }

        NetworkingEngine::Get().onPlayerRemoved.FireNow(&NetworkingEngine::Get(), playerThatLeft);

        // TODO: remaining messages to grab from connection before deletion

        server.connections[connectionI] = std::move(server.connections.back());
        server.connections.pop_back();
        NetworkingEngine::Get().playerConnections.erase(playerThatLeft);
        std::erase_if(NetworkingEngine::Get().players, [playerThatLeft](const std::shared_ptr<Player>& p) { return p.get() == playerThatLeft; });

    }
    else if (pInfo->m_info.m_eState == ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_ProblemDetectedLocally) {
        DebugLogInfo("Server: local connection problem:", pInfo->m_info.m_szEndDebug);
        
        // TODO: remaining messages to grab from connection before deletion

        // Despite the name, the problem probably isn't on the server (unless server lost internet or something), but more likely the client crashed/timed out.
        // Delete the client just like if they closed it deliberately.

        Player* playerThatLeft = nullptr;
        unsigned connectionI = 0;
        for (auto& c : server.connections) {
            if (c->connection == pInfo->m_hConn) {
                playerThatLeft = c->connectee;
                break;
            }
            connectionI++;
        }

        NetworkingEngine::Get().onPlayerRemoved.FireNow(&NetworkingEngine::Get(), playerThatLeft);

        // TODO: remaining messages to grab from connection before deletion

        server.connections[connectionI] = std::move(server.connections.back());
        server.connections.pop_back();
        NetworkingEngine::Get().playerConnections.erase(playerThatLeft);
        std::erase_if(NetworkingEngine::Get().players, [playerThatLeft](const std::shared_ptr<Player>& p) { return p.get() == playerThatLeft; });

        // kill server 
        //NetworkingEngine::Get().stateData = std::monostate();

        //NetworkingEngine::Get().SetState(NetworkState::Offline);
        //NetworkingEngine::Get().players.clear();
        //NetworkingEngine::Get().playerConnections.clear();
    }
	else if (pInfo->m_info.m_eState == ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_FindingRoute) {
		DebugLogInfo("Server: connection state is finding route??");
		// keep waiting
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
	if (players.size() == 0) return nullptr;
    Assert(playerConnections.contains(players[0].get()) == false);
    return players[0];
}

const std::shared_ptr<Player> NetworkingEngine::GetServerPlayer() const {
    if (players.size() == 0) return nullptr;
    if (IsHost()) return GetLocalMachinePlayer();
    else if (IsClient() && players.size() > 1) {
        return players[1];
    }
    return nullptr;
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

bool NetworkingEngine::IsHost() const {
    return state == NetworkState::Server || state == NetworkState::ServerShuttingDown;
}

bool NetworkingEngine::IsClient() const {
    return state == NetworkState::Client || state == NetworkState::ClientConnecting || state == NetworkState::ClientDisconnecting;
}

bool NetworkingEngine::IsReadyClient() const {
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
		onNetworkStateChange.FireNow(this, priorState, newState);
    }
}

NetworkingEngine::NetworkingEngine() {
    SteamDatagramErrMsg err;
    if (!GameNetworkingSockets_Init(nullptr, err)) {
		DebugLogError("Failed to initialize GameNetworkingSockets: ", err);
        Assert(false);
        //std::unreachable();
    }

    SteamNetworkingUtils()->SetDebugOutputFunction(k_ESteamNetworkingSocketsDebugOutputType_Msg, SteamDebugCallback);
    //SteamNetworkingUtils()->SetDebugOutputFunction(k_ESteamNetworkingSocketsDebugOutputType_Debug, SteamDebugCallback);
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

void NetworkingEngine::HandleUserMessage(Player* fromWho, uint8_t* data, size_t size) {
    if (size < sizeof(unsigned int)) return;

    unsigned int channel = Deserialize<unsigned int>(data, size);
    //DebugLogInfo("RECIEVED USER MESSAGE ON CHANNEL ", channel);
    if (userEventChannels.contains(channel)) {
        for (auto& f : userEventChannels[channel].connected) { // TODO: deserialization of entire message should happen here 
            f.second(fromWho, data, size);
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
    listenSocket = SteamNetworkingSockets()->CreateListenSocketIP(localaddr, static_cast<int>(options.size()), options.data());
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
			Assert(p->connectee);
            HandleRecievedMessage(message, p->connectee);
        }
    }
}

void Server::ShutdownServer() {
    for (auto& conn : connections) {
        SteamNetworkingSockets()->CloseConnection(conn->connection, SERVER_SHUTDOWN, nullptr, false);
    }
}

void Server::HandleRecievedMessage(SteamNetworkingMessage_t* message, Player* sender) {
    uint8_t* currentPacketPosition = reinterpret_cast<uint8_t*>(message->m_pData);
    size_t bytesRemaining = message->m_cbSize;
    Assert(bytesRemaining > 0);

    PacketType pktType = (PacketType)Deserialize<uint8_t>(currentPacketPosition, bytesRemaining);

    switch (pktType) {
    case PacketType::UserMessage:
        NetworkingEngine::Get().HandleUserMessage(sender, currentPacketPosition, bytesRemaining);
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
    HSteamNetConnection conn = SteamNetworkingSockets()->ConnectByIPAddress(address, static_cast<int>(options.size()), options.data());
    connection = std::unique_ptr<ConnectionInfo>(new ConnectionInfo(conn));
}

Client::~Client() {

}

void Client::UpdateClient() {
    SteamNetworkingSockets()->RunCallbacks();

    // recieve messages
    SteamNetworkingMessage_t* message = nullptr;
    while (true) {
        int result = SteamNetworkingSockets()->ReceiveMessagesOnConnection(connection->connection, &message, 1);
        if (result == 0) break;
        if (result == -1) {
            DebugLogError("YO WHAT WHY -1");
            break;
        }
        Assert(connection->connectee);
        HandleRecievedMessage(message);
    }
}

void Client::Disconnect() {
    SteamNetworkingSockets()->CloseConnection(connection->connection, 0, nullptr, false);
}

ConnectionInfo::ConnectionInfo(HSteamNetConnection conn): connection(conn), connectee(nullptr) { 

}

ConnectionInfo::~ConnectionInfo() {
    SteamNetworkingSockets()->CloseConnection(connection, 0, nullptr, false);
}

NetworkUserMessageConnection::~NetworkUserMessageConnection() {
    auto& channelInfo = NetworkingEngine::Get().userEventChannels[channel];
	for (size_t i = 0; i < channelInfo.connected.size(); i++) {
		if (channelInfo.connected[i].first == connectionId) {
			channelInfo.connected[i] = std::move(channelInfo.connected.back());
			channelInfo.connected.pop_back();
			break;
		}
	}
	if (channelInfo.connected.empty()) {
		NetworkingEngine::Get().userEventChannels.erase(channel);
	}
}

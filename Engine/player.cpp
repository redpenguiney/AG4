#include "player.hpp"

//Player::Player(Player&& old) {

//}

Player::~Player() {
}

Player::Player() {
	sockConnection = std::nullopt;
}

Player::Player(HSteamNetConnection conn): sockConnection(conn)
{
}

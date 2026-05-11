#include "player.hpp"

Player::Player(Player&& old) {
	connectionInfo = old.connectionInfo;
	old.connectionInfo = nullptr;
}

Player::~Player() {
	if (connectionInfo) delete connectionInfo;
}

Player::Player() {
	connectionInfo = nullptr;
}

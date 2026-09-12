#include "player.hpp"
#include "networking_engine.hpp"

//Player::Player(Player&& old) {

//}

Player::~Player() {
}

bool Player::IsLocalMachine()
{
	return NetworkingEngine::Get().GetLocalMachinePlayer().get() == this;
}

bool Player::IsConnected()
{
	for (auto& player : NetworkingEngine::Get().GetPlayers()) {
		if (player.get() == this) return true;
	}
	return false;
}

static size_t nextId = 0;

Player::Player():
id(nextId++)
{
}
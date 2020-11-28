#include "GameManager.h"

GameManager::GameManager()
{
	nextPlayerId = 0;
	
}
GameManager::~GameManager()
{
	for (auto player : players)
		delete player;
}

void GameManager::addEmptyPlayer(unsigned short clientPort)
{
	players.emplace_back(new Player(nextPlayerId));
	playerIdToPort.insert_or_assign(nextPlayerId, clientPort);
	nextPlayerId++;
}

uint8_t* GameManager::getPlayerData(uint8_t playerId)
{
	return players[playerId]->getMessagePointer();
}

unsigned short GameManager::getPortByPlayerId(uint8_t playerId)
{
	return playerIdToPort[playerId];
}

uint8_t GameManager::getPlayerIdByPort(unsigned short port)
{
	for (auto it = playerIdToPort.begin(); it != playerIdToPort.end(); ++it)
		if (it->second == port)
			return it->first;
	return -1;
}

int GameManager::getPlayersCount()
{
	return players.size();
}

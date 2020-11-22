#pragma once
#include <vector>
#include "Player.h"
#include <unordered_map>
class GameManager
{
public:
	GameManager();
	~GameManager();
	void addEmptyPlayer(unsigned short clientPort);
	uint8_t* getPlayerData(uint8_t playerId);
	unsigned short getPortByPlayerId(uint8_t playerId);
	uint8_t getPlayerIdByPort(unsigned short port);
	int getPlayersCount();
private:
	int nextPlayerId;
	std::vector<Player*> players;
	std::unordered_map<uint8_t, unsigned short> playerIdToPort;

};


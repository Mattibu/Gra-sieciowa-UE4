#pragma once
#include <cstdint>
class Player
{
public:
	Player(uint8_t playerId);
	~Player();
	uint8_t* getMessagePointer();

private:
	uint8_t* dataPointer;
	uint8_t id;
	float xPosition;
	float yPosition;
	float zPosition;
	float xDirection;
	float yDirection;
	float zDirection;
	float speed;
};


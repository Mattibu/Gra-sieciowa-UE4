#include "Player.h"

Player::Player(uint8_t playerId)
{
	dataPointer = new uint8_t[sizeof(Player)];
	id = playerId;
	xPosition = playerId * 100 + 10;
	yPosition = playerId * 100 + 11;
	zPosition = playerId * 100 + 12;
	xDirection = playerId * 100 + 13;
	yDirection = playerId * 100 + 14;
	zDirection = playerId * 100 + 15;
	speed = playerId * 100 + 16;
}

Player::~Player()
{
	delete dataPointer;
}

uint8_t* Player::getMessagePointer()
{
	int pointerPos = 0;
	dataPointer[pointerPos] = id;
	uint8_t* array;
	array = reinterpret_cast<uint8_t*>(&xPosition);
	++pointerPos;
	/*float xPosition*/
	dataPointer[pointerPos] = array[0];
	dataPointer[pointerPos+1] = array[1];
	dataPointer[pointerPos+2] = array[2];
	dataPointer[pointerPos+3] = array[3];
	pointerPos+=4;
	/*float yPosition*/
	array = reinterpret_cast<uint8_t*>(&yPosition);
	dataPointer[pointerPos] = array[0];
	dataPointer[pointerPos + 1] = array[1];
	dataPointer[pointerPos + 2] = array[2];
	dataPointer[pointerPos + 3] = array[3];
	pointerPos += 4;
	/*float zPosition*/
	array = reinterpret_cast<uint8_t*>(&zPosition);
	dataPointer[pointerPos] = array[0];
	dataPointer[pointerPos + 1] = array[1];
	dataPointer[pointerPos + 2] = array[2];
	dataPointer[pointerPos + 3] = array[3];
	pointerPos += 4;
	/*float xDirection*/
	array = reinterpret_cast<uint8_t*>(&xDirection);
	dataPointer[pointerPos] = array[0];
	dataPointer[pointerPos + 1] = array[1];
	dataPointer[pointerPos + 2] = array[2];
	dataPointer[pointerPos + 3] = array[3];
	pointerPos += 4;
	/*float yDirection*/
	array = reinterpret_cast<uint8_t*>(&yDirection);
	dataPointer[pointerPos] = array[0];
	dataPointer[pointerPos + 1] = array[1];
	dataPointer[pointerPos + 2] = array[2];
	dataPointer[pointerPos + 3] = array[3];
	pointerPos += 4;
	/*float zDirection*/
	array = reinterpret_cast<uint8_t*>(&zDirection);
	dataPointer[pointerPos] = array[0];
	dataPointer[pointerPos + 1] = array[1];
	dataPointer[pointerPos + 2] = array[2];
	dataPointer[pointerPos + 3] = array[3];
	pointerPos += 4;
	/*float speed*/
	array = reinterpret_cast<uint8_t*>(&speed);
	dataPointer[pointerPos] = array[0];
	dataPointer[pointerPos + 1] = array[1];
	dataPointer[pointerPos + 2] = array[2];
	dataPointer[pointerPos + 3] = array[3];
	pointerPos += 4;
	return dataPointer;
}

#pragma once
#include "vector.h"
#include <string>

class Entity
{
public:
	std::string name;
	uintptr_t pawnAddress;
	uintptr_t controllerAddress;
	uintptr_t cameraServices;	
	int health;
	int team;
	float flashDuration;
	unsigned int fFlag;
	unsigned int lifeState;
	int entIndex;
	bool spotted;
	Vector3 head;
	Vector3 origin;
	Vector3 viewOffset;
	Vector3 aimPunch;
	Vector3 velocity;
	float distance;
};
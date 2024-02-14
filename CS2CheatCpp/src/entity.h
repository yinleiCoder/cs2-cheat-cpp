#pragma once
#include "vector.h"
#include <string>

class Entity
{
public:
	std::string name;
	uintptr_t pawnAddress;
	uintptr_t controllerAddress;
	int health;
	int team;
	float flashDuration;
	unsigned int fFlag;
	unsigned int lifeState;
	int entIndex;
	Vector3 origin;
	Vector3 viewOffset;
	float distance;
};
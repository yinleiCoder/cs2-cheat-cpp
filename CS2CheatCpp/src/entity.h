#pragma once
#include "vector.h"
#include <string>
#include <vector>

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
	float distance;
	unsigned int fFlag;
	unsigned int lifeState;
	int entIndex;
	short currentWeaponIndex;
	const char* currentWeaponName;
	bool spotted;
	Vector3 head;
	Vector3 origin;
	Vector3 viewOffset;
	Vector3 aimPunch;
	Vector3 velocity;
	std::vector<Vector3> bones;
};
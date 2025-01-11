#pragma once
#include "Vector.h"
#include <cstdint>

class Entity
{
public:
	char pad_0000[92]; //0x0000
	Vector3 feet; //0x005C
	char pad_0068[56]; //0x0068
	Vector3 torso; //0x00A0
	char pad_00AC[52]; //0x00AC
	float health; //0x00E0 Health
	float shield; //0x00E4 
	uint8_t frag_grenade; //0x031E
	char pad_031F[1311]; //0x031F
}; //Size: 0x0850#pragma once

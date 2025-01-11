#pragma once

#include "Camera.h"
#include <d3dx9math.h> 

const uintptr_t pCamera = 0x006AC65C;
const uintptr_t pResolution = 0x0069C638;

class CameraExtended {
public:
	float windowWidth = 0, windowHeight = 0;
	Camera* camera;
	Vector3 WorldToScreen(Vector3 worldPosition);

	CameraExtended();
};
/**
	Breaking out particle effects initiation code for clarity.
	Emil Hedemalm
	2020-04-28
*/

#pragma once

#include "Entity/Entity.h"
#include "Color.h"

void ClearStars();
void NewStars(Vector3f starDir, float emissionSpeed, float starScale = 0.2f, Color color = Color::ColorByHex32(0xffffffff));
void LetStarsTrack(Entity* entity, Vector3f positionOffset);

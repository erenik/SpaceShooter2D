#pragma once

#include "Time/Time.h"

class SpawnGroup;
class LevelMessage;

// Individual parts of a level, usually either a SpawnGroup or a Message/Event
struct LevelElement {
	LevelElement();
	LevelElement(SpawnGroup* sg);
	LevelElement(LevelMessage* sg);
	~LevelElement();

	void InvalidateSpawnTime();
	Time SpawnOrStartTime();
	void Despawn();

	SpawnGroup * spawnGroup = nullptr;
	LevelMessage * levelMessage = nullptr;
	Time spawnOrStartTime;
};


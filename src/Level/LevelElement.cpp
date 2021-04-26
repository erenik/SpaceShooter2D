
#include "LevelElement.h"

#include "Globals.h"

// Game-specific
#include "LevelMessage.h"
#include "SpawnGroup.h"

LevelElement::LevelElement() : spawnGroup(nullptr), levelMessage(nullptr) {}

LevelElement::LevelElement(SpawnGroup* spawnGroup)
	: spawnGroup(spawnGroup), levelMessage(nullptr), spawnOrStartTime(spawnGroup->spawnTime) {
}
LevelElement::LevelElement(LevelMessage* levelMessage)
	: levelMessage(levelMessage), spawnGroup(nullptr), spawnOrStartTime(levelMessage->startTime) {
}
LevelElement::~LevelElement() {
	SAFE_DELETE(levelMessage);
	SAFE_DELETE(spawnGroup);
}

Time LevelElement::SpawnOrStartTime() {
	if (spawnGroup)
		return spawnGroup->spawnTime;
	else if (levelMessage)
		return levelMessage->startTime;
}



void LevelElement::InvalidateSpawnTime() {
	if (spawnGroup)
		spawnGroup->InvalidateSpawnTime();
	if (levelMessage)
		levelMessage->startTime = Time(TimeType::MILLISECONDS_NO_CALENDER);
}

void LevelElement::Despawn() {
	if (spawnGroup) {
		spawnGroup->Despawn();
	}
	else if (levelMessage) {
		levelMessage->DespawnEditorEntity();
	}
}

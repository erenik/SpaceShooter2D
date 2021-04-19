#pragma once

#include "SpaceShooter2D.h"

struct Mission;

class LevelEditor : public SpaceShooter2D
{
public:
	LevelEditor();
	virtual ~LevelEditor();

	// Inherited via AppState
	virtual void OnEnter(AppState* previousState) override;
	virtual void Process(int timeInMs) override;
	void Render(GraphicsState* graphicsState);
	virtual void OnExit(AppState* nextState) override;
	void ProcessMessage(Message* message);

	void LoadMission(Mission * mission);

	void OpenSpawnWindow();
	void CloseSpawnWindow();

private:

	// Spawns spawn group at appropriate place in the editor for manipulation.
	void Spawn(SpawnGroup * sg);
	void Respawn(SpawnGroup * sg);

	Time LastMessageOrSpawnGroupTime();

	void UpdatePositionsOfSpawnGroupsAfterIndex(int index);
	void UpdateWorldEntityForLevelTime(Time levelTime);

	Mission * editedMission;
	Level editedLevel;

	std::shared_ptr<Entity> levelEntity;

	SpawnGroup* editedSpawnGroup;

	bool movingCamera = false;
	Vector2i previousMousePosition;
	float zoomSpeed = 0;
};


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

	String LevelToTest() { return levelToTest; };

private:
	String levelToTest;


	void PopulateSpawnWindowLists();

	// Spawns spawn group at appropriate place in the editor for manipulation.
	void Spawn(SpawnGroup * sg);
	void Respawn(SpawnGroup * sg);

	Time PreviousMessageOrSpawnGroupTime(void * comparedTo);

	void UpdatePositionsOfSpawnGroupsAfterIndex(int index);
	void UpdateWorldEntityForLevelTime(Time levelTime);

	Mission * editedMission;
	Level editedLevel;

	Entity* levelEntity;

	SpawnGroup* editedSpawnGroup;
	LevelMessage* editedLevelMessage;

	bool movingCamera = false;
	Vector2i previousMousePosition;
	float zoomSpeed = 0;
};


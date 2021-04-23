#pragma once

#include "SpaceShooter2D.h"

struct Mission;

class LevelEditor : public SpaceShooter2D
{
public:
	LevelEditor();
	virtual ~LevelEditor();

	void Initialize();

	// Inherited via AppState
	virtual void OnEnter(AppState* previousState) override;
	virtual void Process(int timeInMs) override;
	void Render(GraphicsState* graphicsState);
	virtual void OnExit(AppState* nextState) override;
	void ProcessMessage(Message* message);

	void LoadMission(Mission * mission, bool force);
	bool LoadLevel(String fromPath);

	void OpenSpawnWindow();
	void CloseSpawnWindow();

	String LevelToTest() { return levelToTest; };

private:
	String levelToTest;


	void CreateNewSpawnGroup();
	void CreateNewLevelMessage();
	void PopulateSpawnWindowLists();

	// Spawns at appropriate place in the editor for manipulation.
	void Spawn(LevelElement* levelElement, const Time atTime);
	void Respawn(LevelElement* levelElement);

	// Returns a time in editorTime for a spawngroup to spawn at. Expensive ?
	Time CalculateEditorSpawnTimeFor(LevelElement* levelElement);

	Time PreviousMessageOrSpawnGroupTime(void * comparedTo);

	void UpdatePositionsLevelElementsAfter(int index);
	void UpdatePositionsLevelElementsAfter(LevelElement* levelElement);
	void UpdateWorldEntityForLevelTime(Time levelTime);

	Mission * editedMission;
	Level editedLevel;

	Entity* levelEntity;

	LevelElement* editedLevelElement;
	SpawnGroup* editedSpawnGroup;
	LevelMessage* editedLevelMessage;

	bool movingCamera = false;
	Vector2i previousMousePosition;
	float zoomSpeed = 0;
};


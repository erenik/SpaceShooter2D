#pragma once

#include "SpaceShooter2D.h"

struct Mission;

class LevelEditor : public SpaceShooter2D
{
public:
	LevelEditor();
	virtual ~LevelEditor();


	LevelElement* GetElementClosestToCamera() const;
	void Select(LevelElement* levelElement);
	void OnEditedLevelElementChanged();
	void Initialize();

	/// Callback from the Input-manager, query it for additional information as needed.
	virtual void KeyPressed(int keyCode, bool downBefore) override;

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

	void DeleteEditedElement();

private:
	String levelToTest;
	String levelSavePath;

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

	// Default mode. Clicking another element will set this to false. Escape should set it to true again.
	bool automaticallySelectClosestElement = true;
	bool movingCamera = false;
	Vector2i previousMousePosition;
	float zoomSpeed = 0;
	Vector2f cameraPan;
	float screenDistancePanned;
};


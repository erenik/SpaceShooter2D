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

private:
	Mission * editedMission;
	Level editedLevel;

	std::shared_ptr<Entity> levelEntity;
};


/** 
	Game state while in an active level, flying the ship.

*/
#pragma once

#include "SpaceShooter2D.h"
#include "AppStates/AppState.h"
#include "Graphics/Particles/Sparks.h"
#include "Graphics/Particles/Stars.h"

class Ship;
class PlayingLevel;


EntitySharedPtr LevelEntity();
ShipPtr PlayerShip();
EntitySharedPtr PlayerShipEntity();
PlayingLevel& PlayingLevelRef();

class PlayingLevel : public SpaceShooter2D
{
public:
	/// Searches among actively spawned ships.
	ShipPtr GetShip(EntitySharedPtr forEntity);

	ShipPtr GetShipByID(int id);
	void SetPlayingFieldSize(Vector2f newSize);
	void UpdateUI();
	// Inherited via AppState
	virtual void OnEnter(AppState* previousState) override;

	virtual void Process(int timeInMs) override;

	void Render(GraphicsState* graphicsState);

	virtual void OnExit(AppState* nextState) override;

	void ProcessMessage(Message* message);

	void OpenInGameMenu();

	void Cleanup();

	void UpdatePlayerVelocity();

	void NewPlayer();

	/// Loads target level. The source and separate .txt description have the same name, just different file-endings, e.g. "Level 1.png" and "Level 1.txt"
	void LoadLevel(String levelSource = "CurrentStageLevel");

	void UpdateRenderArrows();

	void RenderInLevel(GraphicsState* graphicsState);


	static bool IsPaused() { return paused; }


	static PlayingLevel* singleton;

	/// Time in current level, from 0 when starting. Measured in milliseconds.
	Time levelTime, flyTime;
	// extern int64 nowMs;
	int timeElapsedMs;
	/// Particle system for sparks/explosion-ish effects.
	Sparks* sparks;

	/// These will hopefully always be in AABB axes.
	Vector3f frustumMin, frustumMax;

	static ShipPtr playerShip;
	/// The level entity, around which the playing field and camera are based upon.
	static EntitySharedPtr levelEntity;
	static Vector2f playingFieldSize;
	static Vector2f playingFieldHalfSize;
	float playingFieldPadding;
	/// All ships, including player.
	List< std::shared_ptr<Entity> > shipEntities;
	List< std::shared_ptr<Entity> > projectileEntities;
	String playerName;
	static bool paused;
	String onDeath; // What happens when the player dies?


	void OpenSpawnWindow();
	void CloseSpawnWindow();

private:


	Time now;

	ParticleEmitter* starEmitter = nullptr;

	/// Particle system for sparks/explosion-ish effects.
	Stars* stars = NULL;

	/// 4 entities constitude the blackness.
	List< std::shared_ptr<Entity> > blacknessEntities;

};


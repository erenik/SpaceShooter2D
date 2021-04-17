/** 
	Game state while in an active level, flying the ship.

*/
#pragma once

#include "SpaceShooter2D.h"
#include "AppStates/AppState.h"
#include "Graphics/Particles/Sparks.h"
#include "Graphics/Particles/Stars.h"

class Ship;
class PlayerShip;
class PlayingLevel;
struct Mission;

ShipPtr GetPlayerShip();
EntitySharedPtr PlayerShipEntity();
PlayingLevel& PlayingLevelRef();

class PlayingLevel : public SpaceShooter2D
{
public:
	PlayingLevel();

	Level& GetLevel() { return level; };

	/// Searches among actively spawned ships.
	ShipPtr GetShip(EntitySharedPtr forEntity);

	ShipPtr GetShipByID(int id);
	void UpdateUI();
	// Inherited via AppState
	virtual void OnEnter(AppState* previousState) override;
	virtual void Process(int timeInMs) override;
	void Render(GraphicsState* graphicsState);
	virtual void OnExit(AppState* nextState) override;
	void ProcessMessage(Message* message);

	void OnPlayerSpawned();
	void OpenInGameMenu();
	void Cleanup();
	void UpdatePlayerVelocity();
	void NewPlayer();
	// If true, game over is pending/checking respawn conditions, don't process other messages.
	bool CheckForGameOver(int timeInMs);
	void OnPlayerDied();

	void SpawnPlayer();
	void ToggleInGameMenu();

	float LeftEdge() const {
		return levelEntity->worldPosition.x - level.playingFieldHalfSize.x;
	}
	float RightEdge() const {
		return levelEntity->worldPosition.x + level.playingFieldHalfSize.x;
	}

	const Vector2f& PlayingFieldSize() const {
		return level.playingFieldSize;
	}
	const Vector2f& PlayingFieldHalfSize() const {
		return level.playingFieldHalfSize;
	}

	/// Loads target level. The source and separate .txt description have the same name, just different file-endings, e.g. "Level 1.png" and "Level 1.txt"
	void LoadLevel(String levelSource, Mission * forMission);

	void UpdateRenderArrows();

	void RenderInLevel(GraphicsState* graphicsState);

	void JumpToAfterMessage(LevelMessage * message);
	void JumpToTime(String timeString);
	void JumpToTime(Time time);

	// Returns the new time set to.
	Time SetTime(Time time);

	static bool IsPaused() { return paused; }

	void HideLevelMessage();


	static PlayingLevel* singleton;

	/** Time in current level, from 0 when starting, which may pause while groups are spawning or messages being displayed. 
		Measured in milliseconds. FlyTime in contrast refers to total seconds played the level as perceived by the User.
		For achievements then low FlyTime is interested, for level planning and spawning groups of enemies LevelTime is interesting.
	*/
	Time levelTime = Time(TimeType::MILLISECONDS_NO_CALENDER); // Time used in level-scripting. Will be paused arbitrarily to allow for easy scripting.
	// Time to rewind to for scripting.
	Time rewindPoint;
	Time flyTime = Time(TimeType::MILLISECONDS_NO_CALENDER); // The actual player-felt time. 
		
	// extern int64 nowMs;
	int timeElapsedMs = 0;
	int hudUpdateMs = 0;
	/// Particle system for sparks/explosion-ish effects.
	std::shared_ptr<ParticleSystem> sparks;

	/// These will hopefully always be in AABB axes.
	Vector3f frustumMin, frustumMax;

	static std::shared_ptr<PlayerShip> playerShip;
	/// The level entity, around which the playing field and camera are based upon.
	static EntitySharedPtr levelEntity;
	//static Vector2f playingFieldSize;
	//static Vector2f playingFieldHalfSize;
	/// All ships, including player.
	List< std::shared_ptr<Entity> > shipEntities;
	List< std::shared_ptr<Entity> > projectileEntities;
	String onDeath; // What happens when the player dies?


	void OpenSpawnWindow();
	void CloseSpawnWindow();


	bool GameTimePausedDueToActiveSpawnGroup();
	bool GameTimePausedDueToActiveLevelMessage();
	bool GameTimePaused();

	bool gameTimePausedDueToScriptableMessage = false;


	// In the most recent spawn group
	bool DefeatedAllEnemiesInTheLastSpawnGroup();
	bool failedToSurvive = false;

	// Position boundaries.
	float removeInvuln = 0;
	float despawnPositionLeft = 0;
	float despawnPositionRight = 0;
	float spawnPositionRight = 0;

	int spaceDebrisCollected = 0;

	void SetLastSpawnGroup(SpawnGroup * sg);

	void LoadWeapons();

	// Statistics for achievements
	String enemyProjectilesDodgedString;
	void UpdateEnemyProjectilesDodgedString();
	List<bool> projectilesDodged;
	int projectilesFired = 0;
	int projectileDamageTaken = 0;
	float armorRegenerated = 0;
	float shieldRegenerated = 0;


	int hudUpdates = 0;

	// Temporary mission score
	int score = 0;

	// Name of events
	List<String> eventsTriggered;

	// Simulation speed
	float gameSpeed = 1.0f;

	void SetPlayerMovement(Vector3f inDirection);

	virtual void CreateDefaultBindings() override;

private:

	Level level;

	SpawnGroup * lastSpawnGroup;
	Vector3f requestedMovement;
	Mission * currentMission;

	Time now;
	int timeDeadMs = 0;
	bool autoProceedMessages = false;

	std::shared_ptr<ParticleEmitter> starEmitter = nullptr;

};


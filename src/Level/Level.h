/// Emil Hedemalm
/// 2015-01-21
/// Level.

#ifndef LEVEL_H
#define LEVEL_H

#include "../Base/Ship.h"
#include "Color.h"

#define SPAWNED_ENEMIES_LOG "SpawnedEnemies.srl"

void GenerateLevel (PlayingLevel& playingLevel, String arguments);

// What is this...?
const float playingFieldPadding = 1;

struct ShipColorCoding 
{
	String ship;
	Vector3i color;
};

class Explosion;
class SpawnGroup;
class Camera;
class Level;
class LevelMessage;
class Message;
struct LevelElement;

extern Camera * levelCamera;
extern Level * activeLevel;

namespace LevelLoader {
	void SetGroupDefaults(SpawnGroup* sg);
};

class Level 
{
public:
	Level();
	virtual ~Level();

	int SpawnGroupsActive();

	void AddSpawnGroup(SpawnGroup* sg);
	void AddMessage(LevelMessage* lm);
	int GetElementIndexOf(LevelMessage* lm);
	int GetElementIndexOf(SpawnGroup* sg);

	/// Deletes all ships, spawngroups, resets variables to defaults.
	void Clear(PlayingLevel& playingLevel);
	bool FinishedSpawning();
	bool Load(String fromSource);
	bool Save(String toFile);
	/// Starts BGM, starts clocks/timers if any, etc.
	void OnEnter();
	// Used for player and camera. 
	Vector3f BaseVelocity();
	/// Creates player entity within this level. (used for spawning)
	Entity* SpawnPlayer(PlayingLevel& playingLevel, Ship* playerShip, ConstVec3fr atPosition);
	void SetupCamera();
	/// o.o
	void Process(PlayingLevel& playingLevel, int timeInMs);
	void UpdateSpawnDespawnLimits(Entity* levelEntity);
	void ProcessMessage(PlayingLevel& playingLevel, Message * message);
	void ProceedMessage();
	// Dialogue, tutorials
	void ProcessLevelMessages(Time levelTime);
	void SetTime(Time newTime);
	/// enable respawing on shit again.
	void OnLevelTimeAdjusted(Time levelTime);
	Entity* ClosestShipOrObstacle(PlayingLevel& playingLevel, bool enemy, ConstVec3fr position, float& distanceOut);
	Entity* ClosestTarget(PlayingLevel& playingLevel, bool enemy, ConstVec3fr position);
	/// o.o'
	void Explode(Weapon & weapon, Entity* causingEntity, bool enemy);
	/// Returns ships close enough to given point. Returns distance to them too. Checks only to center of ship, not edges.
	List<Ship*> GetShipsAtPoint(ConstVec3fr position, float maxRadius, List<float> & distances);

	// # of spawn groups yet to start spawning. (may be 0 while spawning last one or enemies still on screen).
	int SpawnGroupsRemaining();
	// Null if none after this one.
	SpawnGroup* NextSpawnGroup();
	void RemoveRemainingSpawnGroups();
	void SetSpawnGroupsFinishedAndDefeated(Time beforeLevelTime);
	void RemoveExistingEnemies(PlayingLevel& playingLevel);

	void HideLevelMessage(LevelMessage * levelMessage);

	LevelMessage * GetMessageWithTextId(String id);

	/// Yes.
	List<Ship*> PlayerShips(PlayingLevel& playingLevel);

	/// In format mm:ss.ms
	//void JumpToTime(String timeString);

	String source;

	// Sets source and updates entity texture as well.
	void SetBackgroundSource(String source);

	String backgroundSource;

	/// Enemy ships within.
	List<Ship*> enemyShips, alliedShips, ships;

	/// Default.. 20.0. Dictates movable region in Y, at least.
	float height;
	/// Default 0.
	enum 
	{
		NEVER,
		SURVIVE_ALL_SPAWN_GROUPS, NO_MORE_ENEMIES = SURVIVE_ALL_SPAWN_GROUPS,
		EVENT_TRIGGERED,
	};
	int endCriteria;


	// How big area the player can move about in. Also dictates how you design the level and spawn group locations, etc.
	Vector2f playingFieldSize;
	Vector2f playingFieldHalfSize; // Half of the above, only used for convenience.



	List<LevelElement*> levelElements;

	/// New spawn style.
	List<SpawnGroup*> SpawnGroups();
	/// o.o
	List<LevelMessage*> Messages();

	List<Explosion*> explosions;

	/// Music source to play.
	String music;
	/// Goal position in X.
	int goalPosition;

	Vector3f starSpeed;
	Color starColor;

	bool levelCleared;

	/// Displayed ones, that is.
	LevelMessage * activeLevelMessage;

	bool spawnGroupsPauseGameTime;
	bool messagesPauseGameTime;

	// Check spawn groups.
	bool LevelCleared(Time levelTime, List<Entity*> shipEntities, List<Ship*> playerShips);

private:

	Entity* playerAimCrossHair;
};

#endif

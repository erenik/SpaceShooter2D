/// Emil Hedemalm
/// 2015-03-03
/// Spawn group, yo.

#ifndef SPAWN_GROUP_H
#define SPAWN_GROUP_H

#include "Base/Ship.h"
#include "Time/Time.h"
#include "MovementPattern.h"

/// Types. Default LINE_X?
enum class Formation {
	BAD_FORMATION = 0,
	LINE_X,
	DOUBLE_LINE_X,
	LINE_Y,
	LINE_XY,
	X,
	SQUARE,
	CIRCLE,
	HALF_CIRCLE_LEFT,
	HALF_CIRCLE_RIGHT,
	V_X, /// Typical V-bird-formation, flying X-wise.
	V_Y, /// Typical V-bird-formation, flying Y-wise.
	SWARM_BOX_XY, /// Random-based swarm with some minimum threshold distance between each ship, skipping ships if area is not large enough.
	FORMATIONS
};

String GetName(Formation forFormationType);
Formation GetFormationByName(String name);


class SpawnGroup 
{
	friend class Ship;
public:
	SpawnGroup();
	virtual ~SpawnGroup();
	void Nullify();
	void ResetForSpawning();
	void RemoveThis(Ship* sp);

	bool AllShipsSpawned();
	/** Spawns ze entities. 
		True if spawning sub-part of an aggregate formation-type. 
		Returns true if it has finished spawning. 
		Call again until it returns true each iteration (required for some formations).
	*/	
	bool Spawn(PlayingLevel& playingLevel);
	/// To avoid spawning later.
	void SetFinishedSpawning();
	void OnFinishedSpawning(PlayingLevel& playingLevel);
	void SetDefeated();
	bool FinishedSpawning() { return finishedSpawning;};

	/// Gathers all ships internally for spawning. Returns lsit of all ships (used internally)
	void PrepareForSpawning(SpawnGroup * parent = 0);

	// Number of ships active and alive (not despawned or destroyed).
	int ShipsActive();

	/// Query, compares active ships vs. spawned amount
	bool DefeatedOrDespawned();
	void OnShipDestroyed(PlayingLevel& playingLevel, ShipPtr ship);
	void OnShipDespawned(PlayingLevel& playingLevel, ShipPtr ship);
	/// Creates string (sequence of lines) required to create this specific SpawnGroup in e.g. a level file.
	String GetLevelCreationString(Time t);

	Time SpawnTime() const { return spawnTime;  }
	void SetSpawnTime(Time newSpawnTime);

	/// o.o
	String name;
	String shipType;
	bool playerSurvived; // player survived the way?
	Vector3f position;
	/// Number along the formation bounds. Before PrepareForSpawning is called, this is an arbitrary argument, which may or may not be the same after preparing for spawning (e.g. it may multiply for generating a SQUARE formation).
	int number;
	// See enum above.
	Formation formation;
	MovementPattern mp;
	void ParseFormation(String fromString);
	/// Usually just 1 or 2 sizes are used (X,Y)
	Vector3f size;

	/// Time-spacing for spawning a flying incoming formation. Enables "line" of ships flying with similar characteristics but one arriving briefly after the other. If 0 is normal formation spawned instantaneously.
	int spawnIntervalMsBetweenEachShipInFormation;

	/// o-o relative to 5.0 for max.
	float relativeSpeed;
	bool shoot;
	/// If true, pauses game time, until all ships of the group have either been destroyed or despawned by exiting the screen.
	bool pausesGameTime;
	bool ShipsDefeatedOrDespawned();
	int ShipsDespawned();
	int ShipsDefeated();
	int shipsSpawned;

private:

	Time spawnTime;

	ShipPtr GetNextShipToSpawn();
	void SpawnAllShips(PlayingLevel& playingLevel);

	bool finishedSpawning;
	AETime lastSpawn;
	bool preparedForSpawning;
	List<ShipPtr> ships;
	// ?!
//	EntitySharedPtr SpawnShip(ConstVec3fr atPosition);
	void AddShipAtPosition(ConstVec3fr position);
};



#endif
/// Emil Hedemalm & Karl Wängberg
/// 2016-02-25
/// Movement pattern class batching up movement, rotation, etc. for easing level generation.

#ifndef MOVEMENT_PATTERN_H
#define MOVEMENT_PATTERN_H

#include "Base/Movement.h"
#include "Base/Rotation.h"

class MovementPattern
{
public:
	static void Load();
	static List<MovementPattern> movementPatterns;
	List<Movement> movements;
	List<Rotation> rotations;
	String name;
	int ID;
	/// For those movement patterns where directions may want unit to spawn a bit more to one side in order to properly pass through the main game window.
	Vector2f spawnOffset;

};

#endif

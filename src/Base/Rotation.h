/// Emil Hedemalm
/// 2015-01-27
/// Rotation for AIs.

#ifndef ROTATION_H
#define ROTATION_H

class Ship;
class PlayerShip;

#define ShipPtr std::shared_ptr<Ship>

#include "Entity/Entity.h"
#include "String/AEString.h"
#include "MathLib.h"

class PlayingLevel;

class Rotation
{
public:
	enum {
		NONE, // o.o
		MOVE_DIR,
		ROTATE_TO_FACE,
		SPINNING,
		WEAPON_TARGET,
	};
	Rotation();
	Rotation(int type);
	void Nullify();

	String ToString();
	static String Name(int type);
	static List<Rotation> ParseFrom(String);

	void OnEnter(ShipPtr ship);
	void OnFrame(std::shared_ptr<PlayerShip> playerShip, int timeInMs);


	// See enum above.
	int type;
	int durationMs;
	// For RotateToFace
	String target;
	float spinSpeed;

private:
	void MoveDir();	
	void RotateToFace(const Vector3f & position);
	void RotateToFaceDirection(const Vector3f & direction);
	/// Radian-direction last iteration. Used together with ships maxRadiansPerSecond and the time passed to dictate new direction.
	Angle lastFacingAngle;
	ShipPtr ship;
	EntitySharedPtr entity;
//	Vector3f targetPosition;
};

#endif

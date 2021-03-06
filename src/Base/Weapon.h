/// Emil Hedemalm
/// 2015-01-21
/// Weapon..

#ifndef WEAPON_H
#define WEAPON_H

#include "String/AEString.h"
#include "MathLib.h"
#include "Time/Time.h"
#include "Entity/Entity.h"

class PlayingLevel;
class Ship;
class Weapon;
class Entity;

extern Vector4f defaultAlliedProjectileColor;

class WeaponSet : public List<Weapon*> 
{
public:
	/// Handles dynamic allocation of the weapons, both adding and clearing.
	WeaponSet();
	virtual ~WeaponSet();
	WeaponSet(const WeaponSet & otherWeaponSet);
};

class LightningArc
{
public:
	LightningArc();
	int damage;
	Vector3f position;
	Entity* graphicalEntity;
	Entity* targetEntity;
	bool struckEntity;
	bool arcFinished; // When time expires or range has been reached.
	float maxRange;
	int maxBounces;
	Time arcTime;
	LightningArc * child;
};

class Weapon
{
public:

	enum class Type {
		None,
		MachineGun,
		SmallRockets,
		BigRockets,
		Lightning,
		LaserBeam,
		LaserBurst,
		HeatWave,
		IonCannon,
		Types
	};

	Weapon();

	String TypeName();
	static List<Weapon> GetAllOfType(Type type);
	static String GetTypeName(Type type);
	static Weapon::Type ParseType(String fromString);
	// Checks type and level in GameVars
	static bool PlayerOwns(Weapon& weapon);
	static void SetOwnedQuantity(Weapon& weapon, int quantity);

	static Weapon StartingWeapon();

	// Sets
	static bool Get(String byName, Weapon * weapon);
	/** For player-based, returns pointer, but should be used as reference only (*-dereference straight away). 
		Returns 0 if it doesn't exist. */
	static const Weapon * const Get(Type type, int level);
	static bool LoadTypes(String fromFile);
	/// Moves the aim of this weapon turrent.
	void Aim(PlayingLevel& playingLevel, Ship* ship);
	/// Shoots using previously calculated aim.
	void Shoot(PlayingLevel& playingLevel, Ship* ship, const Vector2f& currentAim);
	/// Called to update the various states of the weapon, such as reload time, making lightning arcs jump, etc.
	void Process(PlayingLevel& playingLevel, Ship* ship, int timeInMs);
	void ProcessLightning(PlayingLevel& playingLevel, Ship* ship, bool initial = false);
	void QueueReload();

	// Path to icon
	String Icon();

	/// Based on ship.
	Vector3f WorldPosition(Entity* basedOnShipEntity);
	Vector3f location;

	List<LightningArc*> arcs;
	List<Ship*> shipsStruckThisArc; /// For skipping

	/// Delay in milliseconds between bounces for lightning
	int arcDelay;
	int maxBounces; /// Used to make lightning end prematurely.
	Ship* nextTarget; // For lightning
	int currCooldownMs = 0; /// Used instead of flyTime.
	int previousUIUpdateCooldownMs = 0;
	float stability;
	String name; // Printable
	String id; // Shorter
	bool enabled; // default true.
	int cost; // o-o
	Type type; // mainly for player-based weapons.
	int level; // Also mainly for player-based weapons.
	/// -1 = Infinite, >= 0 = Finite
	bool reloading;
	// Shots left in the current burst/round.
	int shotsLeft;
	// Does nothing at all. If finite, this means the total supply of shots you have before you cannot use the weapon any more.
	int ammunition;
	int lifeTimeMs;
	int numberOfProjectiles; // Per 'firing'
	int distribution; // Default CONE?
	float linearDamping; // Applied for slowing bullets (Ion Flak).
	enum {
		CONE,
	};
	/// Cooldown.
	Time cooldown;
	enum {
		STRAIGHT,
		SPINNING_OUTWARD,
		HOMING,
	};
	/// For boss-spam stuff.
	bool circleSpam;
	int projectilePath;
	float projectileSpeed;
	float homingFactor; // For heat-seaking/auto-aiming missiles.
	String projectileShape;
	float projectileScale;
	float maxRange; // Used for Lightning among other things
	// Damage inflicted.
	float damage;
	float relativeStrength; /// Used to apply distance attentuation to damage, etc. (e.g. Heat-wave) default 1.0
	float explosionRadius; // o.o' More damage closer to the center of impact.
	// Penetration rate.
	float penetration;
	// If true, auto-aims at the player.
	bool aim;
	/// Not just looking, thinking.
	bool estimatePosition;
	/// Angle in degrees.
	int angle;
	// Burst stuff.
	bool burst;
	// Start time of last/active burst.
	Time burstStart;
	/// Delay between each round within the burst.
	Time burstRoundDelay;
	/// For burst.
	int burstRounds;
	// Restarts
	int burstRoundsShot;
	/// Linear scaling over time (multiplied with initial scale?) 
	float linearScaling;
	// Acceleration in forward vector direction.
	float acceleration;
	/// Last show, format Time::Now().Milliseconds()
//	Time lastShot;
	static List<Weapon> types;
	/// Sound effects (SFX)
	String shootSFX, hitSFX;

	/// For aiming weapons, mainly. Normalized vector.
	Vector3f currentAim;
	/// Recalculated every call to Aim()
	Vector3f weaponWorldPosition;
};

String toString(Weapon::Type type);

#endif

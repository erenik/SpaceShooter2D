/// Emil Hedemalm
/// 2015-01-21
/// Ship.

#ifndef SHIP_H
#define SHIP_H

#include "Weapon.h"
#include "MathLib.h"
#include "Movement.h"
#include "Rotation.h"
#include "Base/Gear.h"

class PlayingLevel;
class Script;
class Entity;
class Model;
class SpawnGroup;
class WeaponScript;
class ShipProperty;

#define ShipPtr std::shared_ptr<Ship>

enum SkillType {
	NO_SKILL,
	ATTACK_FRENZY,
	SPEED_BOOST,
	POWER_SHIELD,
};

class Skill {
public:
	static int Cooldown(SkillType type);
	static String Name(SkillType type ) {
		switch (type) {
		case NO_SKILL: return "None";
		case ATTACK_FRENZY: return "Attack Frenzy";
		case SPEED_BOOST: return "Speed Boost";
		case POWER_SHIELD: return "Power Shield";
		}
	}
};

enum class DamageSource {
	Projectile, // Reactivity works well against these.
	Explosion, // For bigger missiles creating larger spread impacts. Reactivity works less well against this.
	Collision // Only Toughness helps here.
};

class alignas(16) Ship
{
protected:
	Ship();
	static ShipPtr NewShip();
public:
	static ShipPtr NewShip(const Ship& ref);

	~Ship();
	std::weak_ptr<Ship> selfPtr;
	ShipPtr GetSharedPtr();

	/// Returns nullptr if it was not found
	static ShipPtr GetByType(String typeName);
	void CopyStatsFrom(const Ship& ref);
	void CopyWeaponsFrom(const Ship& ref);

	/// Call on spawning.
	void RandomizeWeaponCooldowns();
	/// Spawns at local position according to window/player area, creating entities, registering for movement, etc. Returns it and all children spawned with it.
	List< std::shared_ptr<Entity> > Spawn(ConstVec3fr atWorldPosition, ShipPtr parent, PlayingLevel & playingLevel);
	/// Handles spawning of children as needed.
	List< std::shared_ptr<Entity> > SpawnChildren(PlayingLevel& playingLevel);
	void Despawn(PlayingLevel& playingLevel, bool doExplodeEffectsForChildren);
	void ExplodeEffects(PlayingLevel& playingLevel);
	/// Checks current movement. Will only return true if movement is target based and destination is within threshold.
	bool ArrivedAtDestination();
	void Process(PlayingLevel& playingLevel, int timeInMs);
	void ProcessAI(PlayingLevel& playingLevel, int timeInMs);
	void ProcessWeapons(PlayingLevel& playingLevel, int timeInMs);

	/// Disables weapon in this and children ships.
	void DisableWeapon(String weaponName);
	bool DisableWeaponsByID(int id);
	bool DisableAllWeapons();
	bool EnableWeaponsByID(int id);
	// What
	void SetWeaponCooldownByID(int id, AETime newcooldown);
	/// Prepends the source with '/obj/Ships/' and appends '.obj'. Uses default 'Ship.obj' if needed.
	Model * GetModel();
	/// o.o
	void DisableMovement();
	void OnSpeedUpdated(PlayingLevel& playingLevel);
	void SetProjectileSpeedBonus(float newBonus); // Sets new bonus, updates weapons if needed.
	void SetWeaponCooldownBonus(float newBonus); // Sets new bonus, updates weapons if needed.
	void Damage(PlayingLevel& playingLevel, Weapon & usingWeapon);
	/// Returns true if destroyed -> shouldn't touch any more.
	bool Damage(PlayingLevel& playingLevel, float amount, bool ignoreShield, DamageSource source);
	void Destroy(PlayingLevel& playingLevel);
	// Load ship-types.
	static bool LoadTypes(String file);
	/// E.g. "Straight(10), MoveTo(X Y 5 20, 5)"
	void ParseMovement(String fromString);
	/// E.g. "DoveDir(3), RotateToFace(player, 5)"
	void ParseRotation(String fromString);
	/// Sets movement. Clears any other existing movements.
	void SetMovement(PlayingLevel& playingLevel, Movement & movement);
	void SetSpeed(PlayingLevel& playingLevel, float speed);
	/// Creates new ship of specified type.
	static ShipPtr New(String shipByName);

	/// Main script to play.
	String scriptSource;
	Script * script;
	
	/// Unique ID.
	const int ID() {return shipID;}; 

	/// Returns speed, accounting for active skills, weights, etc.
	float Speed();
	/// Accounting for boosting skills.
	float MaxShield();

	/// Checks weapon's latest aim dir.
	Vector3f WeaponTargetDir();

	
	int CurrentWeaponIndex();
	bool SwitchToWeapon(int index);

	/// Calls OnEnter for the initial movement pattern.
	void StartMovement(PlayingLevel& playingLevel);

	/// For player ship.
	Weapon * SetWeaponLevel(Weapon::Type ofType, int level);
	Weapon * GetWeapon(Weapon::Type ofType);

	void SetLevelOfAllWeaponsTo(int level);

	void ActivateSkill();

	/// Names of children to spawn along side it.
	List<String> childrenStrings;

	// Name or type. 
	String name;
	int childrenDestroyed; // 0 initially
	/// Flag false for bosses.
	bool despawnOutsideFrame;
	// Faction.
	String type;
	String physicsModel;

	SpawnGroup * spawnGroup;
	ShipPtr parent;
	ShipProperty * shipProperty;
	List<ShipPtr> children;
	// Bools
	bool canMove;
	bool movementDisabled; // temporarily.
	bool canShoot;
	bool hasShield;
	bool shoot; // if shooting is requested.
	bool weaponScriptActive; // Default false.
	bool boss; //is it a boss?
	int difficulty;
	int timeSinceLastSkillUseMs;
	SkillType skill; // Default 0, see above.
	SkillType activeSkill;

	int SkillCooldown();

	int skillDurationMs;
	float skillCooldownMultiplier; // default 1, lower in situations or tutorial
	String onCollision;
	bool spawned;
	bool destroyed;
	bool despawned;

	/// In order to not take damage allllll the time (depending on processor speed, etc. too.)
	Time lastShipCollision;
	Time collisionDamageCooldown; // default 100 ms.
	// Default 0. Scriptable.
	float projectileSpeedBonus;
	float weaponCooldownBonus;
	/// Mooovemeeeeeeent
	List<Movement> movements;
	int currentMovement; // Index of which pattern is active at the moment.
	int timeInCurrentMovement; // Also milliseconds.
	List<Rotation> rotations;
	int currentRotation;
	int timeInCurrentRotation;
	/// Maximum amount of radians the ship may rotate per second.
	float maxRadiansPerSecond;

	// Parsed value divided by 5.
	float speed;
	float shieldValue, maxShieldValue;
	/// Regen per millisecond
	float shieldRegenRate;
	float hp;
	// Armor regen per second
	float armorRegenRate;
	int maxHP;
	int collideDamage;
	float heatDamageTaken;
	List<String> abilities;
	List<float> abilityCooldown;
	String graphicModel;
	String other;

	WeaponSet weapons;
	Weapon * activeWeapon; // One active weapon at a time.. for the player at least.

	/// If allied or player, false for enemies.
	bool allied;
	/// If the ship.. is enemy ai? Should be renamed
	bool enemy;
	bool spawnInvulnerability;
	/// Yielded when slaying it.
	int score; 

	// Default false
	EntitySharedPtr entity;
	// Data details.
	// Spawn position.
	Vector3f position;
	/// As loaded.
	static List<ShipPtr>  types;

	WeaponScript * weaponScript;

	// Totals from equipped stuff.
	float armorToughness; // Default 10?
	float armorReactivity; // Default 1?

private:
	int shipID;
	static int shipIDEnumerator;
};

#endif

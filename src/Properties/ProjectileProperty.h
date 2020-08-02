/// Emil Hedemalm
/// 2015-01-21
/// Projectile.

#ifndef PROJ_PROP_H
#define PROJ_PROP_H

#include "Base/Weapon.h"
#include "Entity/EntityProperty.h"
#include "Time/Time.h"

class ProjectileProperty : public EntityProperty 
{
public:
	ProjectileProperty(const Weapon & weaponThatSpawnedIt, EntitySharedPtr owner, bool enemy);
	virtual ~ProjectileProperty();

	// Static version.
	static int ID();

	/// If reacting to collisions...
	virtual void OnCollision(Collision & data);

	void Remove();	/// Removes entity.
	void Destroy();	/// Creates GFX for explosions, detonates for missiles

	// Fall asleep.. unregistering it from physics, graphics, etc.
	void SleepThread();
	void ResetVelocity();

	// For Machine guns, line trace
	void CreateProjectileTraceEmitter(Vector3f initialPosition);
	// For rockets
	void CreateThrustEmitter(Vector3f initialPosition);

	/// Time passed in seconds..!
	virtual void Process(int timeInMs);
	virtual void ProcessMessage(Message * message);

	bool ShouldDamage(ShipPtr ship);

	/// Resets sleep-flag, among other things
	void OnSpawn();

	// Whose side the projectile belongs to. Determines who it will react to/damage.
	bool enemy;

	Weapon weapon;
	float distanceTraveled;

	List<ShipPtr> penetratedTargets;
	/// If not currently active (available for re-use).
	bool sleeping;

	/// Used for various effects, such as laser burst wobbling, fading damage/removal for heat-wave/ion flak, etc.
	int timeAliveMs;
	Vector4f color;
	String onCollisionMessage;
	Vector3f direction;
	Vector3f up; // Set upon launch, used to change direction of laser burst projectiles
	int nextWobbleMs; // Next time in timeAliveMs that it should change direction.

	std::shared_ptr<ParticleEmitter> thrustEmitter, traceEmitter;

	EntitySharedPtr targetLock;
};

#endif

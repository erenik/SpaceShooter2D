/// Emil Hedemalm
/// 2015-01-21
/// Projectile.

#include "SpaceShooter2D.h"
#include "Properties/ProjectileProperty.h"
#include "PlayingLevel.h"
#include "Graphics/Particles/ThrustEmitter.h"
#include "Graphics/Particles/SmoothedPositionParticleEmitter.h"

ProjectileProperty::ProjectileProperty(const Weapon & weaponThatSpawnedIt, EntitySharedPtr owner, bool enemy)
: EntityProperty("ProjProp", ID(), owner), weapon(weaponThatSpawnedIt), enemy(enemy)
{
	sleeping = false;
	timeAliveMs = 0;
	nextWobbleMs = 0;
	thrustEmitter = nullptr;
}

ProjectileProperty::~ProjectileProperty() {
	if (thrustEmitter != nullptr)
		QueueGraphics(new GMDetachParticleEmitter(thrustEmitter));
	if (traceEmitter)
		QueueGraphics(new GMDetachParticleEmitter(traceEmitter));
}

// Static version.
int ProjectileProperty::ID()
{
	return PROJ_PROP;
}

/// If reacting to collisions...
void ProjectileProperty::OnCollision(Collision & data)
{
	// Do nothing?
//	Destroy();
}

void ProjectileProperty::Remove()
{
	PlayingLevel& pl = PlayingLevelRef();
	// Remove self.
	sleeping = true;
	MapMan.DeleteEntity(owner);
	pl.projectileEntities.Remove(owner);
}

void ProjectileProperty::Destroy()
{
	if (sleeping)
		return;
	PlayingLevel& pl = PlayingLevelRef();
	
	Remove();

	if (thrustEmitter != nullptr) {
		QueueGraphics(new GMDetachParticleEmitter(thrustEmitter));
		thrustEmitter = nullptr;
	}


	// Check distance to player.
//	Vector3f vectorDistance = (player1->position - atPosition);
//	vectorDistance /= 100.f;
//	float distSquared = vectorDistance.Length();
//	float distanceModifierToVolume = 1 / distSquared;
//	if (distanceModifierToVolume > 1.f)
//		distanceModifierToVolume = 1.f;


	// Check if an explosion should be spawned in its place.
	if (weapon.explosionRadius > 0.001)
	{
		activeLevel->Explode(weapon, owner, enemy);
		float lifeTime = weapon.explosionRadius / 10.f;
		ClampFloat(lifeTime, 2.5f, 10.f);
		// Explosion emitter o-o should prob. have its own system later on.
		auto tmpEmitter = new SparksEmitter(owner->worldPosition);
		tmpEmitter->SetEmissionVelocity(3.f);
		tmpEmitter->constantEmission = int( 40 + weapon.damage * weapon.explosionRadius);
		tmpEmitter->instantaneous = true;
		tmpEmitter->SetParticleLifeTime(2.5f);
		tmpEmitter->SetScale(0.15f);
		tmpEmitter->SetColor(color);
		tmpEmitter->SetRatioRandomVelocity(1.f);
		Graphics.QueueMessage(new GMAttachParticleEmitter(tmpEmitter->GetSharedPtr(), pl.sparks->GetSharedPtr()));
	}
	else /// Sparks for all physically based projectiles with friction against targets.
	{	
		// Add a temporary emitter to the particle system to add some sparks to the collision
		SparksEmitter * tmpEmitter = new SparksEmitter(owner->worldPosition);
		tmpEmitter->SetEmissionVelocity(3.f);
		tmpEmitter->constantEmission = 40;
		tmpEmitter->instantaneous = true;
		tmpEmitter->SetParticleLifeTime(2.5f);
		tmpEmitter->SetScale(0.1f);
		tmpEmitter->SetColor(color);
		tmpEmitter->SetRatioRandomVelocity(1.f);
		Graphics.QueueMessage(new GMAttachParticleEmitter(std::shared_ptr<ParticleEmitter>(tmpEmitter), std::weak_ptr<ParticleSystem>(pl.sparks)));
	}

//	float volume = distanceModifierToVolume * explosionSFXVolume;
//	AudioMan.QueueMessage(new AMPlaySFX("SpaceShooter/235968__tommccann__explosion-01.wav", volume));
}

Random wobbleRand;

/// Time passed in seconds..!
void ProjectileProperty::Process(int timeInMs)
{
	if (sleeping)
		return;
	PlayingLevel& pl = PlayingLevelRef();
	if (pl.paused)
		return;
	timeAliveMs += timeInMs;
	if (timeAliveMs > weapon.lifeTimeMs)
	{
		if (weapon.explosionRadius > 0.001)
		{
			Destroy();
			return;
		}
		Remove();
		return;
	}
	if (weapon.type == Weapon::Type::LaserBurst)
	{
		if (nextWobbleMs == 0 || nextWobbleMs < timeAliveMs)
		{
			bool wasRight = nextWobbleMs & 0x1;
			up = Vector3f(0,1,0);
			// Apply some pulse to the projectile.
			Vector3f velocityImpulse = up * float(wasRight? -1 : 1) * float(timeAliveMs) * 0.05f + Vector3f(1,0,0);
			float prevVel = owner->physics->velocity.Length();
			Vector3f newVel = (owner->physics->velocity + velocityImpulse).NormalizedCopy() * prevVel;
			QueuePhysics(new PMSetEntity(owner, PT_VELOCITY, newVel));
			nextWobbleMs += wobbleRand.Randi(30)+timeInMs;
			/// Set 1-bit accordinly so side-chaining changes each pulse.
			if (wasRight)
				nextWobbleMs = nextWobbleMs & ~(0x0001);
			else
				nextWobbleMs = nextWobbleMs | 0x00001;
		}
	}
	else if (weapon.type == Weapon::Type::HeatWave)
	{
		// Decay over distance / time.
		distanceTraveled = timeAliveMs * weapon.projectileSpeed * 0.001f;
		float alpha = weapon.relativeStrength = (weapon.maxRange - distanceTraveled) / weapon.maxRange;
		/// Update alpha?
		QueueGraphics(new GMSetEntityf(owner, GT_ALPHA, alpha));
		/// Adjust scale over time?
		float scale = owner->scale.x;
		scale = 1 + (1 - alpha) * weapon.linearScaling;
		QueuePhysics(new PMSetEntity(owner, PT_SET_SCALE, scale));
		if (distanceTraveled > weapon.maxRange)
		{
			// Clean-up.
			sleeping = true;
			return;
		}
	}
	else if (weapon.linearDamping < 1.f)
	{
		if (owner->Velocity().LengthSquared() < 1.f)
			sleeping = true;
	}
	if (weapon.homingFactor > 0)
	{
		// Check to see if we should reset our target lock
		if (targetLock) {
			ShipProperty * sp = targetLock->GetProperty<ShipProperty>();
			if (sp->sleeping) {
				targetLock = nullptr;
			}
		}

		// Seek closest enemy.
		// Adjust velocity towards it by the given factor, per second.
		// 1.0 will change velocity entirely to look at the enemy.
		// Values above 1.0 will try and compensate for target velocity and not just current position?
		EntitySharedPtr target = targetLock;
		if (target == nullptr)
			target = spaceShooter->level.ClosestTarget(PlayingLevelRef(), !enemy, owner->worldPosition);
		Vector3f vecToTarget;
		Vector3f targetPosition;
		Vector3f targetVelocity;
		if (target) {
			targetLock = target;
			targetPosition = target->worldPosition;
			targetVelocity = target->physics->currentVelocity;
		}
		else
			targetPosition = owner->worldPosition + Vector3f(250, 0, 0);

		// Set focus point a bit in front of the target depending on it's current speed and our estimated ETA.
		vecToTarget = targetPosition - owner->worldPosition;
		Vector3f vecToTargetNormalized = vecToTarget.NormalizedCopy();
		float velocityDotVecToTarget = owner->physics->currentVelocity.DotProduct(vecToTargetNormalized);
		float distanceToTarget = vecToTarget.Length();
		float offsetDueToMalignedDirection = 20 * (owner->physics->currentVelocity.Length() - velocityDotVecToTarget);
		float secondsToTarget = distanceToTarget / velocityDotVecToTarget + offsetDueToMalignedDirection;
		ClampFloat(secondsToTarget, 0, 100);
		targetPosition += targetVelocity * secondsToTarget;

		Vector3f vecToTargetWithOffset = targetPosition - owner->worldPosition;
		vecToTargetWithOffset.Normalize();

		Angle toTarget(vecToTargetWithOffset);
		Angle lookingAt(owner->LookAt());

		Angle toRotate = toTarget - lookingAt;

		float rotationSpeed = ClampedFloat(toRotate.Radians(), -0.5f, 0.5f) * 10.0f * weapon.homingFactor;
		std::cout << "\nSeconds to target: " << secondsToTarget << " Rotation speed: " << rotationSpeed;

		// Add some rotational velocity to address this need.
		QueuePhysics(new PMSetEntity(owner, PT_ANGULAR_VELOCITY, Vector3f(0, rotationSpeed, 0)));
		
		// + ClampedFloat(rotationSpeed, -2.5f, 2.5f)
		thrustEmitter->velocityEmitter.arcOffset = lookingAt.Radians() + PI;
		//thrustEmitter->SetParticleLifeTime((min(timeAliveMs * 0.001f, 2.0f));

		// Go t'ward it!
		//ResetVelocity();
	}
	// .. 
	if (weapon.projectilePath == Weapon::HOMING)
	{
		// Seek the closest enemy?
		assert(false);
	}
	else if (weapon.projectilePath == Weapon::SPINNING_OUTWARD)
	{
		// Right.
		// 
		// assert(false);
	}
}

void ProjectileProperty::ResetVelocity()
{
	QueuePhysics(new PMSetEntity(owner, PT_VELOCITY, direction * weapon.projectileSpeed));
}


void ProjectileProperty::CreateProjectileTraceEmitter(Vector3f initialPosition) {
	auto tmpEmitter = new SmoothedPositionParticleEmitter(initialPosition);
	tmpEmitter->newType = true;
	tmpEmitter->positionEmitter.type = EmitterType::POINT;
	tmpEmitter->velocityEmitter.type = EmitterType::POINT;

	//	tmpEmitter->type = EmitterType::POINT_DIRECTIONAL;
	tmpEmitter->SetParticlesPerSecond(1000);
	tmpEmitter->entityToTrack = owner;
	tmpEmitter->SetParticleLifeTime(0.2f);
	tmpEmitter->SetScale(0.15f);
	tmpEmitter->SetColor(defaultAlliedProjectileColor * 0.1f);
	traceEmitter = tmpEmitter->GetSharedPtr();
	Graphics.QueueMessage(new GMAttachParticleEmitter(traceEmitter, PlayingLevelRef().sparks->GetSharedPtr()));
}


void ProjectileProperty::CreateThrustEmitter(Vector3f initialPosition) {
	auto tmpEmitter = new ThrustEmitter(initialPosition);
//	tmpEmitter->type = EmitterType::POINT_DIRECTIONAL;
	tmpEmitter->SetEmissionVelocity(12.f);
	tmpEmitter->SetParticlesPerSecond(int(weapon.type == Weapon::Type::BigRockets? 1000 : 500));
	tmpEmitter->velocityEmitter.arcOffset = PI;
	tmpEmitter->velocityEmitter.arcLength = PI * 0.2f;
	tmpEmitter->velocityEmitter.weight = 3.0f;
	tmpEmitter->entityToTrack = owner;
	tmpEmitter->SetParticleLifeTime(1.0f);
	tmpEmitter->SetScale(0.15f);
	tmpEmitter->SetColor(weapon.type == Weapon::Type::BigRockets ? Vector4f(0.2f, 0.7f, 0.5f, 1.0f) : Vector4f(0.1f, 0.5f, 1.0f, 1.0f));
	tmpEmitter->SetRatioRandomVelocity(0.5f);

	thrustEmitter = tmpEmitter->GetSharedPtr();
	Graphics.QueueMessage(new GMAttachParticleEmitter(thrustEmitter, PlayingLevelRef().sparks->GetSharedPtr()));

}

void ProjectileProperty::ProcessMessage(Message * message)
{
	switch(message->type)
	{
		case MessageType::COLLISSION_CALLBACK:
		{
			if (onCollisionMessage.Length())
				MesMan.QueueMessages(onCollisionMessage);
			break;
		}
	}
}

bool ProjectileProperty::ShouldDamage(ShipPtr ship)
{
	if (penetratedTargets.Exists(ship))
		return false;
	return true;
}

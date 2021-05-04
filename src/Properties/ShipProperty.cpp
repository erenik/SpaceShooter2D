/// Emil Hedemalm
/// 2015-01-21
/// Ship property

#include "Properties/ShipProperty.h"
#include "ExplosionProperty.h"
#include "Entity/Entity.h"

#include "Physics/PhysicsManager.h"
#include "Physics/Messages/PhysicsMessage.h"
#include "Physics/PhysicsProperty.h"
#include "PhysicsLib/Shapes/Ray.h"

#include "Maps/MapManager.h"
#include "Model/ModelManager.h"
#include "TextureManager.h"

#include "Graphics/GraphicsManager.h"
#include "Graphics/Messages/GraphicsMessage.h"

#include "Input/InputManager.h"
#include "Window/AppWindowManager.h"

#include "SpaceShooter2D.h"
#include "PlayingLevel.h"

ShipProperty::ShipProperty(Ship* ship, Entity* owner)
: EntityProperty("ShipProperty", ID(), owner), ship(ship)
{
	assert(ship != nullptr);
	shouldDelete = false;
	sleeping = false;
	spawnInvulnerability = true;
//	LoadDataFrom(ship);
}

int ShipProperty::ID()
{
	return SHIP_PROP;
}

void ShipProperty::Remove()
{
	if (sleeping)
		return;
	Graphics.QueueMessage(new GMUnregisterEntity(owner));
	Physics.QueueMessage(new PMUnregisterEntity(owner));
	sleeping = true;
}
	
// Reset sleep.
void ShipProperty::OnSpawn()
{
	sleeping = false;
}
	

/// Time passed in seconds..!
void ShipProperty::Process(int timeInMs)
{
	if (sleeping)
		return;
	if (PlayingLevel::paused)
		return;

	if (spawnInvulnerability)
	{
		if (owner->worldPosition.x < PlayingLevelRef().removeInvuln)
		{
			spawnInvulnerability = false;
			/// Debug - change color of ships once they stop being invincible - should be done with a shield effect instead?
			if (false) {
				QueueGraphics(new GMSetEntityTexture(owner, DIFFUSE_MAP | SPECULAR_MAP, TexMan.GetTextureByColor(Color(255, 255, 255, 255))));
			}
		}
	}
	// Move?
	ship->Process(PlayingLevelRef(), PlayingLevelRef().playerShip, timeInMs);
}
	
/// If reacting to collisions...
void ShipProperty::OnCollision(Collision & data)
{
	// Check what we are colliding with.
	Entity* other = 0;
	if (data.one == owner)
		other = data.two;
	else if (data.two == owner)
		other = data.one;
}	

Random penetrationRand;
/// If reacting to collisions...
void ShipProperty::OnCollision(Entity* withEntity, float impactVelocity)
{
//	std::cout<<"\nShipProperty::OnCollision for entity "<<owner->name;
	if (sleeping)
	{
//		std::cout<<"\nSleeping, skipping stuffs";
		return;
	}
	Entity* other = withEntity;

	ShipProperty * sspp = (ShipProperty *) other->GetProperty(ShipProperty::ID());
	bool despawning = false;
	// Player-player collision? Sleep 'em both.
	if (sspp)
	{
		// Here you may generate some graphics effects if you want, but other than that.. don't do anything that has to do with gameplay logic.
		if (ship->onCollision.Length())
		{
			List<String> stuff = ship->onCollision.Tokenize("&");
			for (int i = 0; i < stuff.Size(); ++i)
			{
				String s = stuff[i];
				if (s == "RemoveThis")
				{
					ship->hp = 0;
					ship->Despawn(PlayingLevelRef(), false);
					despawning = true;
				}
				else 
				{
					MesMan.QueueMessages(s);
				}
			}
		}
		if (despawning)
			return;


		PlayingLevel& pl = PlayingLevelRef();

//		std::cout<<"\nCollision with ship! o.o";
		if (sspp->sleeping)
			return;

		float damageRatioBasedOnVelocity = impactVelocity * 0.1f; // 10 units per second = full damage, less => less, more => more

		// Check collision damage cooldown for if we should apply damage.
		if (ship->lastShipCollision < pl.flyTime - ship->collisionDamageCooldown)
		{
			if (!ship->Damage(pl, float(sspp->ship->collideDamage) * damageRatioBasedOnVelocity, false, DamageSource::Collision))
				ship->lastShipCollision = pl.flyTime;
		}
		// Same for the other ship.
		if (ship && sspp->ship->lastShipCollision < pl.flyTime - sspp->ship->collisionDamageCooldown)
		{
			if (!sspp->ship->Damage(pl, float(ship->collideDamage) * damageRatioBasedOnVelocity, false, DamageSource::Collision))
				sspp->ship->lastShipCollision = pl.flyTime;
		}

		// Add a temporary emitter to the particle system to add some sparks to the collision
		Vector3f position = (owner->worldPosition + other->worldPosition) * 0.5f;
		SparksEmitter * tmpEmitter = new SparksEmitter();
		tmpEmitter->newType = true;
		tmpEmitter->positionEmitter.type = EmitterType::POINT;
		tmpEmitter->positionEmitter.vec = position;
		// Set up velocity emitter direction.
		tmpEmitter->velocityEmitter.type = EmitterType::LINE_BOX;
		Vector3f vec1 = (owner->worldPosition - other->worldPosition).NormalizedCopy().CrossProduct(Vector3f(0,0,1)).NormalizedCopy();
		tmpEmitter->velocityEmitter.vec = vec1;
		tmpEmitter->velocityEmitter.vec2 = vec1.CrossProduct(Vector3f(0,0,1)).NormalizedCopy() * 0.2f;
		
		tmpEmitter->SetRatioRandomVelocity(1.0f);
		float velocity = (owner->Velocity().Length() + other->Velocity().Length()) * 0.5f;
		velocity += 1.f;
		velocity *= 0.5f;
		tmpEmitter->SetEmissionVelocity(velocity);
		tmpEmitter->constantEmission = 50 * damageRatioBasedOnVelocity;
		tmpEmitter->instantaneous = true;
		tmpEmitter->SetScale(0.05f);
		tmpEmitter->SetParticleLifeTime(1.5f);
		tmpEmitter->SetColor(Vector4f(1.f, 0.5f, 0.1f, 1.f));
		Graphics.QueueMessage(new GMAttachParticleEmitter(tmpEmitter, pl.sparks));
		return;
	}

	ProjectileProperty * pp = (ProjectileProperty *) other->GetProperty(ProjectileProperty::ID());
	if (pp && !pp->sleeping)
	{
		if (pp->ShouldDamage(ship))
		{
			// Take damage? D:
			//if (pp->penetratedTargets.Size())
			//	; // std::cout<<"\nPenetrator damaing again: "<<pp->penetratedTargets.Size();
			ship->Damage(PlayingLevelRef(), pp->weapon);
			// Check penetration rate.
			if (penetrationRand.Randf() > pp->weapon.penetration)
			{
				pp->Destroy();
			}
			else {
				pp->penetratedTargets.AddItem(ship);
			//	std::cout<<"\nPenetrated targets: "<<pp->penetratedTargets.Size();
			}
			// Play SFX!
			QueueAudio(new AMPlaySFX("sfx/"+pp->weapon.hitSFX+".wav", 1.f));
		}
		else 
		{
	//		std::cout<<"\nShould not! o.o";
		}
	}
	ExplosionProperty * exp = (ExplosionProperty*) other->GetProperty(ExplosionProperty::ID());
	if (exp && !exp->sleeping)
	{
		if (exp->ShouldDamage(ship))
		{
			ship->Damage(PlayingLevelRef(), exp->CurrentDamage(), false, DamageSource::Explosion);
			exp->damagedTargets.AddItem(ship);
		}
	}
}

bool ShipProperty::IsAllied()
{
	return ship->allied;
}

bool ShipProperty::IsEnemy() {
	return !ship->allied;
}


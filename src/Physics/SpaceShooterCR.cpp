/// Emil Hedemalm
/// 2014-07-16
/// Custom integrator for Breakout-type games.

#include "SpaceShooterCR.h"
#include "Physics/Collision/Collision.h"

#include "Properties/ShipProperty.h"
#include "Properties/ProjectileProperty.h"

/// Returns false if the colliding entities are no longer in contact after resolution.
bool SpaceShooterCR::ResolveCollision(Collision & c)
{
	// OK.... 
	// Check stuff.
	if (c.collisionNormal.MaxPart() == 0)
		return false;
    

	if  (c.dynamicEntities.Size() == 0)
	{
		c.ExtractData();
	}

	/// Wall? just reflect velocity so we're not going into the wall anymore.
	if (c.staticEntities.Size() || c.kinematicEntities.Size())
	{
		Entity* dynamic = c.dynamicEntities[0];
		Entity* staticEntity;
		if (c.staticEntities.Size())
			staticEntity = c.staticEntities[0];
		else
			staticEntity = c.kinematicEntities[0];

		PhysicsProperty * dp = dynamic->physics;
		Vector3f & velocity = dp->velocity;
		// Should resolve!
		bool resolve = true;
				
		if (dynamic->physics->noCollisionResolutions ||
			staticEntity->physics->noCollisionResolutions)
			resolve = false;
		// p=p
		if (resolve)
		{
			// Flag it as resolved.
			c.resolved = true;
		}
		if (resolve)
		{
			// Notify both entities of the collision that just occured.
			c.one->OnCollision(c);
			c.two->OnCollision(c);
		}
		return true;
	}
	// Two dynamic entities.
	else 
	{
		// Apply some velocities from each other (assuming ship-ship collission)?
		auto shipOne = c.one->GetProperty<ShipProperty>();
		auto shipTwo = c.two->GetProperty<ShipProperty>();
		if (shipOne && shipTwo) {
			auto physicsOne = c.one->physics;
			auto physicsTwo = c.two->physics;
			// Not actually colliding into each other anymore? Then ignore.
			if (c.collisionVelocity < 0) {
				return true;
			}
			float massDifferenceMultiplier = physicsOne->mass / physicsTwo->mass;
			float invertedMassDifferenceMultiplier = 1 / massDifferenceMultiplier;
			float totalMultipliersDivider = (physicsOne->elasticity + physicsTwo->elasticity) / (massDifferenceMultiplier + invertedMassDifferenceMultiplier);
			massDifferenceMultiplier *= totalMultipliersDivider;
			invertedMassDifferenceMultiplier *= totalMultipliersDivider;
			// Gain +1 velocity away from the collision.
			physicsOne->velocity += (-c.collisionNormal).NormalizedCopy() * c.collisionVelocity * invertedMassDifferenceMultiplier;
			physicsOne->smoothedVelocity = physicsOne->velocity;

			physicsTwo->velocity += (c.collisionNormal).NormalizedCopy() * c.collisionVelocity * massDifferenceMultiplier;
			physicsTwo->smoothedVelocity = physicsTwo->velocity;
		}
		auto projProp = c.one->GetProperty<ProjectileProperty>();
		if (!projProp)
			projProp = c.two->GetProperty<ProjectileProperty>();
		if (projProp) {
			auto ship = shipOne ? shipOne : shipTwo;
			// Apply a velocity-impulse based on the damage it would have inflicted.
			auto shipEntity = ship->owner;
			shipEntity->physics->velocity += (projProp->owner->physics->velocity).NormalizedCopy() * projProp->weapon.damage * 0.05f * shipEntity->physics->inverseMass;
			shipEntity->physics->smoothedVelocity = shipEntity->physics->velocity;
		}

		// Notify both entities of the collision that just occured.
		c.one->OnCollision(c);
		c.two->OnCollision(c);
	}

	return false;
}



/// Resolves collisions.
int SpaceShooterCR::ResolveCollisions(List<Collision> collisions)
{
	/// sup.
	for (int i = 0; i < collisions.Size(); ++i)
	{
		ResolveCollision(collisions[i]);
	}
	return collisions.Size();
}

/// Emil Hedemalm
/// 2015-01-21
/// Integrator

#include "SSIntegrator.h"
#include "SpaceShooter2D.h"
#include "PlayingLevel.h"
#include "Properties/LevelProperty.h"
#include "File/LogFile.h"

extern EntitySharedPtr playerEntity;

SSIntegrator::SSIntegrator(float zPlane)
{
	constantZ = zPlane;
}


void SSIntegrator::IntegrateDynamicEntities(List< std::shared_ptr<Entity> > & dynamicEntities, float timeInSeconds)
{
	EntitySharedPtr levelEntity = LevelEntity->owner;
	auto playingFieldHalfSize = PlayingLevel::playingFieldHalfSize;
	if (levelEntity)
	{
		frameMin = levelEntity->worldPosition - playingFieldHalfSize;
		frameMax = levelEntity->worldPosition + playingFieldHalfSize;
	}
	static int shipID = ShipProperty::ID();
	Timer timer;
	timer.Start();
	for (int i = 0; i < dynamicEntities.Size(); ++i)
	{
		EntitySharedPtr dynamicEntity = dynamicEntities[i];
		IntegrateVelocity(dynamicEntity, timeInSeconds);
		/// Apply bounding for player in other manner...
		/// Check if player
//		ShipProperty * sp = (ShipProperty*) dynamicEntity->GetProperty(shipID);
		// If so, limit to inside the radiusiusius
		if (dynamicEntity == PlayerShipEntity())
		{
	//		std::cout<<"\nShip property: "<<sp<<" ID "<<sp->GetID()<<" allied: "<<sp->ship->allied;
			/// Adjusting local position may not help ensuring entity is within bounds for child entities.
//			assert(dynamicEntity->parent == 0);
			if (dynamicEntity->parent != 0)
			{
				std::cout<<"\nDE: "<<dynamicEntity->name;
				return;
			}
			Vector3f & position = dynamicEntity->localPosition;
			ClampFloat(position[0], frameMin[0], frameMax[0]);
			ClampFloat(position[1], frameMin[1], frameMax[1]);		
		}
	}
	timer.Stop();
	integrationTimeMs = (int) timer.GetMs();
	
	timer.Start();
	RecalculateMatrices(dynamicEntities);
	timer.Stop();
	entityMatrixRecalcMs = (int)timer.GetMs();
}



/** All entities sent here should be fully kinematic! 
	If not subclassed, the standard IntegrateEntities is called.
*/
void SSIntegrator::IntegrateKinematicEntities(List< std::shared_ptr<Entity> > & kinematicEntities, float timeInSeconds)
{
	for (int i = 0; i < kinematicEntities.Size(); ++i)
	{
		EntitySharedPtr kinematicEntity = kinematicEntities[i];
		IntegrateVelocity(kinematicEntity, timeInSeconds);
	}
	RecalculateMatrices(kinematicEntities);
}
	
float velocitySmoothingLast = 0.f;
float smoothingFactor = 0.f;

void SSIntegrator::IntegrateVelocity(EntitySharedPtr forEntity, float timeInSeconds)
{
	PhysicsProperty * pp = forEntity->physics;
	if (pp->paused)
		return;
	Vector3f & localPosition = forEntity->localPosition;
	Vector3f oldLocalPos = localPosition;
	Vector3f & velocity = pp->velocity;
	Vector3f oldVelocity = velocity;
	/// For linear damping.
	if (pp->linearDamping != 1.0f) {
		float linearDamp = pow(pp->linearDamping, timeInSeconds);
		velocity *= linearDamp;
	}

	Vector3f totalAcceleration;
	if (pp->relativeAcceleration.MaxPart() != 0)
	{
		Vector3f relAcc = pp->relativeAcceleration;
		relAcc.z *= -1;
		Vector3f worldAcceleration = forEntity->rotationMatrix.Product(relAcc);
		totalAcceleration += worldAcceleration;
		assert(totalAcceleration.x == totalAcceleration.x);
	}
	velocity += totalAcceleration * timeInSeconds;

	pp->currentVelocity = velocity;
	if (velocitySmoothingLast != pp->velocitySmoothing)
	{
		smoothingFactor = pow(pp->velocitySmoothing, timeInSeconds);
		velocitySmoothingLast = pp->velocitySmoothing;
	}
	pp->smoothedVelocity = pp->smoothedVelocity * smoothingFactor + pp->currentVelocity * (1 - smoothingFactor);
//	forEntity->position += forEntity->physics->velocity * timeInSeconds;
	localPosition += pp->smoothedVelocity * timeInSeconds;
	if (pp->relativeVelocity.MaxPart())
	{
		Vector3f velocity = forEntity->rotationMatrix * pp->relativeVelocity;
		localPosition += velocity * timeInSeconds;
	}
	
	if (localPosition.IsInfinite()) {
		LogPhysics("Position out of bounds. Position previously: "+ VectorString(oldLocalPos)+ " old velocity: "+VectorString(oldVelocity), ERROR);
		assert(!localPosition.IsInfinite());
	}

	if (pp->angularVelocity.MaxPart())
	{
		forEntity->rotation += pp->angularVelocity * timeInSeconds;
//		std::cout << "\nAccelerating object with rotation: " + VectorString(forEntity->rotation)+" velocity: "+ VectorString(velocity);
		forEntity->hasRotated = true;
		//forEntity->RecalcRotationMatrix(true);
	}

	if (constantZ)
	{
		localPosition[2] = constantZ;
		forEntity->physics->velocity[2] = 0;
	}

	// Force rot to follow vel.
	if (pp->faceVelocityDirection)
	{

	}
}

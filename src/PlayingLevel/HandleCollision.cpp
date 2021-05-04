/** Handles collission between ships, rules depending on how they are set up.
	2020-04-28 Emil Hedemalm
*/

#include "HandleCollision.h"
#include "Entity/Entity.h"
#include "Base/Ship.h"
#include "Properties/ShipProperty.h"

#include "Physics/Messages/CollisionCallback.h"


void HandleCollision(Ship* playerShip, List<Entity*> shipEntities, CollisionCallback* message) {

	CollisionCallback* cc = (CollisionCallback*)message;
	Entity* one = cc->one;
	Entity* two = cc->two;
#define SHIP 0
#define PROJ 1
	//			std::cout<<"\nColCal: "<<cc->one->name<<" & "<<cc->two->name;

	Entity* shipEntity1 = NULL;
	Entity* other = NULL;
	int oneType = (one == playerShip->entity || shipEntities.Exists(one)) ? SHIP : PROJ;
	int twoType = (two == playerShip->entity || shipEntities.Exists(two)) ? SHIP : PROJ;
	int types[5] = { 0,0,0,0,0 };
	++types[oneType];
	++types[twoType];
	//	std::cout<<"\nCollision between "<<one->name<<" and "<<two->name;

	if (oneType == SHIP)
	{
		ShipProperty* shipProp = (ShipProperty*)one->GetProperty(ShipProperty::ID());
		if (shipProp)
			shipProp->OnCollision(two, cc->impactVelocity);
	}
	if (twoType == SHIP)
	{
		ShipProperty* shipProp = (ShipProperty*)two->GetProperty(ShipProperty::ID());
		if (shipProp)
			shipProp->OnCollision(one, cc->impactVelocity);
	}
}

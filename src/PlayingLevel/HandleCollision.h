/** Handles collission between ships, rules depending on how they are set up.
	2020-04-28 Emil Hedemalm
*/

#pragma once

#include "Base/Ship.h"
#include "Entity/Entity.h"

class CollisionCallback;

void HandleCollision(ShipPtr playerShip, List<EntitySharedPtr> shipEntities, CollisionCallback* message);

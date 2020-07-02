/// Emil Hedemalm
/// 2015-01-21
/// Ship property

#ifndef SHIP_PROPERTY_H
#define SHIP_PROPERTY_H

#include "../Base/Ship.h"
#include "MathLib.h"
#include "Entity/EntityProperty.h"
#include "Time/Time.h"

class ShipProperty : public EntityProperty
{
	friend class Ship;
public:
	/// Reference to the game and this property's owner.
	ShipProperty(ShipPtr ship, EntitySharedPtr owner);
	// Static version.
	static int ID();

	const ShipPtr GetShip() const {return ship;};
	void Remove();
	// Reset sleep.
	void OnSpawn();
	
	virtual void Process(int timeInMs);
	/// If reacting to collisions...
	virtual void OnCollision(Collision & data);

	/// If reacting to collisions...
	virtual void OnCollision(EntitySharedPtr withEntity);

	// Since enemies go from right to left..
	bool IsAllied();
	/// When deaded...
	bool sleeping;
	/// hm..
	bool shouldDelete;

	/// o.o
//	void LoadDataFrom(ShipPtr ship);


	// False by default. If true will use default behaviour of following the mouse.
	bool useMouseInput;

	/// o.o
//	ShipPtr ship;

private:
	
	ShipPtr ship;

	bool spawnInvulnerability; // Default true at start.
	long millisecondsPassedSinceLastFire;	
};

#endif









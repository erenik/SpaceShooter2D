/***
	For Level entity handling, ensuring it is registered to physics, adapts size based on events in the game, etc.
	Emil Hedemalm
	2020-04-29
*/

#pragma once

#include "Entity/Entity.h"
#include "Entity/EntityProperty.h"

class LevelProperty : public EntityProperty {
public:
	/// Default annulizing constructor.
	LevelProperty(EntitySharedPtr owner);

	/// Time passed in seconds..!
	virtual void Process(int timeInMs) override;

};

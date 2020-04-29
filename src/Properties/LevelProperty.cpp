#include "Properties/LevelProperty.h"

/// Default annulizing constructor.
LevelProperty::LevelProperty(EntitySharedPtr owner)
	: EntityProperty("LevelProperty", 5, owner)  
{
}


/// Time passed in seconds..!
void LevelProperty::Process(int timeInMs) { 
	assert(owner->registeredForPhysics);
}

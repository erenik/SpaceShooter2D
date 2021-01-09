/***
	For Level entity handling, ensuring it is registered to physics, adapts size based on events in the game, etc.
	Emil Hedemalm
	2020-04-29
*/

#pragma once

#include "Entity/Entity.h"
#include "Entity/EntityProperty.h"

#define LevelEntity LevelProperty::GetSingleton()

class LevelProperty : public EntityProperty {
	static LevelProperty* singleton;
	static EntitySharedPtr levelEntity;
	/// Default annulizing constructor.
	LevelProperty(EntitySharedPtr owner, Vector2f playingFieldSize, float playingFieldPadding);
public:
	static LevelProperty* GetSingleton() { return singleton; };

	// Creates the entity and property, as well as darkness entities to show where the player cannot proceed beyond.
	static EntitySharedPtr Create(Vector2f playingFieldSize, float playingFieldPadding, Camera*  levelCamera);
	void MoveTo(Vector3f position);
	void SetVelocity(Vector3f velocity);
	void CreateBlackness();
	void CreateBackground();
	void ToggleBlackness();
	Vector3f Velocity();

	float DespawnDown();
	float DespawnUp();

	/// Time passed in seconds..!
	virtual void Process(int timeInMs) override;

private:
	float playingFieldPadding;
	Vector2f playingFieldSize, playingFieldHalfSize;
	/// 4 entities constitude the blackness.
	List< std::shared_ptr<Entity> > blacknessEntities;


};

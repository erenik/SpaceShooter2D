/** 
	For Level entity handling, ensuring it is registered to physics, adapts size based on events in the game, etc.
	Emil Hedemalm
	2020-04-29
*/

#include "Properties/LevelProperty.h"
#include "Physics/PhysicsManager.h"
#include "Physics/PhysicsProperty.h"
#include "StateManager.h"
#include "Entity/EntityManager.h"
#include "Model/ModelManager.h"
#include "TextureManager.h"
#include "Graphics/GraphicsManager.h"
#include "Graphics/Messages/GMEntity.h"
#include "Graphics/Messages/GMCamera.h"
#include "Graphics/Messages/GMSetEntity.h"
#include "Window/AppWindowManager.h"
#include "Viewport.h"

LevelProperty* LevelProperty::singleton = nullptr;
EntitySharedPtr LevelProperty::levelEntity = nullptr;

/// Default annulizing constructor.
LevelProperty::LevelProperty(EntitySharedPtr owner, Vector2f playingFieldSize, float playingFieldPadding)
	: EntityProperty("LevelProperty", 5, owner), playingFieldSize(playingFieldSize), playingFieldPadding(playingFieldPadding)
{
	singleton = this;
	levelEntity = owner;
}


EntitySharedPtr LevelProperty::Create(Vector2f playingFieldSize, float playingFieldPadding, Camera * levelCamera)
{
	levelEntity = EntityMan.CreateEntity("LevelEntity", NULL, NULL);
	LevelProperty* lp = new LevelProperty(levelEntity, playingFieldSize, playingFieldPadding);
	levelEntity->properties.Add(lp);
	levelEntity->localPosition = Vector3f();

	PhysicsProperty* pp = levelEntity->physics = new PhysicsProperty();
	pp->collisionsEnabled = false;
	pp->type = PhysicsType::KINEMATIC;

	lp->CreateBlackness();
	lp->CreateBackground();

	// Finalize details before registering.
	levelEntity->localPosition = Vector3f();
	QueuePhysics(new PMRegisterEntities(levelEntity));
	//QueueGraphics(new GMRegisterEntities(levelEntity));
	return levelEntity;
}

void LevelProperty::SetVelocity(Vector3f velocity) {
	QueuePhysics(new PMSetEntity(owner, PT_VELOCITY, velocity));
	owner->physics->velocity = velocity;
}

void LevelProperty::MoveTo(Vector3f position) {
	PhysicsMan.QueueMessage(new PMSetEntity(levelEntity, PT_POSITION, position));
}

/// Time passed in seconds..!
void LevelProperty::Process(int timeInMs) { 
	assert(owner->registeredForPhysics);
}

void LevelProperty::CreateBlackness() {
	playingFieldHalfSize = playingFieldSize / 2;

	/// Add blackness to track the level entity.
	for (int i = 0; i < 4; ++i)
	{
		EntitySharedPtr blackness = EntityMan.CreateEntity("Blackness" + String(i), ModelMan.GetModel("sprite.obj"), TexMan.GetTexture("0x0A"));
		float scale = 150.f;
		float halfScale = scale * 0.5f;
		blackness->scale = scale * Vector3f(1, 1, 1);
		Vector3f position;
		position[2] = 5.f; // Between game plane and camera
		switch (i)
		{
		case 0: position[0] += playingFieldHalfSize[0] + halfScale + playingFieldPadding; break;
		case 1: position[0] -= playingFieldHalfSize[0] + halfScale + playingFieldPadding; break;
		case 2: position[1] += playingFieldHalfSize[1] + halfScale + playingFieldPadding; break;
		case 3: position[1] -= playingFieldHalfSize[1] + halfScale + playingFieldPadding; break;
		}
		blackness->localPosition = position;
		levelEntity->AddChild(blackness);
		blacknessEntities.Add(blackness);
	}
	// Register blackness entities for rendering.
	QueueGraphics(new GMRegisterEntities(blacknessEntities));
}

void LevelProperty::CreateBackground() {
	Texture * texture = TexMan.GetTexture("bgs/purple_space");
	EntitySharedPtr backgroundEntity = EntityMan.CreateEntity("Background", ModelMan.GetModel("sprite.obj"), texture);
	// Ensure it covers the playing field.
	Vector2f scale = Vector2f(texture->Width() / float(texture->Height()), 1);
	Vector2f requiredScaleFactor = playingFieldSize.ElementDivision(scale);
	scale *= requiredScaleFactor.MaxPart() + 2;
	backgroundEntity->scale = Vector3f(scale, 1);
	Vector3f position(0, 0, -5);
	backgroundEntity->localPosition = position;
	levelEntity->AddChild(backgroundEntity);
	QueueGraphics(new GMRegisterEntity(backgroundEntity));
}

void LevelProperty::ToggleBlackness() {
	if (blacknessEntities.Size())
	{
		bool visible = blacknessEntities[0]->IsVisible();
		GraphicsMan.QueueMessage(new GMSetEntityb(blacknessEntities, GT_VISIBILITY, !visible));
		Viewport* viewport = MainWindow()->MainViewport();
		viewport->renderGrid = visible;
	}
}

Vector3f LevelProperty::Velocity() {
	return owner->physics->velocity;
}

float despawnLimit = 10.0f;

float LevelProperty::DespawnDown() {
	return owner->worldPosition.y - playingFieldHalfSize.y - despawnLimit;
}
float LevelProperty::DespawnUp() {
	return owner->worldPosition.y + playingFieldHalfSize.y + despawnLimit;
}


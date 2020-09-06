/// Emil Hedemalm
/// 2015-01-21
/// Level.

#include "../SpaceShooter2D.h"
#include "SpawnGroup.h"
#include "Text/TextManager.h"
#include "LevelMessage.h"
#include "Explosion.h"
#include "File/LogFile.h"
#include "Message/MathMessage.h"
#include "StateManager.h"
#include "../Properties/ExplosionProperty.h"
#include "PlayingLevel.h"
#include "OS/Sleep.h"

Camera * levelCamera = NULL;


Level * activeLevel = NULL;

Level::Level()
{
	height = 20.f;
	endCriteria = NO_MORE_ENEMIES;
	levelCleared = false;
	activeLevelMessage = 0;
}

Level::~Level()
{
	spawnGroups.ClearAndDelete();
	messages.ClearAndDelete();
	enemyShips.Clear();
	alliedShips.Clear();
}

/// Starts BGM, starts clocks/timers if any, etc.
void Level::OnEnter()
{
	// Play music?
	if (music.Length())
	{
		AudioMan.QueueMessage(new AMPlay(AudioType::BGM, music, 1.f));
	}

	// Set Ambient lighting and some star lighting?
	LogMain("Entering level, updating Lighting", INFO);
	Lighting lighting = Lighting();
	lighting.SetAmbient(1.0, 1.0, 1.0, 1.0);
	QueueGraphics(new GMSetLighting(lighting));

}

// Used for player and camera. Based on millisecondsPerPixel.
Vector3f Level::BaseVelocity()
{
	return Vector3f(1,0,0) * 4;
}

/// Creates player entity within this level. (used for spawning)
EntitySharedPtr Level::SpawnPlayer(PlayingLevel& playingLevel, ShipPtr playerShip, ConstVec3fr atPosition)
{	
	EntitySharedPtr entity = playerShip->Spawn(atPosition, 0, playingLevel);
	playingLevel.OnPlayerSpawned();
	return entity;
}

void Level::SetupCamera()
{
	if (!levelCamera)
		levelCamera = CameraMan.NewCamera("LevelCamera");
	// offset to the level entity, just 10 in Z.
	levelCamera->position = Vector3f(0,0,10);
	levelCamera->rotation = Vector3f(0,0,0);
	levelCamera->trackingMode = TrackingMode::ADD_POSITION;
	spaceShooter->ResetCamera();
//	levelCamera->Begin(Direction::RIGHT); // Move it..!
//	levelCamera->
	GraphicsMan.QueueMessage(new GMSetCamera(levelCamera));
}

void Level::Process(PlayingLevel& playingLevel, int timeInMs)
{

	/// Check for suspected buggy ships.
	static int second = 0;
	second += timeInMs;
	if (second >500)
	{
		second = 0;
		List<ShipPtr> spookyShips;
		int playerColliding = 0,
			enemyCategory = 0;
		
		int formations[(int)Formation::FORMATIONS];
		memset(formations, 0, sizeof(int) * (int)Formation::FORMATIONS);
		for (int i = 0; i < ships.Size(); ++i)
		{
			ShipPtr ship = ships[i];
			if (ship->entity)
			{
				if (!ship->entity->registeredForPhysics)
				{
					spookyShips.AddItem(ship);
					++formations[(int)ship->spawnGroup->formation];
				}
			}
			if (!ship->entity)
				continue;
			if (ship->entity->physics->collisionFilter | CC_ALL_PLAYER)
				++playerColliding;
			if (ship->entity->physics->collisionCategory | CC_ALL_ENEMY)
				++enemyCategory;
		}
		if (spookyShips.Size() > 0)
			LogMain(String("Spooky ships found: ")+spookyShips.Size()+" playerFilter: "+playerColliding+" enemyCategory: "+enemyCategory+" out of "+ships.Size()+" ships.", INFO);
		for (int i = 0; i < (int)Formation::FORMATIONS; ++i)
		{
			if (formations[i] > 0)
				std::cout<<"\n in formation "<<i<<": "<<formations[i];
		}
	}


	activeLevel = this;

	PlayingLevelRef().removeInvuln = playingLevel.levelEntity->worldPosition[0] + playingLevel.playingFieldHalfSize[0] + playingLevel.playingFieldPadding + 1.f;
	assert(PlayingLevelRef().removeInvuln > -1000);
	PlayingLevelRef().spawnPositionRight = PlayingLevelRef().removeInvuln + 10.f;
	assert(PlayingLevelRef().spawnPositionRight > -1000);
	PlayingLevelRef().despawnPositionLeft = playingLevel.levelEntity->worldPosition[0] - playingLevel.playingFieldHalfSize[0] - 1.f;
	assert(PlayingLevelRef().despawnPositionLeft > -1000);
	PlayingLevelRef().despawnPositionRight = PlayingLevelRef().spawnPositionRight + 100.0f;

	playingLevel.flyTime.AddMs(timeInMs);

	ProcessLevelMessages(playingLevel.levelTime);

	/// Clearing the level
	if (LevelCleared(playingLevel))
	{
		spaceShooter->OnLevelCleared();
		return; // No more processing if cleared?
	}
	if (playingLevel.GameTimePaused()) {
		LogMain("Game time is paused", EXTENSIVE_DEBUG);
		SleepThread(10);
		return;
	}
	else
	{
		playingLevel.levelTime.AddMs(timeInMs);
	}

	/// Process active explosions. No.
	/*
	for (int i = 0; i < explosions.Size(); ++i)
	{
		Explosion * exp = explosions[i];
		float detonationVelocity = 10.f;
		exp->currentRadius += timeInMs * 0.001f * detonationVelocity;
		bool finalTurn = false;
		if (exp->currentRadius > exp->weapon.explosionRadius)
		{
			exp->currentRadius = exp->weapon.explosionRadius;
			finalTurn = true;
		}
		/// Apply damage to nearby ships (if any)
		List<float> distances;
		List<ShipPtr> relShips = GetShipsAtPoint(exp->position, exp->currentRadius, distances);
		for (int j = 0; j < relShips.Size(); ++j)
		{
			ShipPtr ship = relShips[j];
			if (exp->affectedShips.Exists(ship))
				continue;
			float amount = exp->weapon.damage;
			/// Decrease damage linearly with distance to center of explosion?
			float dist = distances[j];
			float relDist = dist / exp->weapon.explosionRadius;
			float relDmg = 1 - relDist;
			float finalDmg = relDmg * amount;
			exp->totalDamageInflicted += finalDmg;
			ship->Damage(finalDmg, false);
			exp->affectedShips.AddItem(ship); // Ensure no double-triggering.
			std::cout<<"\nAffected ships: "<<exp->affectedShips.Size()<<" total dmg inflicted: "<<exp->totalDamageInflicted;
		}

		if (finalTurn)
		{
			explosions.RemoveItem(exp);
			delete exp;
			--i;
		}
	}
	*/

	/// Check spawn-groups to spawn.
	for (int i = 0; i < spawnGroups.Size(); ++i)
	{
		SpawnGroup * sg = spawnGroups[i];
		if (sg->DefeatedOrDespawned())
			continue;

		Time spawnTime = sg->SpawnTime();
		int msToSpawn = (int)(sg->SpawnTime() - playingLevel.levelTime).Milliseconds();
		if (msToSpawn > 0)
			continue;
		if (spawnTime.Milliseconds() < 100) {
			LogMain("Spawn group spawn time under 100 ms, intended?", WARNING);
		}

		if (!sg->FinishedSpawning())
		{
			LogMain("Spawning group " + sg->name + " at time " + spawnTime, INFO);
			playingLevel.SetLastSpawnGroup(sg);
			sg->Spawn(playingLevel);
			continue;
		}
	}
}

// Dialogue, tutorials
void Level::ProcessLevelMessages(Time levelTime) {
	/// Check messages.
	if (messages.Size())
	{
		for (int i = 0; i < messages.Size(); ++i)
		{
			LevelMessage* lm = messages[i];

			if (lm->hidden)
				continue;

			// If being displayed, check if we should stop displaying it.
			if (lm->displayed && lm->stopTime > levelTime)
				HideLevelMessage(lm);

			if (lm->startTime > levelTime) // Not time yet.
				continue;

			if (lm->displayed) // Already displayed.
				continue;

			if (activeLevelMessage)
				continue;
			if (lm->Trigger(PlayingLevelRef(), this))
				activeLevelMessage = lm;
		}
	}
}

void Level::ProcessMessage(PlayingLevel& playingLevel, Message * message)
{
	String & msg = message->msg;
	switch(message->type)
	{

		case MessageType::STRING:
		{
 			if (msg.StartsWith("GenerateLevel"))
			{
				GenerateLevel(PlayingLevelRef(), msg);
			}
			if (msg == "ListPhysicalEntities")
			{
				Entities entities = PhysicsMan.GetEntities();
				for (int i = 0; i < entities.Size(); ++i)
				{
					std::cout<<"\nPhysical entities: "<<entities[i]->name<<" "<<entities[i]->worldPosition;
				}
/*				GraphicsMan.Re
				for (int i = 0; i < entities.Size(); ++i)
				{
					std::cout<<"\nPhysical entities: "<<entities[i]->name;
				}*/
			}
			if (msg == "EndLevel")
			{
				levelCleared = true;
			}
			if (msg == "PauseGameTime")
				playingLevel.gameTimePausedDueToScriptableMessage = true;
			if (msg == "ResumeGameTime")
				playingLevel.gameTimePausedDueToScriptableMessage = false;
			if (msg == "ResetFailedToSurvive")
				playingLevel.failedToSurvive = false;
			break;		
		}
	}
}

void Level::ProceedMessage()
{
	if (activeLevelMessage)
		activeLevelMessage->Hide(PlayingLevelRef());
	activeLevelMessage = 0;
}

void Level::SetTime(Time newTime)
{
	PlayingLevelRef().SetTime(newTime);
	OnLevelTimeAdjusted(newTime);
}

/// enable respawing on shit again.
void Level::OnLevelTimeAdjusted(Time levelTime)
{
	for (int i = 0; i < spawnGroups.Size(); ++i)
	{
		SpawnGroup * sg = spawnGroups[i];
		if (sg->SpawnTime() > levelTime)
			spawnGroups[i]->ResetForSpawning();
	}
	for (int i = 0; i < messages.Size(); ++i)
	{
		LevelMessage * lm = messages[i];
		if (lm->startTime < levelTime) // Mark earlier messages as displayed
		{
			lm->displayed = lm->hidden = true;
			if (lm->dontSkip)
				lm->Trigger(PlayingLevelRef(), this);
		}
		else { // And future ones as yet to display.
			lm->displayed = lm->hidden = false;
		}
	}
}


// Check spawn groups.
bool Level::LevelCleared(PlayingLevel& playingLevel)
{
//	ferewr
	switch(endCriteria)
	{
		case NEVER:
			return false;
		case NO_MORE_ENEMIES:
			if (playingLevel.levelTime.Seconds() < 3)
				return false;
			if (playingLevel.shipEntities.Size() > this->PlayerShips(playingLevel).Size())
				return false;
			if (!FinishedSpawning())
				return false;
			if (messages.Size())
				return false;
			return true;
		case EVENT_TRIGGERED:
			return levelCleared;
		default:
			return false;
	}
	return false;
}

EntitySharedPtr Level::ClosestTarget(PlayingLevel & playingLevel, bool enemy, ConstVec3fr position)
{
	if (!enemy)
	{
		return playingLevel.playerShip->entity;
	}
	EntitySharedPtr closest = NULL;
	float closestDist = 100000.f;
	for (int i = 0; i < playingLevel.shipEntities.Size(); ++i)
	{
		EntitySharedPtr e = playingLevel.shipEntities[i];
		ShipProperty * sp = e->GetProperty<ShipProperty>();
		if (sp->IsEnemy() != enemy)
			continue;

		float dist = (e->worldPosition - position).LengthSquared();
		if (dist < closestDist)
		{
			closest = e;
			closestDist = dist;
		}
	}
	return closest;
}

#include "PhysicsLib/EstimatorFloat.h"

/// o.o'
void Level::Explode(Weapon & weapon, EntitySharedPtr causingEntity, bool enemy)
{
	EntitySharedPtr explosionEntity = EntityMan.CreateEntity("ExplosionEntity", ModelMan.GetModel("Sphere"), 0 /*TexMan.GetTexture("0xFFFF")*/);
	ExplosionProperty * explosionProperty = new ExplosionProperty(weapon, explosionEntity);
	explosionEntity->properties.AddItem(explosionProperty);
	explosionProperty->weapon = weapon;
	explosionProperty->enemy = enemy;
	explosionProperty->player = !enemy;
	explosionProperty->duration = 1000; // MS.

	PhysicsProperty * prop = new PhysicsProperty();
	prop->collisionCategory = enemy? CC_ENEMY_EXPL : CC_PLAYER_EXPL;
	prop->collisionFilter = enemy? CC_ALL_PLAYER : CC_ALL_ENEMY;
	prop->collisionCallback = true;
	prop->physicalRadius = 5.f;
	prop->velocity = causingEntity->Velocity() * 0.5f;
	prop->type = PhysicsType::DYNAMIC;
	prop->noCollisionResolutions = true;
	explosionEntity->physics = prop;

	explosionEntity->localPosition = causingEntity->worldPosition;
	explosionEntity->RecalculateMatrix();

	/// Add physics thingy straight away.
	EstimatorFloat * estimator = new EstimatorFloat();
	estimator->variablesToPutResultTo.Add(&explosionEntity->scale.x, &explosionEntity->scale.y, &explosionEntity->scale.z);
	estimator->AddStateMs(0.1f, 0);
	estimator->AddStateMs(weapon.explosionRadius, explosionProperty->duration);
	prop->estimationEnabled = true;
	prop->estimators.AddItem(estimator);
	MapMan.AddEntity(explosionEntity, false, true);
}

List<ShipPtr> Level::GetShipsAtPoint(ConstVec3fr position, float maxRadius, List<float> & distances)
{
	List<ShipPtr> relevantShips;
	distances.Clear();
	float maxDist = maxRadius;
	for (int i = 0; i < ships.Size(); ++i)
	{
		ShipPtr ship = ships[i];
		if (ship->destroyed || !ship->spawned)
			continue;
		if (ship->entity == 0)
			continue;
		float dist = (ship->entity->worldPosition - position).Length();
		float radius = ship->entity->Radius();
		float distMinRadius = dist - radius;
		if (distMinRadius > maxDist)
			continue;
		relevantShips.AddItem(ship);
		distances.AddItem(distMinRadius);
	}
	return relevantShips;
}

// # of spawn groups yet to start spawning. (may be 0 while spawning last one or enemies still on screen).
int Level::SpawnGroupsRemaining() {
	int num = 0;
	for (int i = 0; i < spawnGroups.Size(); ++i) {
		SpawnGroup * sg = spawnGroups[i];
		if (sg->FinishedSpawning())
			continue;
		if (sg->DefeatedOrDespawned())
			continue;
		++num;
	}
	return num;
}

// Null if none after this one.
SpawnGroup* Level::NextSpawnGroup() {
	SpawnGroup * nextOneToSpawn = nullptr;
	for (int i = 0; i < spawnGroups.Size(); ++i) {
		SpawnGroup * spawnGroup = spawnGroups[i];
		if (spawnGroup->DefeatedOrDespawned())
			continue;
		if (spawnGroup->FinishedSpawning())
			continue;
		if (spawnGroup->shipsSpawned == 0) {
			if (nextOneToSpawn == nullptr)
				nextOneToSpawn = spawnGroup;
			else if (spawnGroup->SpawnTime() < nextOneToSpawn->SpawnTime())
				nextOneToSpawn = spawnGroup;
		}
	}
	return nextOneToSpawn;
}


void Level::RemoveRemainingSpawnGroups()
{
	for (int i = 0; i < spawnGroups.Size(); ++i)
	{
		SpawnGroup * sg = spawnGroups[i];
		sg->SetFinishedSpawning();
	}
}

void Level::SetSpawnGroupsFinishedAndDefeated(Time beforeLevelTime) {
	for (int i = 0; i < spawnGroups.Size(); ++i)
	{
		SpawnGroup * sg = spawnGroups[i];
		if (sg->SpawnTime() < beforeLevelTime)
		{
			sg->SetFinishedSpawning();
			sg->SetDefeated();
		}
	}
}

void Level::RemoveExistingEnemies(PlayingLevel& playingLevel)
{
	String lg;
	for (int i = 0; i < ships.Size(); ++i)
	{
		ShipPtr ship = ships[i];
		lg += ship->name+" ";
		ship->Despawn(playingLevel, false);
	}
	LogMain("Deleting entities "+lg, INFO);

	Sleep(50);
	for (int i = 0; i < ships.Size(); ++i)
	{
		ShipPtr s = ships[i];
		s->spawnGroup = 0;
	}
	ships.Clear();
}

void Level::HideLevelMessage(LevelMessage * levelMessage) {
	// Retain sorting.
	if (levelMessage == nullptr)
		levelMessage = activeLevelMessage;
	if (levelMessage == nullptr) {
		LogMain("No message to hide", ERROR);
		return;
	}
	levelMessage->Hide(PlayingLevelRef());
	if (activeLevelMessage == levelMessage)
	{
		activeLevelMessage = 0;
	}
}

LevelMessage * Level::GetMessageWithTextId(String id) {
	for (int i = 0; i < messages.Size(); ++i)
		if (messages[i]->textID == id)
			return messages[i];
	return nullptr;
}


List<ShipPtr> Level::PlayerShips(PlayingLevel& playingLevel)
{
	List<ShipPtr> playerShips;
	playerShips.AddItem(playingLevel.playerShip);
	return playerShips;
}

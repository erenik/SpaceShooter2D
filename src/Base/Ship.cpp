/// Emil Hedemalm
/// 2015-01-21
/// Ship.

#include "../SpaceShooter2D.h"
#include "Ship.h"

#include "File/File.h"
#include "String/StringUtil.h"

#include "File/LogFile.h"
#include "Game/GameVariable.h"
#include "Level/SpawnGroup.h"
#include "WeaponScript.h"
#include "SpaceShooterScript.h"
#include "Properties/ShipProperty.h"
#include "PlayingLevel.h"
#include "Base/PlayerShip.h"
#include "PlayingLevel/HUD.h"

int Ship::shipIDEnumerator = 0;

extern SpaceShooterEvaluator spaceShooterEvaluator;

Ship::Ship()
{
	shipProperty = 0;
	weaponCooldownBonus = 0;
	childrenDestroyed = 0;
	projectileSpeedBonus = 0;
	shipID = ++shipIDEnumerator;
	despawnOutsideFrame = true;
	script = 0;
	collisionDamageCooldown = Time(TimeType::MILLISECONDS_NO_CALENDER, 100);
	lastShipCollision = Time(TimeType::MILLISECONDS_NO_CALENDER, 0);

	heatDamageTaken = 0;
	spawnGroup = 0;
	shoot = false;
	activeWeapon = 0;
	spawned = false;
	entity = NULL;
	enemy = true;
	allied = false;
	maxHP = 10;
	hp = 10.f;
	canMove = false;
	canShoot = false;
	spawnInvulnerability = false;
	hasShield = false;
	shieldValue = 0;
	maxShieldValue = 10;
	currentMovement = 0;
	timeInCurrentMovement = 0;
	speed = 0.f;
	score = 10;
	destroyed = false;
	spawned = false;
	despawned = false;

	currentRotation = 0;
	timeInCurrentRotation = 0;

	collideDamage = 1;
	armorRegenRate = 2;
	
	graphicModel = "obj/Ships/Ship.obj";

	timeSinceLastSkillUseMs = 0;
	maxRadiansPerSecond = PI / 12;
	movementDisabled = false;
	weaponScriptActive = false;
	weaponScript = 0;
	activeSkill = NO_SKILL;
	skill = NO_SKILL;
	skillCooldownMultiplier = 1.f;
}

ShipPtr Ship::NewShip() {
	Ship* ship = new Ship();
	ShipPtr sp = ShipPtr(ship);
	ship->selfPtr = sp;
	return sp;
}

ShipPtr Ship::NewShip(const Ship& ref)
{
	Ship* ship = new Ship();
	ship->CopyStatsFrom(ref);
	ship->CopyWeaponsFrom(ref);

	ShipPtr sp = ShipPtr(ship);
	ship->selfPtr = sp;
	return sp;
}


Ship::~Ship()
{
	weaponSet.ClearAndDelete();
	if (spawnGroup)
	{
		spawnGroup->RemoveThis(this);
		spawnGroup = 0;
	}
	if (entity)
	{
		this->shipProperty->shouldDelete = true;
		this->shipProperty->sleeping = 0;
		this->shipProperty->ship = 0;
		shipProperty = 0;
	}
}

ShipPtr Ship::GetSharedPtr() {
	ShipPtr ptr = selfPtr.lock();
	assert(ptr != nullptr);
	return ptr;
}

ShipPtr Ship::GetByType(String typeName) {
	List<String> typesNames;
	for (int i = 0; i < types.Size(); ++i)
	{
		ShipPtr type = types[i];
		if (type->name == typeName)
		{
			return type;
		}
		typesNames.Add(type->name);
	}
	String shipTypesStr = MergeLines(typesNames, ", ");
	// For now, just add a default one.
	LogMain("ERROR: Couldn't find ship by name \'" + typeName + "\'. Available ships types as follows:\n\t" + shipTypesStr, ERROR);
	return nullptr;
}

Random cooldownRand;
void Ship::RandomizeWeaponCooldowns()
{
//	if (!ai)
//		return;
	for (int i = 0; i < weaponSet.Size(); ++i)
	{
		Weapon * weap = weaponSet[i];
		weap->currCooldownMs = (int) (weap->cooldown.Milliseconds() * cooldownRand.Randf());
//		weap->lastShot = /*flyTime +*/ Time::Milliseconds(weap->cooldown.Milliseconds() * cooldownRand.Randf());
	}
}

List< std::shared_ptr<Entity> > Ship::Spawn(ConstVec3fr atWorldPosition, ShipPtr in_parent, PlayingLevel & playingLevel)
{	

	std::cout<<"\nPossible kills: "<<++spaceShooter->LevelPossibleKills()->iValue;

	/// Reset stuffs if not already done so.
	movementDisabled = false;
	destroyed = false;
	RandomizeWeaponCooldowns();

	Vector3f atPosition = atWorldPosition;
	atPosition.z = 0;
//	atPosition.y += levelEntity->worldPosition

	/// Stuff.
	name.SetComparisonMode(String::NOT_CASE_SENSITIVE);
	if (name.Contains("boss"))
		despawnOutsideFrame = false;
	parent = in_parent;
	if (parent)
	{
		parent->children.AddItem(GetSharedPtr());
	}

	EntitySharedPtr entity = EntityMan.CreateEntity(name, GetModel(), TexMan.GetTextureByColor(Color(0,255,0,255)));
	entity->localPosition = atPosition;
	
	PhysicsProperty * pp = new PhysicsProperty();
	entity->physics = pp;
	// Setup physics.
	pp->type = PhysicsType::DYNAMIC;
	pp->linearDamping = 0.99f;
	float radians = PI / 2;
	if (enemy)
	{
		pp->collisionCategory = CC_ENEMY;
		pp->collisionFilter = CC_ALL_PLAYER;
		/// Turn to face left -X
		if (!parent)
			entity->SetRotation(Vector3f(radians, radians, 0));
	}
	/// Allied
	else 
	{
		pp->velocity = spaceShooter->level.BaseVelocity();
		assert(!pp->velocity.IsInfinite());
		pp->collisionCategory = CC_PLAYER;
		pp->collisionFilter = CC_ALL_ENEMY;
		/// Turn to face X+
		entity->SetRotation(Vector3f(radians, -radians, 0));
	}
	pp->collisionCallback = true;
	pp->shapeType = PhysicsShape::AABB;
	/// Adjust physics model as needed.
	if (this->physicsModel.Length())
	{
		if (this->physicsModel == "GraphicModel")
		{
			// Same model as in graphics.
			pp->shapeType = PhysicsShape::MESH;
		}
	}
	// By default, set invulerability on spawn.
	this->spawnInvulnerability = true;
	ShipProperty * sp = new ShipProperty(GetSharedPtr(), entity);
	shipProperty = sp;
	entity->properties.Add(sp);
	this->entity = entity;
	this->spawned = true;
	this->StartMovement(playingLevel);

	/// Spawn children if applicable.
	List< std::shared_ptr<Entity> > children = SpawnChildren(playingLevel);
	/// Set up parenting.
	if (parent)
	{
		parent->entity->children.AddItem(this->entity);
		this->entity->parent = parent->entity;
//		QueuePhysics(new PMSetEntity(entity, PT_PARENT, parent->entity));
	}
	/// IF final aprent, register for rendering, etc.
	else 
	{
		List< std::shared_ptr<Entity> > all = children + entity;
		playingLevel.shipEntities.Add(all);
		MapMan.AddEntities(all);
		/// Recalculate matrix and all children matrices.
		entity->RecalculateMatrix(Entity::ALL_PARTS, true);
	}
	/// Load script if applicable.
	if (scriptSource.Length())
	{
		script = new SpaceShooterScript();
		script->Load(scriptSource);
		script->entity = entity;
		/// Add custom variables based on who started the script.
		script->variables.AddItem(Variable("self", shipID));
		script->variables.AddItem(Variable("player", playingLevel.playerShip->shipID));
		script->variables.Add(Movement::GetTypesAsVariables());
		script->functionEvaluators.AddItem(&spaceShooterEvaluator);
		script->OnBegin();
	}

	return entity;
}

/// Handles spawning of children as needed.
List< std::shared_ptr<Entity> > Ship::SpawnChildren(PlayingLevel & playingLevel)
{
	/// Translate strings.
	List<String> childStrings;
	for (int i = 0; i < childrenStrings.Size(); ++i)
	{
		String str = childrenStrings[i];
		if (str.EndsWith('*'))
		{
			String strWoStar = str - "*";
			/// Grab all starting with it.
			for (int j = 0; j < Ship::types.Size(); ++j)
			{
				ShipPtr type = Ship::types[j];
				if (type->name.Contains(strWoStar))
					childStrings.AddItem(type->name);
			}
		}
	}
	List< std::shared_ptr<Entity> > childrenSpawned;
	for (int i = 0; i < childStrings.Size(); ++i)
	{
		String str = childStrings[i];
		ShipPtr newShip = Ship::New(str);
		if (!newShip)
		{
			LogMain("Ship::SpawnChildren: Unable to create ship of type: "+str, CAUSE_ASSERTION_ERROR);
			continue;
		}
		if (enemy)
			activeLevel->enemyShips.AddItem(newShip);
		else 
			activeLevel->alliedShips.AddItem(newShip);
		activeLevel->ships.AddItem(newShip);

		ShipPtr ship = newShip;
		ship->allied = this->allied;
		ship->RandomizeWeaponCooldowns();
		ship->spawnGroup = this->spawnGroup;
		ship->Spawn(Vector3f(), GetSharedPtr(), playingLevel);
		childrenSpawned.AddItem(ship->entity);
		/// Apply spawn group properties.
//		ship->shoot &= shoot;
//		ship->speed *= relativeSpeed;
	}
	return childrenSpawned;
}

/// Despawns children. Does not resolve parent-pointers.
void Ship::Despawn(PlayingLevel& playingLevel, bool doExplodeEffectsForChildren)
{
	if (!spawned)
		return;
	for (int i = 0; i < children.Size(); ++i)
	{
		ShipPtr child = children[i];
		if (doExplodeEffectsForChildren)
			child->ExplodeEffects(playingLevel);
		child->parent = 0;
		child->Despawn(playingLevel, doExplodeEffectsForChildren);
	}
//	LogMain("Despawning ship "+name+" with children "+childrrr, INFO);
	spawned = false;

	// Delete entity first.
	EntitySharedPtr tmp = entity;
	entity = 0;
//	std::cout<<"\nDeleting entity "+tmp->name;
	MapMan.DeleteEntity(tmp);
	/// Waaaat.
	playingLevel.shipEntities.RemoveItem(tmp);

	/// Unbind link from property to this ship.
	shipProperty->sleeping = true; // Set sleeping so it shouldn't process anything anymore.
	shipProperty->ship = 0;

	/// Notify parent if child?
	if (parent)
	{
		++parent->childrenDestroyed;
		parent->children.RemoveItem(GetSharedPtr());
		parent = 0;
	}

	children.Clear();
	if (spawnGroup)
	{
		if (hp <= 0)
			spawnGroup->OnShipDestroyed(PlayingLevelRef(), GetSharedPtr());
		else
			spawnGroup->OnShipDespawned(PlayingLevelRef(), GetSharedPtr());
	}
}

/// Checks current movement. Will only return true if movement is target based and destination is within threshold.
bool Ship::ArrivedAtDestination()
{
//	LogMain("Update maybe", INFO);
	return false;
}

void Ship::Process(PlayingLevel& playingLevel, int timeInMs)
{
	/// If destroyed from elsewhere..?
	if (entity == 0)
		return;
	// Skill cooldown.
	if (timeSinceLastSkillUseMs >= 0)
	{
		timeSinceLastSkillUseMs += timeInMs;
		if (timeSinceLastSkillUseMs > skillDurationMs)
		{
			activeSkill = NO_SKILL;
			spaceShooter->UpdateHUDSkill();
		}
	}
	/// Process scripts (pretty much AI and other stuff?)
	if (script)
		script->Process(timeInMs);
	// Increment time in movement if applicable.
	if (movements.Size())
		movements[currentMovement].OnFrame(playingLevel, timeInMs);
	// AI
	ProcessAI(playingLevel, timeInMs);
	// Weapon systems.
	ProcessWeapons(playingLevel, timeInMs);
	// Shield
	if (hasShield)
	{
		// Repair shield
		if (shieldValue < MaxShield()) {
			float toRegen = timeInMs * shieldRegenRate * (activeSkill == POWER_SHIELD ? 0.1f : 0.001f) * (activeSkill == ATTACK_FRENZY ? -1.0f : 1.0f);
			playingLevel.shieldRegenerated += toRegen;
			shieldValue += toRegen;
		}
		if (shieldValue > MaxShield())
			shieldValue = shieldValue * 0.998f + 0.002f * MaxShield(); // If past the max, tune to max by 1% per frame?
		if (allied)
			HUD::Get()->UpdateUIPlayerShield(false);
	}
	if (allied)
	{
		if (hp < maxHP) {
			float toRegen = timeInMs * armorRegenRate * 0.001f;
			playingLevel.armorRegenerated += toRegen;
			hp += toRegen;
			if (hp > maxHP)
				hp = (float)maxHP;
			HUD::Get()->UpdateUIPlayerHP(false);
		}
	}
}

void Ship::ProcessAI(PlayingLevel& playingLevel, int timeInMs)
{
	// Don't process inactive ships..
	if (!enemy)
		return;
	if (rotations.Size() == 0)
		return;
	// Rotate accordingly.
	Rotation & rota = rotations[currentRotation];
	rota.OnFrame(playingLevel, timeInMs);
	// Increase time spent in this state accordingly.
	timeInCurrentRotation += timeInMs;
	if (timeInCurrentRotation > rota.durationMs && rota.durationMs > 0)
	{
		currentRotation = (currentRotation + 1) % rotations.Size();
		timeInCurrentRotation = 0;
		Rotation & rota2 = rotations[currentRotation];
		rota2.OnEnter(GetSharedPtr());
	}
	if (!canMove)
		return;
	// Move?
	EntitySharedPtr shipEntity = entity;
	Movement & move = movements[currentMovement];
	move.OnFrame(playingLevel, timeInMs);
	// Increase time spent in this state accordingly.
	timeInCurrentMovement += timeInMs;
	if (timeInCurrentMovement > move.durationMs && move.durationMs > 0)
	{
		currentMovement = (currentMovement + 1) % movements.Size();
		timeInCurrentMovement = 0;
		Movement & newMove = movements[currentMovement];
		newMove.OnEnter(playingLevel, GetSharedPtr());
	}
}


void Ship::ProcessWeapons(PlayingLevel& playingLevel, int timeInMs)
{
	if (!weaponSet.Size())
		return;

	if (weaponScriptActive && weaponScript)
	{
		weaponScript->Process(GetSharedPtr(), timeInMs);
	}

	/// Process ze weapons.
	for (int i = 0; i < weaponSet.Size(); ++i)
		weaponSet[i]->Process(playingLevel, GetSharedPtr(), timeInMs);

	// enemy AI fire all weapons simultaneously for the time being.
	if (enemy)
	{
		shoot = false;
		if (weaponSet.Size() == 0)
		{
			std::cout<<"\nLacking weapons..";
		}
		// Do stuff.
		for (int i = 0; i < weaponSet.Size(); ++i)
		{
			Weapon * weapon = weaponSet[i];
			// Aim.
			weapon->Aim(playingLevel, GetSharedPtr());
			// Dude..
			shoot = true;
			// Shoot all weapons by default.
			weapon->Shoot(playingLevel, GetSharedPtr());
		}
		return;
	}
	if (!shoot)
		return;
	if (activeWeapon == 0)
		activeWeapon = weaponSet.Size()? weaponSet[0] : 0;
	// Shoot with current weapon for player.
	if (activeWeapon)
		activeWeapon->Shoot(playingLevel, GetSharedPtr());
}
// Sets new bonus, updates weapons if needed.
void Ship::SetProjectileSpeedBonus(float newBonus)
{
	projectileSpeedBonus = newBonus;
} 
	
// Sets new bonus, updates weapons if needed.
void Ship::SetWeaponCooldownBonus(float newBonus)
{
	weaponCooldownBonus = newBonus;
} 


void Ship::SetWeaponCooldownByID(int id, AETime newcooldown)
{
	assert(false);
	//for (int i = 0; i < weapons.Size(); ++i)
	//{
	//	Weapon * weap = weapons[i];
	//	if (weap->type == id)
	//		weap->cooldown = newcooldown;
	//}
	//for (int i = 0; i < children.Size(); ++i)
	//{
	//	children[i]->SetWeaponCooldownByID(id, newcooldown);
	//}
}
/// Disables weapon in this and children ships.
void Ship::DisableWeapon(String weaponName)
{
	for (int i = 0; i < weaponSet.Size(); ++i)
	{
		Weapon * weap = weaponSet[i];
		if (weap->name == weaponName)
			weap->enabled = false;
	}
	for (int i = 0; i < children.Size(); ++i)
	{
		children[i]->DisableWeapon(weaponName);
	}
}


/// Prepends the source with '/obj/Ships/' and appends '.obj'. Uses default 'Ship.obj' if needed.
Model * Ship::GetModel()
{
	String folder = "obj/";//Ships/";
	Model * model = ModelMan.GetModel(graphicModel);
	if (!model)
	{
		std::cout<<"\nUnable to find ship model with name \'"<<graphicModel<<"\', using default model.";
		graphicModel.SetComparisonMode(String::NOT_CASE_SENSITIVE);
		if (graphicModel.Contains("turret"))
			model = ModelMan.GetModel("obj/Ships/Turret");
		else
			model = ModelMan.GetModel("obj/Ships/Ship");
	}
	return model;
}

void Ship::DisableMovement()
{
	movementDisabled = true;
}
void Ship::OnSpeedUpdated(PlayingLevel& playingLevel)
{
	/// Update based on current movement?
	if (movements.Size())
		movements[currentMovement].OnSpeedUpdated(playingLevel);
}


void Ship::Damage(PlayingLevel& playingLevel, Weapon & weapon)
{
	if (allied) {
		playingLevel.projectilesDodged.Add(false);
		playingLevel.UpdateEnemyProjectilesDodgedString();
	}

	float damage = weapon.damage * weapon.relativeStrength;
	bool ignoreShield = false;
	if (weapon.type == Weapon::Type::HeatWave)
	{
		heatDamageTaken += damage;
		damage += heatDamageTaken;
	}
	Damage(playingLevel, damage, ignoreShield, DamageSource::Projectile);
}

extern bool playerInvulnerability;

/// Returns true if destroyed -> shouldn't touch any more.
bool Ship::Damage(PlayingLevel& playingLevel, float amount, bool ignoreShield, DamageSource source)
{
	if (allied && playerInvulnerability)
		return false;
	if (!enemy)
		LogMain("Player took "+String((int)amount)+" damage!", DEBUG);
	if (spawnInvulnerability)
	{
	//	std::cout<<"\nInvulnnnn!";
	//	return;
	}
	int remainingDamage = (int) amount;
	if (hasShield && !ignoreShield)
	{
		float oldShieldValue = shieldValue;
		shieldValue -= amount;
		// Shield destroyed. Show some particles perhaps?
		if (shieldValue < 0) {
			shieldValue = 0;
		}
		remainingDamage = (int) (amount - oldShieldValue);
		if (this->allied)
			HUD::Get()->UpdateUIPlayerShield(true);
		// If no more dmg, no need to calc armor.
		if (remainingDamage < 0)
			return false;
	}
	// Modulate amount depending on armor toughness and reactivity.
	float activeToughness = (float)armorStats.toughness;
	/// Projectile/explosion-type attacks, reactivity effects.
	switch (source) {
	case DamageSource::Collision: // No addition, unless..?
		break;
	case DamageSource::Explosion:
		activeToughness += armorStats.reactivity * 0.5f;
		break;
	case DamageSource::Projectile:
		activeToughness += armorStats.reactivity;
		break;
	}
	// 3 toughness = 333% damage 
	// 5 toughness = 200% damage 
	// 8 toughness = 125% damage
	// 12 toughness = 83% 
	// 16 toughness = 62% 
	// 20 toughness = 50% 
	// 30 toughness = 33% damage 
	// 45 toughness = 22% damage 
	remainingDamage = int(remainingDamage / (activeToughness / 10.f)); 

	hp -= remainingDamage;
	if (hp < 0)
		hp = 0;
	if (this->allied)
		HUD::Get()->UpdateUIPlayerHP(false);
	if (hp <= 0)
	{
		Destroy(playingLevel);
		return true;
	}
	return false;
}

void Ship::Destroy(PlayingLevel& playingLevel)
{	
	if (destroyed)
		return;
	destroyed = true;
	if (entity)
	{
		if (spawnGroup)
		{
			spawnGroup->OnShipDestroyed(PlayingLevelRef(), GetSharedPtr());
		}
		ShipProperty * sp = entity->GetProperty<ShipProperty>();
		if (sp)
			sp->sleeping = true;


		// Explosion?
		ExplodeEffects(playingLevel);
		// Increase score and kills.
		if (!allied)
		{
			spaceShooter->LevelScore()->iValue += this->score;
			std::cout<<"\nKills: "<<spaceShooter->LevelKills()->iValue++;
			spaceShooter->OnScoreUpdated();
		}
		else {
			playingLevel.OnPlayerDied();
		}
		/// Despawn.
		Despawn(playingLevel, true);
	}
}

/// GFX and SFX
void Ship::ExplodeEffects(PlayingLevel& playingLevel)
{
	// Add a temporary emitter to the particle system to add some sparks to the collision
	SparksEmitter * tmpEmitter;

	/// Make cool emitter that emits from vertices or faces of the model?
	if (boss)
	{
		std::cout<<"\nExploding @ "<<this->entity->worldPosition;
		List<Triangle> tris = entity->GetTris();
		tmpEmitter = new SparksEmitter(tris);
		tmpEmitter->newType = true;
		/// Set emitters?
		tmpEmitter->positionEmitter.type = EmitterType::TRIANGLES;
		tmpEmitter->velocityEmitter.type = EmitterType::PLANE_XY; // Random XY
		tmpEmitter->constantEmission = 3000;
		tmpEmitter->SetParticleLifeTime(3.5f);
		tmpEmitter->SetEmissionVelocity(3.5f);
	}
	else 
	{
		tmpEmitter = new SparksEmitter(entity->worldPosition);
		tmpEmitter->SetParticleLifeTime(2.5f);
		tmpEmitter->constantEmission = 750;
		tmpEmitter->SetEmissionVelocity(2.5f);
	}

	tmpEmitter->SetRatioRandomVelocity(1.0f);
	tmpEmitter->instantaneous = true;
	tmpEmitter->SetScale(0.1f);
	Vector4f color =  Vector4f(1,0.5f,0.1f,1.f);// entity->diffuseMap->averageColor;
	color.w *= 0.5f;
	tmpEmitter->SetColor(color);
	GraphicsMan.QueueMessage(new GMAttachParticleEmitter(tmpEmitter->GetSharedPtr(), playingLevel.sparks->GetSharedPtr()));
	/// SFX
	QueueAudio(new AMPlaySFX("sfx/Ship Death.wav"));
}


bool Ship::DisableWeaponsByID(int id)
{
	assert(false);

	//for (int i = 0; i < weapons.Size(); ++i)
	//{
	//	Weapon * weap = weapons[i];
	//	if (weap->type == id)
	//		weap->enabled = false;
	//}
	//for (int i = 0; i < children.Size(); ++i)
	//	children[i]->DisableWeaponsByID(id);
	return true;
}

#define FOR_ALL_WEAPONS \
	for (int i = 0; i < weapons.Size(); ++i)\
	{\
		Weapon * weap = weapons[i];
bool Ship::EnableWeaponsByID(int id)
{
	assert(false);
	//FOR_ALL_WEAPONS
	//	if (weap->type == id)
	//		weap->enabled = true;
	//}
	//for (int i = 0; i < children.Size(); ++i)
	//	children[i]->EnableWeaponsByID(id);
	return true;
}


void Ship::SetLevelOfAllWeaponsTo(int level) {
	for (int i = 0; i < weaponSet.Size(); ++i)
		weaponSet[i]->level = level;
}


bool Ship::DisableAllWeapons()
{
	for (int i = 0; i < weaponSet.Size(); ++i)
	{
		Weapon * weap = weaponSet[i];
		weap->enabled = false;
	}
	for (int i = 0; i < children.Size(); ++i)
		children[i]->DisableAllWeapons();
	return true;
}


void Ship::SetMovement(PlayingLevel& playingLevel, Movement & movement)
{
	this->movements.Clear();
//		move.vec = Vector2f(-10.f, targetEntity->worldPosition.y); 
	this->movements.AddItem(movement);
	movements[0].OnEnter(playingLevel, GetSharedPtr());
//	.OnEnter(ship);
}

void Ship::SetSpeed(PlayingLevel& playingLevel, float newSpeed)
{
	this->speed = newSpeed;
	this->OnSpeedUpdated(playingLevel);
}

/// Creates new ship of specified type.
ShipPtr Ship::New(String shipByName)
{
	shipByName.RemoveSurroundingWhitespaces();
	ShipPtr ref = GetByType(shipByName);
	ShipPtr newShip = Ship::NewShip(*ref);
	newShip->CopyWeaponsFrom(*ref);
	return newShip;
}


void Ship::CopyWeaponsFrom(const Ship& ref) {
	// Create copies of the weapons.
	this->weaponSet.Clear();
	for (int j = 0; j < ref.weaponSet.Size(); ++j)
	{
		Weapon * refWeap = ref.weaponSet[j];
		Weapon * newWeap = new Weapon();
		*newWeap = *refWeap; // Weapon::Get(refWeap->type, refWeap->level);
		this->weaponSet.AddItem(newWeap);
	}
}

void Ship::CopyStatsFrom(const Ship& ref) {
	name = ref.name;
	childrenDestroyed = ref.childrenDestroyed;
	despawnOutsideFrame = ref.despawnOutsideFrame;
	type = ref.type;
	physicsModel = ref.physicsModel;
	spawnGroup = ref.spawnGroup;
	parent = ref.parent;
	
	// Created later on
	shipProperty = nullptr;
	children.Clear();

	canMove = ref.canMove;
	movementDisabled = ref.movementDisabled;
	canShoot = ref.canShoot;
	hasShield = ref.hasShield;
	shoot = ref.shoot;

	weaponScriptActive = ref.weaponScriptActive;
	boss = ref.boss;
	difficulty = ref.difficulty;
	timeSinceLastSkillUseMs = ref.timeSinceLastSkillUseMs;
	skill = ref.skill;
	activeSkill = ref.activeSkill;

	skillDurationMs = ref.skillDurationMs;
	skillCooldownMultiplier = ref.skillCooldownMultiplier; 
	onCollision = ref.onCollision;
	spawned = ref.spawned;
	destroyed = ref.destroyed;
	despawned = ref.despawned;

	/// In order to not take damage allllll the time (depending on processor speed, etc. too.)
	lastShipCollision;
	collisionDamageCooldown = ref.collisionDamageCooldown;
	
	// Default 0. Scriptable.
	projectileSpeedBonus = ref.projectileSpeedBonus;
	weaponCooldownBonus = ref.weaponCooldownBonus;
	
	/// Mooovemeeeeeeent
	movements = ref.movements;
	currentMovement = ref.currentMovement;
	timeInCurrentMovement = ref.timeInCurrentMovement; 
	rotations = ref.rotations;
	currentRotation = ref.currentRotation;
	timeInCurrentRotation = ref.timeInCurrentRotation;
	/// Maximum amount of radians the ship may rotate per second.
	maxRadiansPerSecond = ref.maxRadiansPerSecond;

	// Parsed value divided by 5.
	speed = ref.speed;
	shieldValue = ref.shieldValue;
	maxShieldValue = ref.maxShieldValue;
	/// Regen per millisecond
	shieldRegenRate = ref.shieldRegenRate;
	hp = ref.hp;
	armorRegenRate = ref.armorRegenRate;
	maxHP = ref.maxHP;
	collideDamage = ref.collideDamage;
	heatDamageTaken = ref.heatDamageTaken;
	abilities = ref.abilities;
	abilityCooldown = ref.abilityCooldown;
	graphicModel = ref.graphicModel;
	other = ref.other;

	weaponSet = WeaponSet(ref.weaponSet);
	if (weaponSet.Size())
		activeWeapon = weaponSet[0]; 

	/// If allied or player, false for enemies.
	allied = ref.allied;
	/// If the ship.. is enemy ai? Should be renamed
	enemy = ref.enemy;
	spawnInvulnerability = ref.spawnInvulnerability;
	/// Yielded when slaying it.
	score = ref.score;

	position = ref.position;
	
	WeaponScript * weaponScript;
}



/// Returns speed, accounting for active skills, weights, etc.
float Ship::Speed()
{
	if (speed == 0) {
		LogMain("Speed 0 for ship. Is this correct? For ship type: " + type + ", name: " + name, WARNING);
		speed = 1;
	}
	if (activeSkill == SPEED_BOOST)
		return speed * 1.75f;
	return speed;
}

/// Accounting for boosting skills.
float Ship::MaxShield()
{
	if (activeSkill == POWER_SHIELD)
		return maxShieldValue * 10;
	return maxShieldValue * this->shieldGeneratorEfficiency;
}

/// Checks weapon's latest aim dir.
Vector3f Ship::WeaponTargetDir()
{
	for (int i = 0; i < weaponSet.Size(); ++i)
	{
		Weapon * weapon = weaponSet[i];
		if (weapon->aim)
			return weapon->currentAim;
	}
	return Vector3f();
}

int Ship::CurrentWeaponIndex() {
	return weaponSet.GetIndexOf(activeWeapon);
}

bool Ship::SwitchToWeapon(int index)
{
	if (index < 0)
		index = weaponSet.Size() - 1;
	if (index >= weaponSet.Size())
		index = 0;
	if (index < 0 || index >= weaponSet.Size())
	{
		std::cout<<"\nSwitchToWeapon bad index";
		return false;
	}
	activeWeapon = weaponSet[index];
	std::cout<<"\nSwitched to weapon: "<<activeWeapon->name;
//	UpdateStatsFromGear();
	// Update ui
	if (!enemy)
	{
		QueueGraphics(new GMSetUIi("Weapons", GMUI::INTEGER_INPUT, index));
	}
	return true;
}

/// Calls OnEnter for the initial movement pattern.
void Ship::StartMovement(PlayingLevel& playingLevel)
{
	if (rotations.Size())
		rotations[0].OnEnter(GetSharedPtr());
	if (movements.Size())
		movements[0].OnEnter(playingLevel, GetSharedPtr());
}

/// For player ship.
Weapon * Ship::SetWeaponLevel(Weapon::Type weaponType, int level)
{
	Weapon * weapon = GetWeapon(weaponType);
	const Weapon * targetWeapon = Weapon::Get(weaponType, level);
	if (!targetWeapon)
	{
		if (level == 0)
		{
			targetWeapon = Weapon::Get(weaponType, 1);
			if (targetWeapon)
			{
				*weapon = *targetWeapon;
				weapon->level = 0;
				weapon->name = "";
				weapon->cooldown = Time(TimeType::MILLISECONDS_NO_CALENDER, 400);
			}
		}
		return weapon;
	}
	*weapon = *targetWeapon;
	assert(weapon->name.Length() > 0);
	return weapon;
}

Weapon * Ship::GetWeapon(Weapon::Type ofType)
{
	for (int i = 0; i < weaponSet.Size(); ++i)
	{
		Weapon * weapon = weaponSet[i];
		if (weapon->type == ofType)
			return weapon;
	}
	Weapon * newWeapon = new Weapon();
	newWeapon->type = ofType;
	newWeapon->level = 0;
	weaponSet.AddItem(newWeapon);
	return weaponSet.Last();
}

int Skill::Cooldown(SkillType skill) {
	int skillCooldownMs = -1;
	switch (skill)
	{
	case POWER_SHIELD:
		skillCooldownMs = 25000;
		break;
	case SPEED_BOOST:
		skillCooldownMs = 20000;
		break;
	case ATTACK_FRENZY:
		skillCooldownMs = 30000;
		break;
	default:
		break;
	}
	return skillCooldownMs;
}

int Ship::SkillCooldown() {
	return (int)(Skill::Cooldown(activeSkill) * skillCooldownMultiplier);
}

void Ship::ActivateSkill()
{
	// Check cooldown?
	LogMain("Attempting to use skill: "+ Skill::Name(activeSkill) + " timeSinceLastSkillUseMs "+String(timeSinceLastSkillUseMs)+" coolDown: "+ SkillCooldown(), INFO);
	if (SkillCooldown() == -1)
		return;
	if (timeSinceLastSkillUseMs < SkillCooldown())
		return;

	activeSkill = skill;
	timeSinceLastSkillUseMs = 0;
	skillDurationMs = 7000;
	// Reflect activation in HUD?
	spaceShooter->UpdateHUDSkill();
}

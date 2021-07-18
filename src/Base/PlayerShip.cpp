/// Emil Hedemalm
/// 2020-09-06
/// Player-ship specific code

#include "Base/PlayerShip.h"
#include "File/LogFile.h"
#include "PlayingLevel.h"
#include "Input/InputManager.h"

// Fetches player ship along with gear and updated stats.
PlayerShip::PlayerShip()
	: Ship()
{

	Ship* ref = Ship::GetByType("Default");
	assert(ref != nullptr);
	CopyStatsFrom(*ref);
	CopyWeaponsFrom(*ref);

	enemy = false;
	allied = true;

	weapons.Add(Gear::StartingWeapon());	
	UpdateGearFromVars();
	UpdateStatsFromGear();
	SetLevelOfAllWeaponsTo(0);
	autoAim = false;
}

// For AIs, limits it linearly when within 10 distance
void PlayerShip::SetAIVelocityVector(PlayingLevel& playingLevel, Vector3f vector) {
	float distance = vector.Length();
	float magnitude = distance;
	// Clamp from 0 to 1 already?
	ClampFloat(magnitude, 0, 1.0f);
	// and if below X distance, scale down linearly for smooth approach
	if (distance < 5.0f) {
		magnitude *= distance / 5.0f;
	}
	playingLevel.SetPlayerMovement(vector.NormalizedCopy() * magnitude);
}

#include "PlayingLevel/HUD.h"

void PlayerShip::Process(PlayingLevel& playingLevel, PlayerShip* playerShip, int timeInMs) {
	float targetRotationSpeed = 2.0f;

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


	Ship::Process(playingLevel, playerShip, timeInMs);
}

void PlayerShip::ProcessAI(PlayerShip* playerShip, int timeInMs) {
	if (!autoAim)
		return;

	PlayingLevel& playingLevel = PlayingLevelRef();

	// Dodge the closest enemy if too close?
	float distance;
	auto closestShipOrObstacle = playingLevel.GetLevel().ClosestShipOrObstacle(playingLevel, true, this->entity->worldPosition, distance);
	if (closestShipOrObstacle && distance < (this->entity->Radius() * 2 + 10.0f)) {
		Vector3f fromTarget = playingLevel.levelEntity->worldPosition - closestShipOrObstacle->worldPosition;
		SetAIVelocityVector(playingLevel, fromTarget);
		shoot = false;
		return;
	}

	auto target = playingLevel.GetLevel().ClosestTarget(playingLevel, true, this->entity->worldPosition);
	if (target == nullptr){
		shoot = false;
		// Move to the middle when no mobs, also a bit forward?
		Vector3f vector = playingLevel.levelEntity->worldPosition - entity->worldPosition;
		SetAIVelocityVector(playingLevel, vector);
		return;
	}
	shoot = true;

	Vector3f vector = Vector3f(0, target->worldPosition.y - this->entity->worldPosition.y, 0);
	SetAIVelocityVector(playingLevel, vector);

	if (activeWeapon == nullptr) {
		this->SwitchToWeapon(CurrentWeaponIndex() + 1);
	}
	else if (activeWeapon->reloading) {
		this->SwitchToWeapon(CurrentWeaponIndex() + 1);
	}
}

void PlayerShip::SetAutoAim(bool value) {
	autoAim = value;
}

int PlayerShip::MaxGearForType(Gear::Type type) const {
	switch (type) {
	case Gear::Type::Weapon: return maxWeapons;
	case Gear::Type::Armor: return maxArmorLayers;
	case Gear::Type::Shield: return maxShieldGenerators;
	}
	assert(false);
	return 0;
}

List<Gear> PlayerShip::Equipped(Gear::Type byType) const {
	switch (byType) {
	case Gear::Type::All: return weapons + shieldGenerators + armorLayers;
	case Gear::Type::Weapon: return EquippedWeapons();
	case Gear::Type::Armor: return EquippedArmorLayers();
	case Gear::Type::Shield: return EquippedShieldGenerators();
	}
	assert(false);
	return List<Gear>();
}

List<Gear> PlayerShip::EquippedWeapons() const {
	return weapons;
}
List<Gear> PlayerShip::EquippedShieldGenerators() const {
	return shieldGenerators;
}
List<Gear> PlayerShip::EquippedArmorLayers() const {
	return armorLayers;
}

void PlayerShip::UnequipWeapons() {
	while (EquippedWeapons().Size() > 0) {
		this->UnequipGear(EquippedWeapons()[0]);
	}
}

// Equip given gear, if possible.
bool PlayerShip::Equip(const Gear& gear) {
	switch (gear.type) {
	case Gear::Type::Weapon:
		if (weapons.Size() >= maxWeapons)
			return false;
		weapons.Add(gear);
		break;
	case Gear::Type::Shield:
		if (shieldGenerators.Size() >= maxShieldGenerators)
			return false;
		shieldGenerators.Add(gear);
		break;
	case Gear::Type::Armor:
		if (armorLayers.Size() >= maxArmorLayers)
			return false;
		armorLayers.Add(gear);
		break;
	}
	SaveGearToVars();
	UpdateStatsFromGear();
	return true;
}

void PlayerShip::EquipTutorialLevel1Weapons() {
	UnequipWeapons();
	Equip(Gear::Get("Machine gun I"));
	Equip(Gear::Get("Small rockets I"));
	Equip(Gear::Get("Big rockets I"));
}

void PlayerShip::EquipTutorialLevel3Weapons() {
	UnequipWeapons();
	Equip(Gear::Get("Machine gun III"), 0);
	Equip(Gear::Get("Small rockets III"), 1);
	Equip(Gear::Get("Big rockets III"), 2);
}

// Equip given gear, replacing the other gear.
void PlayerShip::Equip(const Gear& gear, const int replacingGearIndex) {
	switch (gear.type) {
	case Gear::Type::Weapon:
		weapons[replacingGearIndex] = gear;
		break;
	case Gear::Type::Shield:
		shieldGenerators[replacingGearIndex] = gear;
		break;
	case Gear::Type::Armor:
		armorLayers[replacingGearIndex] = gear;
		break;
	}
	SaveGearToVars();
	UpdateStatsFromGear();
}



// Unequips given gear, assuming it's already equipped.
bool PlayerShip::UnequipGear(const Gear& gear) {
	bool success = weapons.RemoveItem(gear);
	if (!success)
		success = shieldGenerators.RemoveItem(gear);
	if (!success)
		success = armorLayers.RemoveItem(gear);
	if (!success) {
		LogMain("Failed to unequip anything!", INFO);
		return false;
	}
	SaveGearToVars();
	UpdateStatsFromGear();
	return true;
}


// Updates weapon levels, armor upgrades, shield upgrades, etc. based on save game vars
void PlayerShip::UpdateGearFromVars() {
	weapons = Gear::EquippedWeapons();
	armorLayers = Gear::EquippedArmorLayers();
	shieldGenerators = Gear::EquippedShieldGenerators();
}

void PlayerShip::SaveGearToVars() {
	Gear::SetEquippedWeapons(weapons);
	Gear::SetEquippedArmorLayers(armorLayers);
	Gear::SetEquippedShieldGenerators(shieldGenerators);
}

/// If using Armor and Shield gear (Player mainly).
void PlayerShip::UpdateStatsFromGear()
{
	// Armor stats
	this->maxHP = 100;
	this->armorRegenRate = 1;
	this->armorStats = ArmorStats();
	this->speed = 20;
	this->rateOfFireMultiplier = 1.0f;
	this->reloadMultiplier = 1.0f;
	this->shieldRegenRate = 1;
	this->maxShieldValue = 50;
	this->shieldGeneratorEfficiency = 1.0f;

	for (int i = 1; i < weapons.Size(); ++i) {
		reloadMultiplier *= 1.1f;
		this->shieldGeneratorEfficiency *= 0.9f;
		this->rateOfFireMultiplier *= 0.95f;
	}

	for (int i = 0; i < armorLayers.Size(); ++i) {
		float factor = 1.0f;
		this->speed -= 2; // Decrease speed regardless of armor?
		if (i > 0) {
			factor = 0.5f;
			this->speed *= 0.8f; // -20% speed per extra layer of armor.
		}
		Gear armor = armorLayers[i];
		this->maxHP += (int) (armor.maxHP * factor);
		this->armorRegenRate += armor.armorRegen * factor;
		this->armorStats.reactivity += int( armor.Reactivity() * factor);
		this->armorStats.toughness += int(armor.Toughness() * factor);
	}
	hp = (float)maxHP;

	// Shield stats
	for (int i = 0; i < shieldGenerators.Size(); ++i) {
		float factor = 1.0f;
		if (i > 0) {
			factor = 0.5f;
			this->speed *= 0.9f;
			this->reloadMultiplier *= 1.1f;
			this->rateOfFireMultiplier *= 0.95f;
		}
		Gear shield = shieldGenerators[i];
		this->shieldRegenRate += shield.shieldRegen * factor;
		this->maxShieldValue += shield.maxShield * factor;
	}
	this->shieldValue = this->MaxShield();
	this->shieldRegenRate *= shieldGeneratorEfficiency;
}

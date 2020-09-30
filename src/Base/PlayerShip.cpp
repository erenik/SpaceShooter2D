/// Emil Hedemalm
/// 2020-09-06
/// Player-ship specific code

#include "Base/PlayerShip.h"
#include "File/LogFile.h"

// Fetches player ship along with gear and updated stats.
PlayerShip::PlayerShip()
	: Ship()
{

	ShipPtr ref = Ship::GetByType("Default");
	assert(ref != nullptr);
	CopyStatsFrom(*ref);
	CopyWeaponsFrom(*ref);

	enemy = false;
	allied = true;

	weapons.Add(Gear::StartingWeapon());	
	UpdateGearFromVars();
	UpdateStatsFromGear();
	SetLevelOfAllWeaponsTo(0);
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
	this->armorReactivity = 1;
	this->armorToughness = 10;
	for (int i = 0; i < armorLayers.Size(); ++i) {
		float factor = 1.0f;
		if (i > 0)
			factor = 0.5f;
		Gear armor = armorLayers[i];
		this->maxHP += armor.maxHP * factor;
		this->armorRegenRate += armor.armorRegen * factor;
		this->armorReactivity += armor.reactivity * factor;
		this->armorToughness += armor.toughness * factor;
	}
	hp = (float)maxHP;

	// Shield stats
	this->shieldRegenRate = 1;
	this->maxShieldValue = 50;
	for (int i = 0; i < shieldGenerators.Size(); ++i) {
		float factor = 1.0f;
		if (i > 0)
			factor = 0.5f;
		Gear shield = shieldGenerators[i];
		this->shieldRegenRate += shield.shieldRegen * factor;
		this->maxShieldValue += shield.maxShield * factor;
	}
	this->shieldValue = this->MaxShield();
}

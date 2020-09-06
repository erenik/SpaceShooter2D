/// Emil Hedemalm
/// 2020-09-06
/// Player-ship specific code

#include "Base/PlayerShip.h"


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

	weapons = WeaponSet();
	const Weapon * starterWeapon = Weapon::Get(Weapon::Type::MachineGun, 1);
	Weapon * weapon = new Weapon();
	*weapon = *starterWeapon;
	assert(starterWeapon != nullptr);
	weapons.Add(weapon);
	
	UpdateGearFromVars();
	UpdateStatsFromGear();
	SetLevelOfAllWeaponsTo(0);
}

// Updates weapon levels, armor upgrades, shield upgrades, etc. based on save game vars
void PlayerShip::UpdateGearFromVars() {
	armor = Gear::EquippedArmor();
	shield = Gear::EquippedShield();
}

/// If using Armor and Shield gear (Player mainly).
void PlayerShip::UpdateStatsFromGear()
{
	// Armor stats
	this->maxHP = armor.maxHP;
	hp = (float)maxHP;
	this->armorRegenRate = armor.armorRegen;

	// Shield stats
	this->shieldRegenRate = shield.shieldRegen;
	this->maxShieldValue = (float) shield.maxShield;
	this->shieldValue = this->MaxShield();
}

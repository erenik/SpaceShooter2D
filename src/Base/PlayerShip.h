/// Emil Hedemalm
/// 2020-09-06
/// Player-ship specific code

#pragma once

#include "Base/Ship.h"

class PlayerShip : public Ship {
public:
	PlayerShip();

	virtual void ProcessAI(std::shared_ptr<PlayerShip> playerShip, int timeInMs) override;

	bool AutoAim() const { return autoAim; }
	void SetAutoAim(bool value);
	int MaxGearForType(Gear::Type type) const;

	List<Gear> Equipped(Gear::Type byType) const;
	List<Gear> EquippedWeapons() const;
	List<Gear> EquippedShieldGenerators() const;
	List<Gear> EquippedArmorLayers() const;

	// Equip given gear, if possible.
	bool Equip(const Gear& gear);
	// Equip given gear, replacing the other gear.
	void Equip(const Gear& gear, const int replacingGearIndex);

	void UnequipWeapons();
	void EquipTutorialLevel1Weapons();
	void EquipTutorialLevel3Weapons();

	// Unequips given gear, assuming it's already equipped.
	bool UnequipGear(const Gear& gear);

	// For AIs, limits it linearly when within 10 distance
	void SetAIVelocityVector(PlayingLevel& playingLevel, Vector3f vector);

	// Updates weapon levels, armor upgrades, shield upgrades, etc. based on save game vars
	void UpdateGearFromVars();
	void SaveGearToVars();

	/// If using Armor and Shield gear (Player mainly).
	void UpdateStatsFromGear();

	int maxWeapons = 5;
	int maxArmorLayers = 3;
	int maxShieldGenerators = 3;

private:
	bool autoAim;

	/// Used by player, mainly.
	List<Gear> weapons;
	List<Gear> shieldGenerators;
	List<Gear> armorLayers;
};


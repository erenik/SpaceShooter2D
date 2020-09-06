/// Emil Hedemalm
/// 2020-09-06
/// Player-ship specific code

#pragma once

#include "Base/Ship.h"

class PlayerShip : public Ship {
public:
	PlayerShip();
	// Updates weapon levels, armor upgrades, shield upgrades, etc. based on save game vars
	void UpdateGearFromVars();

	/// If using Armor and Shield gear (Player mainly).
	void UpdateStatsFromGear();

private:
	/// Used by player, mainly.
	Gear weapon, shield, armor;
};


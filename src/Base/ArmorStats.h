/// Emil Hedemalm
/// 2020
/// Cleaning up stats from Gear

#pragma once

class ArmorStats {
public:
	ArmorStats() {
		toughness = 10;
		reactivity = 1;
	}

	// Default 10. Higher values will reduce all damage.
	int toughness;
	// Default 0. Higher values will reduce incoming projectile damage. Does not affect collision damage.
	int reactivity;
};

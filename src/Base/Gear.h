/// Emil Hedemalm
/// 2015-02-06
/// o.o

#ifndef GEAR_H
#define GEAR_H

#include "String/AEString.h"
#include "Time/Time.h"
#include "Weapon.h"
#include "ArmorStats.h"

class Gear
{
	friend class Weapon;
public:
	Gear();
	String name;
	int price;
#define WEAPON Weapon
#define ARMOR Armor
#define SHIELD_GENERATOR Shield
	enum class Type {
		All,
		Weapon,
		Armor,
		Shield,
		AllCategories
	};
	Type type;

	// If type Weapon, may include the weapon info... hopefully.
	Weapon weapon;

	// Weapon stats?
	int damage;
	Time reloadTime;
	// Shield stats.
	int maxShield;
	float shieldRegen;
	// Armor stats
	int maxHP;
	float armorRegen;

	ArmorStats armorStats;

	// Default 10. Higher values will reduce all damage.
	int Toughness() const { return armorStats.toughness; };
	// Default 0. Higher values will reduce incoming projectile damage. Does not affect collision damage.
	int Reactivity() const { return armorStats.reactivity; };

	String description;

	static String TypeIcon(Type type);
	String Icon();

	/// o.o
	static bool Load(String fromFile);
	static List<Gear> GetType(Type type);
	static Gear Get(String byName);
	static List<Gear> GetGearList(List<String> byNames);

	static bool Get(String byName, Gear& gear);
	static List<Gear> GetAllOfType(Type type);
	static List<Gear> GetAllOwnedOfType(Type type);

	static Gear StartingWeapon();
	static Gear StartingArmor();
	static Gear StartingShield();

	static void SetOwned(const Gear& gear);
	static void SetOwned(const Gear& gear, int count);
	static bool Owns(const Gear& gear);

	static void SetEquippedWeapons(List<Gear> weapons);
	static void SetEquippedArmorLayers(List<Gear> armor);
	static void SetEquippedShieldGenerators(List<Gear> shield);
	static List<Gear> EquippedWeapons();
	static List<Gear> EquippedArmorLayers();
	static List<Gear> EquippedShieldGenerators();

	/// Available to buy/use, etc. Basically all loaded content in the game.
	static List<Gear> AvailableGear();

private:
	static List<Gear> weapons, armorAndShields;
};

String toString(Gear::Type type);
Gear::Type gearTypeFromString(String str);

bool operator ==(const Gear& one, const Gear& two);

#endif

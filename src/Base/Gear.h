/// Emil Hedemalm
/// 2015-02-06
/// o.o

#ifndef GEAR_H
#define GEAR_H

#include "String/AEString.h"
#include "Time/Time.h"

class Gear 
{
public:
	Gear();
	String name;
	int price;
	enum class Type {
		WEAPON,
		SHIELD_GENERATOR,
		ARMOR,
	};
	Type type;
	// Weapon stats?
	int damage;
	Time reloadTime;
	// Shield stats.
	int maxShield;
	float shieldRegen;
	// Armor stats
	int maxHP;
	float armorRegen;
	// Default 10. Higher values will reduce collision damage. Lower values will increase collision damage.
	int toughness;
	// Default 0. Higher values will reduce incoming projectile damage
	int reactivity;

	String description;

	/// o.o
	static bool Load(String fromFile);
	static List<Gear> GetType(Type type);
	static Gear Get(String byName);

	static bool Get(String byName, Gear& gear);

	static Gear StartingWeapon();
	static Gear StartingArmor();
	static Gear StartingShield();

	static void SetEquipped(const Gear& gear);
	static void SetOwned(const Gear& gear);
	static bool Owns(const Gear& gear);

	static void SetEquippedArmor(Gear armor);
	static void SetEquippedShield(Gear shield);
	static Gear EquippedArmor();
	static Gear EquippedShield();

	/// Available to buy!
	static List<Gear> availableGear;
};

#endif

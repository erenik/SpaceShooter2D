/// Emil Hedemalm
/// 2015-02-06
/// o.o

#include "Gear.h"
#include "File/File.h"
#include "String/StringUtil.h"
#include "Game/GameVariableManager.h"

/// Available to buy! .. what?
List<Gear> Gear::AvailableGear() {
	return weapons + armorAndShields;
};
List<Gear> Gear::weapons;
List<Gear> Gear::armorAndShields;

String toString(Gear::Type type) {
	switch (type) {
	case Gear::Type::All: return "All";
	case Gear::Type::Weapon: return "Weapon";
	case Gear::Type::Armor: return "Armor";
	case Gear::Type::Shield: return "Shield";
	case Gear::Type::AllCategories: return "AllGearTypes";
	}
	assert(false);
	return "";
}

Gear::Type gearTypeFromString(String str) {
	for (int i = 0; i < int(Gear::Type::AllCategories); ++i) {
		Gear::Type type = Gear::Type(i);
		if (str == toString(type))
			return type;
	}
	assert(false);
	return Gear::Type::AllCategories;
}

bool operator ==(const Gear& one, const Gear& two) {
	if (one.name == two.name)
		return true;
	return false;
}


Gear::Gear()
{
	armorStats = ArmorStats();
	price = -1;
	maxHP = -1;
	maxShield = -1;
	reloadTime = Time(TimeType::MILLISECONDS_NO_CALENDER, 0);
}

Gear::Type ParseType(String fromString) {
	if (fromString == "Weapon")
		return Gear::Type::WEAPON;
	if (fromString == "Armor")
		return Gear::Type::ARMOR;
	assert(false);
}

String Gear::TypeIcon(Type type) {
	return "img/icons/GearType/"+toString(type);
}

String Gear::Icon() {
	switch (type) {
	case Type::Weapon: {
		return weapon.Icon();
	case Type::Armor:
	case Type::Shield:
		return TypeIcon(type);
		break;
	default:
		return TypeIcon(type);
	}
	}
	assert(false);
	return "";
}


/// o.o
bool Gear::Load(String fromFile)
{
	List<String> lines = File::GetLines(fromFile);
	if (lines.Size() == 0)
		return false;
	String separator;
	/// Column-names. Parse from the first line.
	List<String> columns;
	String firstLine = lines[0];
	int commas = firstLine.Count(',');
	int semiColons = firstLine.Count(';');
	int delimiter = semiColons > commas? ';' : ',';
	columns = TokenizeCSV(firstLine, delimiter);
	for (int j = 1; j < lines.Size(); ++j)
	{
		String & line = lines[j];
		Gear gear;
		if (fromFile.Contains("Shield"))
			gear.type = Gear::Type::SHIELD_GENERATOR;
		else if (fromFile.Contains("Armor"))
			gear.type = Gear::Type::ARMOR;
		else 
			gear.type = Gear::Type::WEAPON;
		List<String> values = TokenizeCSV(line, delimiter);
		for (int k = 0; k < values.Size(); ++k)
		{
			String column;
			bool error = false;
			/// In-case extra data is added beyond the columns defined above..?
			if (columns.Size() > k)
				column = columns[k];
			String value = values[k];
			if (value == "n/a")
				continue;
			column.SetComparisonMode(String::NOT_CASE_SENSITIVE);
			if (column == "Name")
			{
				value.RemoveSurroundingWhitespaces();
				gear.name = value;
			}
			else if (column == "Type")
				gear.type = ParseType(value);
			else if (column == "Price")
				gear.price = value.ParseInt();
			else if (column == "Damage")
				gear.damage = value.ParseInt();
			else if (column == "Reload time")
				gear.reloadTime.intervals = value.ParseInt();
			else if (column == "Max Shield")
				gear.maxShield = value.ParseInt();
			else if (column == "Shield Regen")
				gear.shieldRegen = value.ParseInt();
			else if (column == "Max HP")
				gear.maxHP = value.ParseInt();
			else if (column == "ArmorRegen")
				gear.armorRegen = value.ParseFloat();
			else if (column == "Toughness")
				gear.armorStats.toughness = value.ParseInt();
			else if (column == "Reactivity")
				gear.armorStats.reactivity = value.ParseInt();
			else if (column == "Info")
				gear.description = value;
		}
		// Remove copies or old data.
		for (int i = 0; i < armorAndShields.Size(); ++i)
		{
			if (armorAndShields[i].name == gear.name)
			{
				armorAndShields.RemoveIndex(i);
				--i;
			}
		}
		armorAndShields.Add(gear);
	}
}

List<Gear> Gear::GetType(Gear::Type type)
{
	List<Gear> list;
	List<Gear> availableGear = AvailableGear();
	for (int i = 0; i < availableGear.Size(); ++i)
	{
		if (availableGear[i].type == type)
			list.Add(availableGear[i]);
	}
	return list;
}

Gear Gear::Get(String byName)
{
	List<Gear> availableGear = AvailableGear();
	for (int i = 0; i < availableGear.Size(); ++i)
	{
		if (availableGear[i].name == byName)
			return availableGear[i];
	}
	assert(false);
	return Gear();	
}

bool Gear::Get(String byName, Gear& gear) {
	List<Gear> availableGear = AvailableGear();
	for (int i = 0; i < availableGear.Size(); ++i)
	{
		if (availableGear[i].name == byName) {
			gear = availableGear[i];
			return true;
		}
	}
	return false;
}

List<Gear> Gear::GetAllOwnedOfType(Type type) {
	List<Gear> allOfType = GetAllOfType(type);
	List<Gear> owned;
	for (int i = 0; i < allOfType.Size(); ++i) {
		Gear gear = allOfType[i];
		if (Owns(gear)) {
			owned.Add(gear);
		}
	}
	return owned;
}


List<Gear> Gear::GetAllOfType(Type type) {
	List<Gear> allOfType;
	List<Gear> availableGear = AvailableGear();
	for (int i = 0; i < availableGear.Size(); ++i) {
		if (availableGear[i].type == type) {
			allOfType.Add(availableGear[i]);
		}
	}
	return allOfType;
}

Gear Gear::StartingWeapon()
{
	List<Gear> weapons = GetType(Gear::Type::WEAPON);
	for (int i = 0; i < weapons.Size(); ++i)
	{
		Gear & weapon = weapons[i];
		if (weapon.price == 0)
			return weapon;
	}
	assert(false);
	return Gear();
}
Gear Gear::StartingArmor()
{
	List<Gear> armors = GetType(Gear::Type::ARMOR);
	for (int i = 0; i < armors.Size(); ++i)
	{
		Gear & armor = armors[i];
		if (armor.price == 0)
			return armor;
	}
	assert(false);
	return Gear();
}
Gear Gear::StartingShield()
{
	List<Gear> shields = GetType(Gear::Type::SHIELD_GENERATOR);
	for (int i = 0; i < shields.Size(); ++i)
	{
		Gear & shield = shields[i];
		if (shield.price == 0)
			return shield;
	}
	assert(false);
	return Gear();
}

void Gear::SetOwned(const Gear& gear) {
	GameVars.SetInt(gear.name, 1);
}

void Gear::SetOwned(const Gear& gear, int count) {
	GameVars.SetInt(gear.name, count);
}

bool Gear::Owns(const Gear& gear) {
	GameVar* var = GameVars.Get(gear.name);
	if (var == nullptr)
		return false;
	return var->iValue > 0;
}

List<String> GetGearNames(List<Gear> gear) {
	List<String> names;
	for (int i = 0; i < gear.Size(); ++i)
		names.Add(gear[i].name);
	return names;
}

const String VarEquippedWeapons = "EquippedWeapons",
	VarEquippedArmor = "EquippedArmor",
	VarEquippedShield = "EquippedShield";

const String VarGlue = ";";

void Gear::SetEquippedWeapons(List<Gear> weapons) {
	GameVars.SetString(VarEquippedWeapons, MergeLines(GetGearNames(weapons), VarGlue));
}
void Gear::SetEquippedArmorLayers(List<Gear> armorLayers) {
	GameVars.SetString(VarEquippedArmor, MergeLines(GetGearNames(armorLayers), VarGlue));
}
void Gear::SetEquippedShieldGenerators(List<Gear> shieldGenerators) {
	GameVars.SetString(VarEquippedShield, MergeLines(GetGearNames(shieldGenerators), VarGlue));
}

List<Gear> Gear::GetGearList(List<String> fromNames) {
	List<Gear> gearList;
	for (int i = 0; i < fromNames.Size(); ++i) {
		Gear gear = Gear::Get(fromNames[i]);
		gearList.Add(gear);
	}
	return gearList;
}

List<Gear> Gear::EquippedWeapons() {
	GameVar * equippedWeapon = GameVars.GetString(VarEquippedWeapons);
	if (equippedWeapon == nullptr)
		return Gear::StartingWeapon();
	return GetGearList(equippedWeapon->strValue.Tokenize(VarGlue));
}

List<Gear> Gear::EquippedArmorLayers() {
	GameVar * equippedArmor = GameVars.GetString(VarEquippedArmor);
	if (equippedArmor == nullptr)
		return Gear::StartingArmor();
	return GetGearList(equippedArmor->strValue.Tokenize(VarGlue));
}

List<Gear> Gear::EquippedShieldGenerators() {
	GameVar * equippedShield = GameVars.GetString(VarEquippedShield);
	if (equippedShield == nullptr)
		return Gear::StartingShield();
	return GetGearList(equippedShield->strValue.Tokenize(VarGlue));
}



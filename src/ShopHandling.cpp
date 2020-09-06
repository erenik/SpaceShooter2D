/// Emil Hedemalm
/// 2015-10-01
/// Handling of shop-purchase-selling code

#include "SpaceShooter2D.h"
#include "PlayingLevel.h"

// For shop/UI-interaction.
Vector2i WeaponTypeLevelFromString(String str)
{
	String weaponString = str - "Weapon";
	int weapon = weaponString.ParseInt();
	String levelString = str.Tokenize("_")[1] - "Level";
	int level = levelString.ParseInt();
	return Vector2i(weapon, level);
} 

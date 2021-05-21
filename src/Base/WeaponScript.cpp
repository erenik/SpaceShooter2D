
/// Emil Hedemalm
/// 2015-10-04
/// Editable user-scripts

#include "WeaponScript.h"
#include "File/LogFile.h"

List<WeaponScript*> weaponScripts;

String ScriptAction::GetStringForType(int type)
{
	switch(type)
	{
		case SWITCH_TO_WEAPON:
			return "Switch to weapon";
		default:
			return "Bad type";
	}
}

ScriptAction ScriptAction::SwitchWeapon(int toWeaponIndex, int durationToHoldMs)
{
	ScriptAction sa;
	sa.type = SWITCH_TO_WEAPON;
	sa.name = ScriptAction::GetStringForType(sa.type);
	sa.weaponIndex = toWeaponIndex;
	sa.durationMs = durationToHoldMs;
	return sa;
}

ScriptAction::ScriptAction()
{
	Nullify();
}

ScriptAction::ScriptAction(int in_type)
{
	Nullify();
	this->type = in_type;
	name = ScriptAction::GetStringForType(type);
}
void ScriptAction::Nullify()
{
	type = -1;
	weaponIndex = 0;
	durationMs = 0;
}


void ScriptAction::OnEnter(Ship* forShip)
{
	if (type == SWITCH_TO_WEAPON)
	{
		if (weaponIndex >= forShip->weaponSet.Size()) {
			LogMain("Unable to switch to weapon. Given index is not present in the active weapon-set.", INFO);
			return;
		}
		forShip->activeWeapon = forShip->weaponSet[weaponIndex];
		forShip->shoot = true;
	}
}

WeaponScript * lastEdited = 0;

WeaponScript::WeaponScript()
{
	timeInCurrentActionMs = 0;
	currentAction = 0;
	static int numScripts = 0;
	name = "Weapon script "+String(numScripts++);
	lastEdited = this;
}

/*
WeaponScript* WeaponScript::CreateDefault()
{
	WeaponScript * weaponScript = new WeaponScript();
	weaponScript->AddDefaultWeapons();
	weaponScripts.AddItem(weaponScript);
	return weaponScript;
}*/

void WeaponScript::AddDefaultWeapons() {
	actions.AddItem(ScriptAction::SwitchWeapon(0, 1000));
	actions.AddItem(ScriptAction::SwitchWeapon(1, 500));
	actions.AddItem(ScriptAction::SwitchWeapon(2, 100));
}


void WeaponScript::Process(Ship* forShip, int timeInMs)
{
	assert(actions.Size());
	timeInCurrentActionMs += timeInMs;
	ScriptAction & current = actions[currentAction];
	if (timeInCurrentActionMs > current.durationMs)
	{
		currentAction = (currentAction + 1) % actions.Size();
		// When entering a new one, do stuff.
		ScriptAction & newOne = actions[currentAction];
		newOne.OnEnter(forShip);
		timeInCurrentActionMs = 0;
	}
}

WeaponScript * WeaponScript::LastEdited()
{
	return lastEdited;
}

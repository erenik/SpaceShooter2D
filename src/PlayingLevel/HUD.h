/** 
	In-game HUD while playing/flying through the level.
	Emil Hedemalm
*/
#pragma once

class Message;

class HUD {
private:
	static HUD hud;
public:
	static HUD* Get();


	void Show();
	void Hide();


	// Checks GameVars if it has changed and updates visibility if so.
	void UpdateDebugVisibility();

	// Updates all
	void UpdateUI();

	void UpdateActiveWeapon();

	// Updates part of it
	void UpdateHUDGearedWeapons();
	// Only if cooldowns not already created.
	void UpdateHUDGearedWeaponsIfNeeded();
	void UpdateUIPlayerHP(bool force);
	void UpdateUIPlayerShield(bool force);
	void UpdateCooldowns();
	void UpdateDebug();

	void ShowLevelStats();

	void HideLevelStats();


	void ProcessMessage(Message* message);

	void OpenInGameMenu();
	void CloseInGameMenu();
	bool IsMenuOpen();

	bool activeWeaponsShown = false;
private:
	bool inGameMenuOpened = false;
	bool showLevelStats = false;
	bool overlaysCreated = false;


};

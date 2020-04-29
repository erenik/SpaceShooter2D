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

	// Updates all
	void UpdateUI();

	// Updates part of it
	void UpdateHUDGearedWeapons();
	void UpdateUIPlayerHP(bool force);
	void UpdateUIPlayerShield(bool force);
	void UpdateCooldowns();
	void ShowLevelStats();

	void HideLevelStats();


	void ProcessMessage(Message* message);

	void OpenInGameMenu();
	void CloseInGameMenu();

	bool activeWeaponsShown = false;
private:
	bool inGameMenuOpened = false;
	bool showLevelStats = false;


};

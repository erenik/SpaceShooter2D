/**
	In-game HUD while playing/flying through the level.
	Emil Hedemalm
*/

#include "PlayingLevel/HUD.h"
#include "PlayingLevel.h"
#include "Graphics/GraphicsManager.h"
#include "Graphics/Messages/GMUI.h"
#include "Base/Weapon.h"
#include "Window/AppWindow.h"
#include "PlayingLevel.h"
#include "UI/UIUtil.h"
#include "Input/InputManager.h"
#include "Level/SpawnGroup.h"

HUD HUD::hud = HUD();

String hudPath = "gui/HUD.gui";

static int lastHP;
static int lastShield;


HUD* HUD::Get() {
	return &hud;
}

void HUD::Show() {
	
	overlaysCreated = false;
	PushUI(hudPath);
	// By default remove hover from any UI element. Select weapons with 1-9 keys.
	QueueGraphics(new GMSetHoverUI(nullptr));

	lastHP = lastShield = 0;
}

void HUD::Hide() {
	PopUI(hudPath);
}

void HUD::UpdateUI() {
	UpdateActiveWeapon();
	UpdateHUDGearedWeapons();
	UpdateUIPlayerHP(true);
	UpdateUIPlayerShield(true);

	if (PlayingLevelRef().mode == PLAYING_LEVEL)
		PlayingLevelRef().OpenSpawnWindow();
	else
		PlayingLevelRef().CloseSpawnWindow();
}

void HUD::UpdateActiveWeapon() {
	PlayingLevel& pl = PlayingLevelRef();
	Weapon* activeWeapon = pl.playerShip->activeWeapon;
	if (!activeWeapon)
		return;
	QueueGraphics(new GMSetUIs("ActiveWeaponName", GMUI::TEXT, activeWeapon->name));
	String status;
	if (activeWeapon->reloading) {
		status = "Reloading...";
  		float reloadRatio = activeWeapon->currCooldownMs / float(activeWeapon->cooldown.Milliseconds());
		QueueGraphics(new GMSetUIs("ActiveWeaponAmmo", GMUI::TEXTURE_SOURCE, "0xFFFF00"));
		QueueGraphics(new GMSetUIf("ActiveWeaponAmmo", GMUI::BAR_FILL_RATIO, 1 - reloadRatio));
	}
	else if (activeWeapon->shotsLeft > 0) {
		status = "Ammo " + String(activeWeapon->shotsLeft);
		float shotsLeftRatio = activeWeapon->shotsLeft / float(activeWeapon->burstRounds);
		QueueGraphics(new GMSetUIs("ActiveWeaponAmmo", GMUI::TEXTURE_SOURCE, "0xFFFFFF"));
		QueueGraphics(new GMSetUIf("ActiveWeaponAmmo", GMUI::BAR_FILL_RATIO, shotsLeftRatio));
	}
	QueueGraphics(new GMSetUIs("ActiveWeaponStatus", GMUI::TEXT, status));
}

void RequeueHUDUpdate()
{
	/// Queue an update for later?
	Message* msg = new Message("UpdateHUDGearedWeapons");
	msg->timeToProcess = Time::Now() + Time::Milliseconds(1000);
	MesMan.QueueDelayedMessage(msg);
}

/// Update UI
void HUD::UpdateHUDGearedWeapons()
{
	MutexHandle mh(uiMutex);
	// Fetch the names of the checkboxes.
	UserInterface* ui = MainWindow()->ui;
	if (!ui)
	{
		RequeueHUDUpdate();
		return;
	}
	UIElement* activeWeapon = ui->GetElementByName("Weapons");
	if (!activeWeapon)
	{
		RequeueHUDUpdate();
		return;
	}

	PlayingLevel& pl = PlayingLevelRef();

	// Fetch children.
	assert(activeWeapon);
	List<UIElement*> children = activeWeapon->GetChildren();
	List<Weapon*>& weapons = pl.playerShip->weapons;
	for (int i = 0; i < children.Size(); ++i)
	{
		UIElement* child = children[i];
		if (i >= weapons.Size())
		{
			QueueGraphics(new GMSetUIs(child->name, GMUI::TEXT, "."));
			QueueGraphics(new GMSetUIb(child->name, GMUI::ACTIVATABLE, false));
			QueueGraphics(new GMSetUIb(child->name, GMUI::HOVERABLE, false));
			continue;
		}
		Weapon* weapon = weapons[i];
		QueueGraphics(new GMSetUIs(child->name, GMUI::TEXT, weapon->name));
		QueueGraphics(new GMSetUIb(child->name, GMUI::ACTIVATABLE, true));
		QueueGraphics(new GMSetUIb(child->name, GMUI::HOVERABLE, true));

		String overlayName = "WeaponCooldownOverlay" + String(i);
		String textureSource = "img/ui/Cooldown_37.png";
		if (overlaysCreated) {
			QueueGraphics(new GMSetUIs(overlayName, GMUI::TEXTURE_SOURCE, textureSource));
		}
		else {
			overlaysCreated = true;
			/// Clear and add associated picture and cooldown-overlay on top.
			QueueGraphics(new GMClearUI(child->name));
			UIElement* cooldownOverlay = new UIElement();
			cooldownOverlay->name = overlayName;
			cooldownOverlay->textureSource = textureSource;
			QueueGraphics(new GMAddUI(cooldownOverlay, child->name));

			// If weapon begs question of ammo (bursts).
			if (weapon->burst)
			{
				UIElement* ammunition = new UIElement();
				ammunition->name = "WeaponAmmunitionOverlay" + String(i);
				ammunition->textureSource = "0x0000";
				ammunition->text = "inf";
				QueueGraphics(new GMAddUI(ammunition, child->name));
			}
		}
	}
}

// Only if cooldowns not already created.
void HUD::UpdateHUDGearedWeaponsIfNeeded() {
	if (overlaysCreated)
		return;
	UpdateHUDGearedWeapons();
}

void HUD::UpdateUIPlayerHP(bool force)
{
	PlayingLevel& pl = PlayingLevelRef();
	if (pl.playerShip == nullptr)
		return;
	if (lastHP == pl.playerShip->hp && !force)
		return;
	lastHP = (int)pl.playerShip->hp;
	GraphicsMan.QueueMessage(new GMSetUIi("HP", GMUI::INTEGER_INPUT, (int)pl.playerShip->hp));
	float ratio = pl.playerShip->hp / (float)pl.playerShip->maxHP;
	GraphicsMan.QueueMessage(new GMSetUIf("HPBar", GMUI::BAR_FILL_RATIO, ratio));
}
void HUD::UpdateUIPlayerShield(bool force)
{
	PlayingLevel& pl = PlayingLevelRef();
	if (pl.playerShip == nullptr)
		return;
	if (lastShield == pl.playerShip->shieldValue && !force)
		return;
	lastShield = (int)pl.playerShip->shieldValue;
	GraphicsMan.QueueMessage(new GMSetUIi("Shield", GMUI::INTEGER_INPUT, (int)pl.playerShip->shieldValue));
	float ratio = (float) pl.playerShip->shieldValue / pl.playerShip->MaxShield();
	GraphicsMan.QueueMessage(new GMSetUIf("ShieldBar", GMUI::BAR_FILL_RATIO, ratio));
}

void HUD::UpdateCooldowns()
{
	if (!activeWeaponsShown)
		return;

	PlayingLevel& pl = PlayingLevelRef();
	if (pl.playerShip == nullptr)
		return;

	List<Weapon*>& weapons = pl.playerShip->weapons;
	for (int i = 0; i < weapons.Size(); ++i)
	{
		Weapon* weapon = weapons[i];
		if (weapon->currCooldownMs == weapon->previousUIUpdateCooldownMs)
			continue;
		// Update it here.
		weapon->previousUIUpdateCooldownMs = weapon->currCooldownMs;

		float timeTilNextShotMs = (float)weapon->currCooldownMs;
		//		if (weapon->burst)
			//		timeTilNextShotMs = (flyTime - weapon->burstStart).Milliseconds();
		float maxCooldown = (float)weapon->cooldown.Milliseconds();
		float ratioReady = (1 - timeTilNextShotMs / maxCooldown) * 100.f;
		// Change texture accordingly.
		List<int> avail(0, 12, 25, 37);
		avail.Add(50, 62, 75, 87, 100);
		int good = 0;
		for (int j = 0; j < avail.Size(); ++j)
		{
			int av = avail[j];
			if (ratioReady < av)
				break;
			good = av;
		}
		QueueGraphics(new GMSetUIs("WeaponCooldownOverlay" + String(i), GMUI::TEXTURE_SOURCE, "img/ui/Cooldown_" + String(good) + ".png"));
		if (weapon->burst)
		{
			int shotsLeft = weapon->shotsLeft;
			if (weapon->reloading)
				shotsLeft = 0;
			//			int target = GMUI::TEXT;
			QueueGraphics(new GMSetUIs("WeaponAmmunitionOverlay" + String(i), GMUI::TEXT, String(shotsLeft)));
		}
	}
}

void HUD::UpdateDebug() {
	PlayingLevel& pl = PlayingLevelRef();
	QueueGraphics(new GMSetUIs("LevelTime", GMUI::STRING_INPUT, pl.levelTime.ToString("m:S")));
	QueueGraphics(new GMSetUIi("SpawnGroupsRemaining", GMUI::INTEGER_INPUT, pl.level.SpawnGroupsRemaining()));
	QueueGraphics(new GMSetUIi("SpawnGroupsActive", GMUI::INTEGER_INPUT, pl.level.SpawnGroupsActive()));
	
	SpawnGroup * nextSpawnGroup = pl.level.NextSpawnGroup();
	QueueGraphics(new GMSetUIs("NextSpawnGroupTime", GMUI::STRING_INPUT, (nextSpawnGroup != nullptr? nextSpawnGroup->SpawnTime().ToString("m:S") : "N/A" )));
}

String levelStatsGui = "gui/LevelStats.gui";

void HUD::ShowLevelStats()
{
	PushUI(levelStatsGui);

	PlayingLevel& pl = PlayingLevelRef();
	std::cout << "\n Kills : " << pl.LevelKills()->ToString() << " of possible: " << pl.LevelPossibleKills()->ToString();
	pl.mode = SHOWING_LEVEL_STATS;
	GraphicsMan.QueueMessage(new GMSetUIs("LevelKills", GMUI::TEXT, pl.LevelKills()->ToString()));
	QueueGraphics(new GMSetUIs("TotalKillsPossible", GMUI::TEXT, pl.LevelPossibleKills()->ToString()));
	GraphicsMan.QueueMessage(new GMSetUIs("LevelScore", GMUI::TEXT, pl.LevelScore()->ToString()));
	GraphicsMan.QueueMessage(new GMSetUIs("ScoreTotal", GMUI::TEXT, pl.score->ToString()));
	showLevelStats = true;
}

void HUD::HideLevelStats() {
	PopUI(levelStatsGui);
	showLevelStats = false;
}

String inGameMenuGui = "gui/InGameMenu.gui";

void HUD::OpenInGameMenu()
{
	inGameMenuOpened = true;
	PushUI(inGameMenuGui);
	InputMan.ForceNavigateUI(true);
}

void HUD::CloseInGameMenu()
{
	PopUI(inGameMenuGui);
	InputMan.ForceNavigateUI(false);
}



void HUD::ProcessMessage(Message* message) {
	String msg = message->msg;
	switch (message->type)
	{
		case MessageType::STRING:
		{
			msg.RemoveSurroundingWhitespaces();
			int found = msg.Find("//");
			if (found > 0)
				msg = msg.Part(0, found);


			if (!false) {}
			else if (msg.StartsWith("ShowLevelStats"))
				ShowLevelStats();
			else if (msg.StartsWith("HideLevelStats"))
				HideLevelStats();
			else if (msg == "UpdateHUDGearedWeapons")
				UpdateHUDGearedWeapons();
		}
	}
}

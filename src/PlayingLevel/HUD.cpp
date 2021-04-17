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
#include "Base/PlayerShip.h"

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
	String status;
	/*
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
	*/

	for (int i = 0; i < pl.playerShip->weaponSet.Size(); ++i) {
		Weapon * weapon = pl.playerShip->weaponSet[i];
		const String loadedPassiveWeapon = "0x89A";
		const String loadedActiveWeapon = "0xABC";
		const String loadingActiveWeapon = "0x404550";
		const String loadingPassiveWeapon = "0x253035";

		//QueueGraphics(new GMSetUIs("HUDWeaponStatus" + String(i), GMUI::TEXTURE_SOURCE,
		//	weapon->reloading? (activeWeapon == weapon? loadingActiveWeapon : loadingPassiveWeapon) :
		//	activeWeapon == weapon ? loadedActiveWeapon : loadedPassiveWeapon));

		String hudWeaponStatusName = "HUDWeaponStatus" + String(i);

		//QueueGraphics(new GMSetUIb(hudWeaponStatusName, GMUI::HOVER_STATE, activeWeapon == weapon));
		QueueGraphics(new GMSetUIb("WeaponName"+String(i), GMUI::HOVER_STATE, activeWeapon == weapon));
		//QueueGraphics(new GMSetUIb(hudWeaponStatusName, GMUI::TOGGLED, !weapon->reloading));
		
		QueueGraphics(new GMSetUIs("Ammunition" + String(i), GMUI::TEXT, String(weapon->shotsLeft)));
		QueueGraphics(new GMSetUIv4f("Ammunition" + String(i), GMUI::TEXT_COLOR, weapon->reloading ? Vector4f(0.6f, 0.6f, 0.6f,1) : Vector4f(0.9f, 0.9f, 0.9f,1)));
		QueueGraphics(new GMSetUIs("Cooldown" + String(i), GMUI::TEXT, String(weapon->currCooldownMs / 1000.0f, 1)));
	}

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

	// Remove old ones first.
	QueueGraphics(new GMClearUI("clWeapons"));

	ShipPtr ship = GetPlayerShip();
	List<UIElement*> weaponStatuses;
	for (int i = 0; i < ship->weaponSet.Size(); ++i) {
		Weapon * weapon = ship->weaponSet[i];
		UIElement * weaponStatus = UserInterface::LoadUIAsElement("gui/HUDWeaponStatus.gui");
		weaponStatus->name += String(i);
		weaponStatus->GetElementByName("Icon")->textureSource = weapon->Icon();
		
		UIElement * weaponName = weaponStatus->GetElementByName("WeaponName");
		weaponName->SetText(weapon->name);
		weaponName->name += String(i);

		weaponStatus->GetElementByName("Ammunition")->SetText(String(weapon->shotsLeft));
		weaponStatus->GetElementByName("Ammunition")->name += String(i);
		weaponStatus->GetElementByName("Cooldown")->name += String(i);
		weaponStatuses.Add(weaponStatus);
	}
	QueueGraphics(new GMAddUI(weaponStatuses, "clWeapons"));

		/*
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
		*/
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

	List<Weapon*>& weapons = pl.playerShip->weaponSet;
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
	Level& level = pl.GetLevel();

	QueueGraphics(new GMSetUIs("LevelTime", GMUI::STRING_INPUT, pl.levelTime.ToString("m:S")));
	QueueGraphics(new GMSetUIi("SpawnGroupsRemaining", GMUI::INTEGER_INPUT, level.SpawnGroupsRemaining()));
	QueueGraphics(new GMSetUIi("SpawnGroupsActive", GMUI::INTEGER_INPUT, level.SpawnGroupsActive()));
	
	SpawnGroup * nextSpawnGroup = level.NextSpawnGroup();
	QueueGraphics(new GMSetUIs("NextSpawnGroupTime", GMUI::STRING_INPUT, (nextSpawnGroup != nullptr? nextSpawnGroup->SpawnTime().ToString("m:S") : "N/A" )));
	QueueGraphics(new GMSetUIi("NextSpawnGroupLine", GMUI::INTEGER_INPUT, nextSpawnGroup != nullptr ? nextSpawnGroup->lineNumber : 0));


	/*
	IntegerLabel EnemyProjectilesDodged "Enemy projectiles dodged"
	IntegerLabel RoundsFired "Rounds fired"
	IntegerLabel ProjectileDamageTaken "Projectile damage taken"
	IntegerLabel ArmorRegenerated "Armor regenerated"
	IntegerLabel ShieldRegenerated "Shield regenerated"
	*/
	QueueGraphics(new GMSetUIs("EnemyProjectilesDodged", GMUI::STRING_INPUT, PlayingLevelRef().enemyProjectilesDodgedString));
	QueueGraphics(new GMSetUIi("RoundsFired", GMUI::INTEGER_INPUT, PlayingLevelRef().projectilesFired));
	QueueGraphics(new GMSetUIi("ProjectileDamageTaken", GMUI::INTEGER_INPUT, PlayingLevelRef().projectileDamageTaken));
	QueueGraphics(new GMSetUIi("ArmorRegenerated", GMUI::INTEGER_INPUT, PlayingLevelRef().armorRegenerated));
	QueueGraphics(new GMSetUIi("ShieldRegenerated", GMUI::INTEGER_INPUT, PlayingLevelRef().shieldRegenerated));
	
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
	GraphicsMan.QueueMessage(new GMSetUIs("LevelScore", GMUI::TEXT, String(pl.score)));
	GraphicsMan.QueueMessage(new GMSetUIs("ScoreTotal", GMUI::TEXT, String(pl.score)));
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
	InputMan.SetNavigateUI(true);
}

void HUD::CloseInGameMenu()
{
	PopUI(inGameMenuGui);
	InputMan.SetNavigateUI(false);
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

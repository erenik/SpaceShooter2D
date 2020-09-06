// Emil Hedemalm
// 2020-08-05
// In the hanger, interacting with NPCs, buying gear, observing the ship in 3D view...? :D 

#include "InHangar.h"

#include "Graphics/Messages/GMUI.h"
#include "Input/InputManager.h"
#include "UI/Input/UIStringInput.h"
#include "UI/Input/UIIntegerInput.h"
#include "Text/TextManager.h"
#include "Missions.h"
#include "File/LogFile.h"
#include "Base/Gear.h"
#include "Base/Ship.h"
#include "Base/PlayerShip.h"
#include "Message/MathMessage.h"

String hangarGui = "gui/Hangar.gui";
String missionsGui = "gui/Missions.gui";

InHangar::~InHangar() {
}

/// Function when entering this state, providing a pointer to the previous StateMan.
void InHangar::OnEnter(AppState * previousState) {

	relevantWeapons = GetRelevantWeapons();

	// Push Hangar UI 
	PushUI();
		
	/// Enable Input-UI navigation via arrow-keys and Enter/Esc.
	InputMan.SetNavigateUI(true);

	// Set up some 3D scene with the ship?
	SetUpScene();

	// Play different bgm or stop it.
	QueueAudio(new AMStopBGM());

	hangarMessageDisplayed = false;

	if (FirstTimeCompletion().Length() > 0 || RepeatCompletion().Length() > 0) {
		// Display dialog for it
		int reward = 1000;
		messageQueue.Add(HangarMessage(
			TextMan.GetText("MissionCompleted")
				.Replaced("$missionName", FirstTimeCompletion() + RepeatCompletion()), 
			0));
		Text text;
		if (FirstTimeCompletion().Length()) {
			text = TextMan.GetText("MissionClearedFirstTime");
		}
		else if (RepeatCompletion()) {
			text = TextMan.GetText("MissionClearedRepeat");
			reward = reward / 4; // 25% reward upon re-play to enable grinding?
		}
		messageQueue.Add(HangarMessage(text.Replaced("$moneyReward", String(reward)), reward));
	}

	playerShip = new PlayerShip();
	playerShip->UpdateStatsFromGear();

}

void InHangar::DisplayMessage(HangarMessage msg) {
	UIElement * element = UserInterface::LoadUIAsElement("gui/HangarMessage.gui");
	element->GetElementByName("HangarMessage")->SetText(msg.text);
	QueueGraphics(GMPushUI::ToUI(element));
	currentMessage = msg;
}

/// Main processing function, using provided time since last frame.
void InHangar::Process(int timeInMs) {
	if (messageQueue.Size() && !hangarMessageDisplayed) {
		hangarMessageDisplayed = true;
		DisplayMessage(messageQueue[0]);
		messageQueue.RemoveIndex(0, ListOption::RETAIN_ORDER);
	}

	if (displayedMoney != Money()) {
		if (Money() > displayedMoney) {
			displayedMoney += timeInMs;
			if (displayedMoney > Money())
				displayedMoney = Money();
		}
		else {
			displayedMoney -= timeInMs;
			if (displayedMoney < Money())
				displayedMoney = Money();
		}
		QueueGraphics(new GMSetUIi("Money", GMUI::INTEGER_INPUT, displayedMoney));
	}
}

/// Function when leaving this state, providing a pointer to the next StateMan.
void InHangar::OnExit(AppState * nextState) {
	QueueGraphics(new GMPopUI(hangarGui, true));
	QueueGraphics(new GMPopUI(missionsGui, true));
}


/// Callback function that will be triggered via the MessageManager when messages are processed.
void InHangar::ProcessMessage(Message* message)
{
	String msg = message->msg;
	switch (message->type)
	{
	case MessageType::MOUSE_MESSAGE:
		break;
	case MessageType::ON_UI_ELEMENT_HOVER:
	{
		if (msg.StartsWith("SetHoverUpgrade:"))
		{
			String upgrade = msg.Tokenize(":")[1];
			UpdateGearDetails(upgrade);
		}
		break;
	}
	case MessageType::INTEGER_MESSAGE:
	{
		IntegerMessage * im = (IntegerMessage*)message;
		if (msg == "SetGearCategory")
		{
			gearCategory = im->value == 0 ? Gear::Type::WEAPON : Gear::Type::ARMOR;
			UpdateGearList();
		}
	}
	case MessageType::STRING:
		if (msg == "OnReloadUI") {
			PushUI();
			if (hangarMessageDisplayed) {
				DisplayMessage(currentMessage);
			}
		}
		else if (msg.StartsWith("ActiveUpgrade:"))
		{
			String upgrade = msg.Tokenize(":")[1];
			//      if (previousActiveUpgrade == upgrade)
			BuySellToUpgrade(upgrade);
			UpdateHoverUpgrade(upgrade, true);
			//      UpdateActiveUpgrade(upgrade);
			previousActiveUpgrade = upgrade;
		}
		else if (msg.StartsWith("PlayMission")) {
			// Play the given mission!
			GameVars.SetString("CurrentMission", (msg - "PlayMission").WithSurroundingWhitespacesRemoved());
			SetMode(SSGameMode::PLAYING_LEVEL);
		}
		else if (msg == "HangarMessageConfirmed") {
			hangarMessageDisplayed = false; // Display next one next frame is needed.
			if (currentMessage.gainMoney > 0) {
				// Play SFX
				Money() += currentMessage.gainMoney;
			}
		}
		else if (msg.StartsWith("BuyGear:"))
		{
			LogMain("Reimplement", INFO);
			String name = msg;
			name.Remove("BuyGear:");
			Gear gear = Gear::Get(name);
			switch(gear.type)
			{
			case Gear::Type::SHIELD_GENERATOR:
				Gear::SetEquippedShield(gear);
				break;
			case Gear::Type::ARMOR:
				Gear::SetEquippedArmor(gear);
				break;
			}
			// Update stats.
			playerShip->UpdateStatsFromGear();
			/// Reduce munny
			Money() -= gear.price;
			/// Update UI to reflect reduced funds.
			UpdateGearList();
			// Play some SFX too?

			// Auto-save.
			MesMan.QueueMessages("AutoSave(silent)");			
		}
		break;
	case MessageType::ON_UI_PUSHED:
		if (msg == "MissionsScreen") {
			// Fill up the missions.
			PopulateMissionsList();
		}
		else if (msg == "WorkshopScreen") {
			UpdateGearList();
			UpdateUpgradesLists();
			//UpdateWeaponScriptUI();
			break;
		}
		break;
	default:
		std::cout << "Received unhandled message: "+ String(message->type);
	}
	// Process general messages.
	SpaceShooter2D::ProcessMessage(message);
}


void InHangar::PushUI() {
	// Remove old ones first.
	QueueGraphics(new GMPopUI("gui/Hangar.gui", true));

	UIElement * hangar = UserInterface::LoadUIAsElement("gui/Hangar.gui");
	((UIStringInput*)hangar->GetElementByName("Name"))->SetValue(PlayerName());
	((UIIntegerInput*)hangar->GetElementByName("TotalScore"))->SetValue(score->iValue);
	((UIIntegerInput*)hangar->GetElementByName("Money"))->SetValue(Money());
	((UIStringInput*)hangar->GetElementByName("Difficulty"))->SetValue(DifficultyString(difficulty->iValue));
	((UIStringInput*)hangar->GetElementByName("FlyTime"))->SetValue(FlyTime().ToString("H:m"));
	QueueGraphics(GMPushUI::ToUI(hangar));

	displayedMoney = Money();
}

void InHangar::PopulateMissionsList() {
	QueueGraphics(new GMClearUI("MissionsList"));
	Missions missions = MissionsMan.GetAvailableMissions();
	List<UIElement*> entries;
	for (int i = 0; i < missions.Size(); ++i) {
		Mission* mission = missions[i];
		UIElement * missionButton = UserInterface::LoadUIAsElement("gui/MissionButton.gui");
		(missionButton->GetElementByName("PlayMissionButton"))->SetText(mission->name);
		(missionButton->GetElementByName("PlayMissionButton"))->activationMessage = "PlayMission "+ mission->name;
		if (i == 0) // Hover to first one by default?
			(missionButton->GetElementByName("PlayMissionButton"))->SetState(UIState::HOVER);
		entries.Add(missionButton);
	}
	QueueGraphics(new GMAddUI(entries, "MissionsList"));
}

void InHangar::SetUpScene(){
	MapMan.DeleteAllEntities();
}

List<Weapon> InHangar::GetRelevantWeapons() {
	List<Weapon> relevantWeapons;
	relevantWeapons.Add(Weapon::GetAllOfType(Weapon::Type::MachineGun));
	relevantWeapons.Add(Weapon::GetAllOfType(Weapon::Type::SmallRockets));
	relevantWeapons.Add(Weapon::GetAllOfType(Weapon::Type::BigRockets));
	if (false)
		relevantWeapons.Add(Weapon::GetAllOfType(Weapon::Type::LaserBeam));
	if (false)
		relevantWeapons.Add(Weapon::GetAllOfType(Weapon::Type::LaserBurst));
	if (false)
		relevantWeapons.Add(Weapon::GetAllOfType(Weapon::Type::Lightning));
	if (false)
		relevantWeapons.Add(Weapon::GetAllOfType(Weapon::Type::IonCannon));
	if (false)
		relevantWeapons.Add(Weapon::GetAllOfType(Weapon::Type::HeatWave));

	// Pre-filter a bit.
	for (int i = 0; i < relevantWeapons.Size(); ++i) {
		Weapon& weapon = relevantWeapons[i];
		bool skip = false;
		if (weapon.cost > Money() * 2 + 1000) {
			skip = true;
		}
		if (Weapon::PlayerOwns(weapon)) {
			skip = false;
		}
		if (skip) {
			relevantWeapons.RemoveIndex(i, ListOption::RETAIN_ORDER);
			--i;
		}
	}
	return relevantWeapons;
}

#include "UI/Lists/UIColumnList.h"
#include "UI/UIButtons.h"

void InHangar::UpdateUpgradesLists()
{
	QueueGraphics(new GMClearUI("lWeaponCategories"));
	/// Fill with column lists for each weapon.
	List<UIElement*> weaponEntries;
	List<Weapon> guns = relevantWeapons;
	for (int i = 0; i < guns.Size(); ++i)
	{
		Weapon weapon = guns[i];
		Color textColor = UIElement::defaultTextColor;
		UIElement * buyWeaponButton = UserInterface::LoadUIAsElement("gui/BuyWeaponButton.gui");
		
		buyWeaponButton->activationMessage = "ActiveUpgrade:" + weapon.name;
		if (Weapon::PlayerOwns(weapon)) { // Owned
			buyWeaponButton->textureSource = "0x00FF00AA";
			textColor = Vector4f(0.9f, 0.9f, 0.9f, 1);
		}
		else { // Not owned
			buyWeaponButton->textureSource = "0x44AA";
			if (weapon.cost > Money()) {
				//buyWeaponButton->activateable = false;
				//buyWeaponButton->textureSource = "0x55555555";
				buyWeaponButton->textureSource = "0x34AA";
				textColor = Vector4f(1, 1, 1, 1) * 0.5f;
				buyWeaponButton->activationMessage = "PlayErrorSFX TODO";
			}
			else {
			}
		}



		buyWeaponButton->GetElementByName("WeaponName")->SetText(Text(weapon.name).WithColor(textColor));
		buyWeaponButton->GetElementByName("Price")->SetText(Text(weapon.cost).WithColor(textColor));
		buyWeaponButton->GetElementByName("Owned")->SetText(Weapon::PlayerOwns(weapon)? "Owned" : "");
		buyWeaponButton->onHover = "SetHoverUpgrade:" + weapon.name;

		weaponEntries.Add(buyWeaponButton);
	}
	QueueGraphics(new GMAddUI(weaponEntries, "lWeaponCategories"));
	UpdateUpgradeStatesInList();
}

void InHangar::UpdateUpgradeStatesInList()
{
	List<Weapon> guns = relevantWeapons;
	for (int i = 0; i < guns.Size(); ++i)
	{
		Weapon weapon = guns[i];
		String buttonName = weapon.name;
		String textureSource;
		Vector4f color(1,1,1,1);
		textureSource = "ui/SpaceShooterUpgrade_White";//textureSource = "0xFFFF00AA";
		if (Weapon::PlayerOwns(weapon))
			color = Vector4f(1,1,0,1);
		// Orange?
		// color = Color(102, 255, 0, 255);
		if (weapon.cost > Money())
			color = Vector4f(1, 0, 0, 1);
		else
			color = Vector4f(1, 1, 1, 1) * 0.25f;

		QueueGraphics(new GMSetUIv4f(buttonName, GMUI::COLOR, color));
		QueueGraphics(new GMSetUIs(buttonName, GMUI::TEXTURE_SOURCE, textureSource));
	}
}

String hoverUpgrade;

#include "UI/Input/UIStringInput.h"

void InHangar::UpdateGearDetails(String upgrade)
{
	if (upgrade == previousHoveredUpgrade)
		return;

	previousHoveredUpgrade = upgrade;

	Weapon weapon;
	bool isWeapon = Weapon::Get(upgrade, &weapon);
	QueueGraphics(new GMClearUI("SelectedStats"));

	UIElement* gearDesc = nullptr;
	if (isWeapon) {
		gearDesc = UserInterface::LoadUIAsElement("gui/GearWeaponDescription.gui");
		((UIStringInput*)gearDesc->GetElementByName("GearName"))->SetValue(TextMan.GetText(upgrade));
		((UIIntegerInput*)gearDesc->GetElementByName("GearPrice"))->SetValue(weapon.cost);
		((UIIntegerInput*)gearDesc->GetElementByName("GearDamage"))->SetValue((int)weapon.damage);
		((UIIntegerInput*)gearDesc->GetElementByName("GearBurstCooldown"))->SetValue((int)weapon.burstRoundDelay.Milliseconds());
		((UIIntegerInput*)gearDesc->GetElementByName("GearRounds"))->SetValue((int)weapon.burstRounds);
		((UIIntegerInput*)gearDesc->GetElementByName("GearCooldown"))->SetValue(weapon.cooldown);
		gearDesc->GetElementByName("GearTextDescription")->SetText(TextMan.GetText(weapon.name+"Desc"));

		// Depending on type, add extra statistics too that might be interesting? ^^
		switch (weapon.type)
		{
		case Weapon::Type::MachineGun:
			//tmpElements.AddItem(BasicLabel("Penetration: " + String(weapon->penetration, 2)));
			//tmpElements.AddItem(BasicLabel("Stability: " + String(weapon->stability, 2)));
			break;
		case Weapon::Type::SmallRockets:
			//tmpElements.AddItem(BasicLabel("Burst: " + String(weapon->burstRounds) + "/" + String(weapon->burstRoundDelay.Milliseconds())));
			//tmpElements.AddItem(BasicLabel("Homing: " + String(weapon->homingFactor, 2)));
			//tmpElements.AddItem(BasicLabel("Explosion radius: " + String(weapon->explosionRadius, 1)));
			break;
		/*case BIG_ROCKETS:
			tmpElements.AddItem(BasicLabel("Homing: " + String(weapon->homingFactor, 2)));
			tmpElements.AddItem(BasicLabel("Explosion radius: " + String(weapon->explosionRadius, 1)));
			break;
		case LIGHTNING:
			tmpElements.AddItem(BasicLabel("Max range: " + String(weapon->maxRange)));
			tmpElements.AddItem(BasicLabel("Max bounces: " + String(weapon->maxBounces)));
			break;
		case LASER_BEAM:
		case LASER_BURST:
			break;
		case HEAT_WAVE:
			tmpElements.AddItem(BasicLabel("Max range: " + String(weapon->maxRange)));
			break;
		case ION_FLAK:
			tmpElements.AddItem(BasicLabel("Num Projectiles/Salvo: " + String(weapon->numberOfProjectiles)));
			tmpElements.AddItem(BasicLabel("Stability: " + String(weapon->stability, 2)));
			break;*/
		};
	}
	if (gearDesc != nullptr)
		QueueGraphics(new GMAddUI(gearDesc, "SelectedStats"));
}

void InHangar::UpdateHoverUpgrade(String upgrade, bool force)
{
	if (hoverUpgrade == upgrade && !force)
		return; // Already set.
	hoverUpgrade = upgrade;
	UpdateGearDetails(hoverUpgrade);
	//	OnHoverActiveUpdated(true, false);
}

void InHangar::UpdateActiveUpgrade(String upgrade)
{
}

void InHangar::UpdateGearList()
{
	GraphicsMan.QueueMessage(new GMSetUIs("wsMunny", GMUI::TEXT, "Money: " + String(Money())));

	String gearListUI = "GearList";
	GraphicsMan.QueueMessage(new GMClearUI(gearListUI));

	List<Gear> gearList = Gear::GetType(gearCategory);
	// Sort by price.
	for (int i = 0; i < gearList.Size(); ++i)
	{
		Gear & gear = gearList[i];
		for (int j = i + 1; j < gearList.Size(); ++j)
		{
			Gear & gear2 = gearList[j];
			if (gear2.price < gear.price)
			{
				Gear tmp = gear;
				gear = gear2;
				gear2 = tmp;
			}
		}
	}

	List<UIElement*> toAdd;
	for (int i = 0; i < gearList.Size(); ++i)
	{
		Gear & gear = gearList[i];
		if (gear.name.Length() == 0)
			continue;
		UIColumnList * list = new UIColumnList();
		if (gear.price < Money())
		{
			list->hoverable = true;
			list->activateable = true;
			list->textureSource = "0x3344";
		}
		else
		{
			list->textureSource = "0x1544";
		}
		list->sizeRatioY = 0.2f;
		list->padding = 0.02f;
		list->onHover = "ShowGearDesc:" + gear.description;
		list->activationMessage = "BuyGear:" + gear.name;
		// First a label with the name.
		UILabel * label = new UILabel(gear.name);
		label->sizeRatioX = 0.3f;
		label->hoverable = false;
		list->AddChild(nullptr, label);
		assert(false);
		// Add stats?
		switch(gearCategory)
		{
			// Weapons:
		
			{
				break;
			}
			// Shields
			case Gear::Type::SHIELD_GENERATOR:
			{
				label = new UILabel("Max Shield: "+String(gear.maxShield));
				label->hoverable = false;
				label->sizeRatioX = 0.2f;
				list->AddChild(nullptr, label);
				label = new UILabel("Regen: "+String(gear.shieldRegen));
				label->hoverable = false;
				label->sizeRatioX = 0.1f;
				list->AddChild(nullptr, label);
				break;
			}
			// Armors
			case Gear::Type::ARMOR:
			{
				label = new UILabel("Max HP: "+String(gear.maxHP));
				label->hoverable = false;
				label->sizeRatioX = 0.15f;
				list->AddChild(nullptr, label);
				label = new UILabel("Toughness: "+String(gear.toughness));
				label->hoverable = false;
				label->sizeRatioX = 0.1f;
				list->AddChild(nullptr, label);
				label = new UILabel("Reactivity: "+String(gear.reactivity));
				label->hoverable = false;
				label->sizeRatioX = 0.1f;
				list->AddChild(nullptr, label);
				break;		
			}
		}
		// Add price.
		label = new UILabel(String(gear.price));
		label->hoverable = false;
		label->sizeRatioX = 0.2f;
		list->AddChild(nullptr, label);

		// Add buy button
		toAdd.Add(list);
	}
	GraphicsMan.QueueMessage(new GMAddUI(toAdd, gearListUI));
}

void InHangar::UpdateUpgradesMoney()
{
	QueueGraphics(new GMSetUIi("WorkshopMoney", GMUI::INTEGER_INPUT, Money()));
}


void InHangar::BuySellToUpgrade(String upgrade) {
	Gear gear;
	bool ok = Gear::Get(upgrade, gear);
	if (ok) {
		// Buy this one. Equip it also by default.
		Gear::SetEquipped(gear);
		Gear::SetOwned(gear);
	}
	Weapon weapon;
	bool isWeapon = Weapon::Get(upgrade, &weapon);
	if (isWeapon) {
		// Sell it?
		if (Weapon::PlayerOwns(weapon)) {
			Money() += weapon.cost;
			Weapon::SetOwnedQuantity(weapon, 0);
		}
		else {
			Money() -= weapon.cost;
			Weapon::SetOwnedQuantity(weapon, 1);
		}
		UpdateUpgradesLists();
	}

}
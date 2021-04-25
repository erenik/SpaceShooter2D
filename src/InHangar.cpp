// Emil Hedemalm
// 2020-08-05
// In the hanger, interacting with NPCs, buying gear, observing the ship in 3D view...? :D 

#include "InHangar.h"

#include "Graphics/Messages/GMUI.h"
#include "Input/InputManager.h"
#include "Input/Gamepad/GamepadMessage.h"
#include "UI/Input/UIStringInput.h"
#include "UI/Input/UIIntegerInput.h"
#include "UI/Input/UIFloatInput.h"
#include "Text/TextManager.h"
#include "Missions.h"
#include "File/LogFile.h"
#include "Base/Gear.h"
#include "Base/Ship.h"
#include "Base/PlayerShip.h"
#include "Message/MathMessage.h"
#include "File/SaveFile.h"
#include "Missions.h"

String hangarGui = "gui/Hangar.gui";
String missionsGui = "gui/Missions.gui";

InHangar::~InHangar() {
}

/// Function when entering this state, providing a pointer to the previous StateMan.
void InHangar::OnEnter(AppState * previousState) {

	LoadAutoSave();

	relevantGear = GetRelevantGear();

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
		String missionName = FirstTimeCompletion() + RepeatCompletion();
		int reward = MissionsMan.GetMissionByName(missionName)->bounty;
		messageQueue.Add(HangarMessage(
			TextMan.GetText("MissionCompleted")
				.Replaced("$missionName", missionName),
			0));
		Text text;
		if (FirstTimeCompletion().Length()) {
			text = TextMan.GetText("MissionClearedFirstTime");
		}
		else if (RepeatCompletion()) {
			text = TextMan.GetText("MissionClearedRepeat");
			reward = (int)(reward * MissionRepeatClearBountyMultiplier());
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
			List<String> tokens = msg.Tokenize(":");
			if (tokens.Size() < 2)
				break;
			String upgrade = tokens[1];
			UpdateGearDetails(upgrade);
		}
		else if (msg.StartsWith("SetHoverMission:")) {
			String name = msg.Tokenize(":")[1];
			Mission * mission = MissionsMan.GetMissionByName(name);
			if (mission == nullptr)
				break;
			QueueGraphics(new GMSetUIs("MissionDetailsName", GMUI::STRING_INPUT_TEXT, TextMan.GetText(mission->name)));
			// int bounty = RepeatCompletion()
			int highscore = GetHighscore(mission->name);
			QueueGraphics(new GMSetUIs("MissionDetailsBounty", GMUI::INTEGER_INPUT_TEXT, highscore > 0? "Repeat bounty" : "Bounty"));
			QueueGraphics(new GMSetUIi("MissionDetailsBounty", GMUI::INTEGER_INPUT, 
				int(highscore > 0 ? 
					mission->bounty * MissionRepeatClearBountyMultiplier() :
					mission->bounty))
			);
			QueueGraphics(new GMSetUIi("MissionDetailsHighscore", GMUI::INTEGER_INPUT, GetHighscore(mission->name)));
		}
		break;
	}
	case MessageType::INTEGER_MESSAGE:
	{
		IntegerMessage * im = (IntegerMessage*)message;
		if (msg == "SetGearCategory")
		{
			gearCategory = im->value == 0 ? Gear::Type::WEAPON : Gear::Type::ARMOR;
			assert(false);
			//UpdateGearList();
		}
	}
	case MessageType::STRING:
		if (msg == "OnReloadUI") {
			PushUI();
			if (hangarMessageDisplayed) {
				DisplayMessage(currentMessage);
			}
		}
		else if (msg.StartsWith("UnequipGearInActiveSlot")) {
			playerShip->UnequipGear(playerShip->Equipped(replaceGearType)[replaceGearIndex]);
			OnGearChanged();
		}
		else if (msg.StartsWith("EquipGear")) {
			String name = msg.Tokenize(":")[1];
			Gear gear = Gear::Get(name);
			if (replaceGear)
				playerShip->Equip(gear, replaceGearIndex);
			else 
				playerShip->Equip(gear);
			OnGearChanged();
		}
		else if (msg.StartsWith("OpenGearListEquipGear")) {
			replaceGear = false;
			List<String> tokens = msg.Tokenize(":,");
			replaceGearType = gearTypeFromString(tokens[1]);
			replaceGearIndex = tokens[2].ParseInt();
			FillSelectGearList("SelectGearList", replaceGearType, Gear());
		}
		else if (msg.StartsWith("OpenGearListChangeGear")) {
			replaceGear = true;
			List<String> tokens = msg.Tokenize(":,");
			String typeStr = tokens[1];
			replaceGearIndex = tokens[2].ParseInt();
			replaceGearType = gearTypeFromString(typeStr);
			Gear currentlyEquipped = playerShip->Equipped(replaceGearType)[replaceGearIndex];
			FillSelectGearList("SelectGearList", replaceGearType, currentlyEquipped);
		}
		else if (msg == "AllCategory") {
			SetGearCategory(Gear::Type::All);
		}
		else if (msg == "WeaponsCategory") {
			SetGearCategory(Gear::Type::Weapon);
		}
		else if (msg == "ArmorCategory") {
			SetGearCategory(Gear::Type::Armor);
		}
		else if (msg == "ShieldCategory") {
			SetGearCategory(Gear::Type::Shield);
		}
		else if (msg.StartsWith("ShowGearDesc:"))
		{
			String text = msg;
			text.Remove("ShowGearDesc:");
			GraphicsMan.QueueMessage(new GMSetUIs("GearInfo", GMUI::TEXT, text));
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
			GameVars.SetInt("PlayTutorial", 0);
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
		break;
	case MessageType::ON_UI_PUSHED:
		if (msg == "MissionsScreen") {
			// Fill up the missions.
			PopulateMissionsList();
		}
		else if (msg == "WorkshopScreen") {
			//UpdateGearList();
			editingShip = false;
			OnGearCategoryChanged(); // Will update upgrades list
			//UpdateUpgradesLists();
			//UpdateWeaponScriptUI();
			break;
		}
		else if (msg == "EditShipScreen") {
			editingShip = true;
			UpdateEditShipScreen();
		}
		break;
	case MessageType::GAMEPAD_MESSAGE: {
		// No special logic
		GamepadMessage * gm = (GamepadMessage*)message;
		// Bumper switches
		if (gm->leftButtonPressed) {
			int index = (((int)gearCategory - 1) % (int)(Gear::Type::AllCategories));
			if (index < 0)
				index = int(Gear::Type::AllCategories) - 1;
			SetGearCategory((Gear::Type) index);
		}
		else if (gm->rightButtonPressed) {
			int index = int(gearCategory) + 1;
			if (index >= int(Gear::Type::AllCategories))
				index = int(Gear::Type::All);
			SetGearCategory((Gear::Type) index);
		}
		break;
	}
	default:
		std::cout << "Received unhandled message: "+ String(message->type);
	}
	// Process general messages.
	SpaceShooter2D::ProcessMessage(message);
}

String GetButtonNameForCategory(Gear::Type category) {
	switch (category) {
	case Gear::Type::All:
		return "AllCategory";
	case Gear::Type::Weapon:
		return "WeaponsCategory";
	case Gear::Type::Armor:
		return "ArmorCategory";
	case Gear::Type::Shield:
		return "ShieldCategory";
	}

}

void InHangar::SetGearCategory(Gear::Type category) {
	gearCategory = category;
	OnGearCategoryChanged();
}

void InHangar::OnGearCategoryChanged() {
	for (int i = 0; i < int(Gear::Type::AllCategories); ++i) {
		if (i == int(gearCategory))
			QueueGraphics(new GMSetUIv4f(GetButtonNameForCategory(Gear::Type(i)), GMUI::COLOR, (UIElement::onToggledHighlightFactor + 1.0f) * Vector4f(1, 1, 1, 1)));
		else
			QueueGraphics(new GMSetUIv4f(GetButtonNameForCategory(Gear::Type(i)), GMUI::COLOR, 1.0f * Vector4f(1, 1, 1, 1)));
	}
	UpdateUpgradesLists();
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
		UIElement * missionButton = UserInterface::LoadUIAsElement("gui/SelectMissionButton.gui");
		missionButton->GetElementByName("Icon")->textureSource = "0x334455";
		missionButton->GetElementByName("Completed")->SetText(mission->Completed()? "Completed" : "");
		UIElement * missionName = missionButton->GetElementByName("MissionName");
		missionName->SetText(TextMan.GetText(mission->name));
		missionName->SetOnHoverTextColor(ssActiveTextColor);
//		(missionButton->GetElementByName("Bounty"))->SetText(String(mission->bounty));
		(missionButton->GetElementByName("SelectMissionButton"))->activationMessage = "PlayMission "+ mission->name;
		if (i == 0) // Hover to first one by default?
			(missionButton->GetElementByName("SelectMissionButton"))->SetState(UIState::HOVER);

		missionButton->onHover = "SetHoverMission:" + mission->name;
		entries.Add(missionButton);
	}
	QueueGraphics(new GMAddUI(entries, "MissionsList"));
}

void InHangar::SetUpScene(){
	MapMan.DeleteAllEntities();
}

List<Gear> InHangar::GetRelevantGear() {
	List<Gear> relevantGear;
	relevantGear.Add(Gear::GetAllOfType(Gear::Type::Weapon));
	relevantGear.Add(Gear::GetAllOfType(Gear::Type::Armor));
	relevantGear.Add(Gear::GetAllOfType(Gear::Type::Shield));

	// Pre-filter a bit.
	for (int i = 0; i < relevantGear.Size(); ++i) {
		Gear& gear = relevantGear[i];
		bool skip = false;
		if (gear.price > Money() * 2 + 1000) {
			skip = true;
		}
		if (Gear::Owns(gear)) {
			skip = false;
		}
		if (skip) {
			relevantGear.RemoveIndex(i, ListOption::RETAIN_ORDER);
			--i;
		}
	}
	return relevantGear;
}


#include "UI/Lists/UIColumnList.h"
#include "UI/UIButtons.h"

struct StoreButtonData {
	bool isWeapon = false;
	Weapon weapon;
	Gear gear;
	String buttonName;
	String isOwnedLabelName;
	String productNameLabel;
	String productPriceLabel;
};

List<StoreButtonData> storeButtonData;

void InHangar::UpdateUpgradesLists()
{
	storeButtonData.Clear();
	String productsList = "ProductsList";
	QueueGraphics(new GMClearUI(productsList));
	/// Fill with column lists for each weapon.
	List<UIElement*> shopProductEntries;
	UIElement * buyProductButtonTemplate = UserInterface::LoadUIAsElement("gui/BuyProductButton.gui");
	List<Gear> gearToDisplay = relevantGear;
	for (int i = 0; i < gearToDisplay.Size(); ++i)
	{
		Gear gear = gearToDisplay[i];
		// Skip irrelevant gear depending on current filter.
		if (gearCategory != gear.type && gearCategory != Gear::Type::All)
			continue;
		TextColors* textColors = UIElement::defaultTextColors;
		UIElement * buyProductButton = buyProductButtonTemplate->Copy();
		buyProductButton->name += gear.name;

		buyProductButton->activationMessage = "ActiveUpgrade:" + gear.name;
		if (Gear::Owns(gear)) { // Owned
		}
		else { // Not owned
			if (gear.price > Money()) {
				buyProductButton->activationMessage = "PlayErrorSFX TODO";
			}
			else {
			}
		}
		UIElement * productName = buyProductButton->GetElementByName("ProductName");
		productName->SetText(Text(gear.name));
		productName->SetOnHoverTextColor(ssActiveTextColor);

		auto productLabelName = buyProductButton->GetElementByName("ProductName")->name += gear.name;
		buyProductButton->GetElementByName("Price")->SetText(Text(gear.price));
		auto priceLabelName = buyProductButton->GetElementByName("Price")->name += gear.name;
		auto isOwnedLabel = buyProductButton->GetElementByName("Owned");
		isOwnedLabel->name += gear.name;
		
		buyProductButton->GetElementByName("Icon")->textureSource = gear.Icon();

		buyProductButton->onHover = "SetHoverUpgrade:" + gear.name;
		shopProductEntries.Add(buyProductButton);

		StoreButtonData data;
		data.gear = gear;
		data.buttonName = buyProductButton->name;
		data.isOwnedLabelName = isOwnedLabel->name;
		data.productNameLabel = productLabelName;
		data.productPriceLabel = priceLabelName;

		storeButtonData.Add(data);

	}
	QueueGraphics(new GMAddUI(shopProductEntries, productsList));

	UpdateUpgradeStatesInList();

	// Hover to the first element by default
	assert(shopProductEntries.Size() > 0);
	QueueGraphics(new GMSetHoverUI(shopProductEntries[0]->name));

	GraphicsMan.QueueMessage(new GMSetUIi("WorkshopMoney", GMUI::INTEGER_INPUT, Money()));
}

void InHangar::FillSelectGearList(String list, Gear::Type type, Gear currentlyEquippedGearInSlot) {
	QueueGraphics(new GMClearUI(list));
	/// Fill with column lists for each weapon.
	List<UIElement*> selectGearEntries;
	UIElement * adjustEquippedItemButtonTemplate = UserInterface::LoadUIAsElement("gui/AdjustEquippedItemButton.gui");
	List<Gear> gearToDisplayInList = Gear::GetAllOwnedOfType(type);

	// Filter out gear currently equipped
	List<Gear> currentlyEquipped = playerShip->Equipped(Gear::Type::All);
	for (int i = 0; i < currentlyEquipped.Size(); ++i) {
		Gear equipped = currentlyEquipped[i];
		gearToDisplayInList.Remove(equipped);
	}

	// Generate buttons
	for (int i = -1; i < gearToDisplayInList.Size(); ++i)
	{
		UIElement * selectGearButton = adjustEquippedItemButtonTemplate->Copy();

		UIElement* gearNameLabel = selectGearButton->GetElementByName("GearName");

		if (i == -1) {
			// Skip if no actual gear to remove there.
			if (currentlyEquippedGearInSlot.name.Length() == 0)
				continue;
			gearNameLabel->SetText(Text(TextMan.GetText("Unequip") + " " + TextMan.GetText(currentlyEquippedGearInSlot.name)));
			selectGearButton->onHover = "SetHoverUpgrade:" + currentlyEquippedGearInSlot.name;
			selectGearButton->activationMessage = "UnequipGearInActiveSlot";
			selectGearButton->textureSource = "0x661122";
			selectGearButton->GetElementByName("GearIcon")->textureSource = currentlyEquippedGearInSlot.Icon();
		}
		else {
			Gear gear = gearToDisplayInList[i];
			selectGearButton->name += gear.name;
			gearNameLabel->SetText(Text(TextMan.GetText(gear.name)));
			selectGearButton->onHover = "SetHoverUpgrade:" + gear.name;
			selectGearButton->activationMessage = "EquipGear:" + gear.name;
			selectGearButton->GetElementByName("GearIcon")->textureSource = gear.Icon();
		}

		gearNameLabel->SetOnHoverTextColor(ssActiveTextColor);

		selectGearEntries.Add(selectGearButton);
	}

	if (selectGearEntries.Size() == 0) {
		// Display error message at the bottom?
		QueueGraphics(new GMSetUIs("Notice", GMUI::TEXT, TextMan.GetText("NoAvailableGear")));
		return;
	}

	QueueGraphics(new GMSetUIs("Notice", GMUI::TEXT, ""));

	QueueGraphics(new GMAddUI(selectGearEntries, list));

	// Hover to the first element by default
	assert(selectGearEntries.Size() > 0);
	QueueGraphics(new GMSetHoverUI(selectGearEntries[0]->name));

	QueueGraphics(GMPushUI::ToUI("SelectGearList"));
}

void InHangar::OnGearChanged() {
	UpdateEditShipScreen();
	QueueGraphics(new GMPopUI("SelectGearList", true));
	SaveGame();
}

void InHangar::UpdateShipStats() {
	QueueGraphics(new GMClearUI("lEditScreenShipStats"));
	UIElement* gearDesc = nullptr;
	gearDesc = UserInterface::LoadUIAsElement("gui/ShipStatsDescription.gui");
	
	//((UIStringInput*)gearDesc->GetElementByName("ShipName"))->SetValue("Vertigo");
	//	((UIIntegerInput*)gearDesc->GetElementByName("ShipPrice"))->SetValue(0);

	((UIFloatInput*)gearDesc->GetElementByName("ReloadTime"))->SetValue(playerShip->reloadMultiplier);
	((UIFloatInput*)gearDesc->GetElementByName("RateOfFire"))->SetValue(playerShip->rateOfFireMultiplier);
	
	((UIIntegerInput*)gearDesc->GetElementByName("ShipSpeed"))->SetValue((int)playerShip->Speed());

	((UIIntegerInput*)gearDesc->GetElementByName("MaxArmor"))->SetValue((int)playerShip->maxHP);
	((UIFloatInput*)gearDesc->GetElementByName("ArmorRegen"))->SetValue(playerShip->armorRegenRate);

	((UIIntegerInput*)gearDesc->GetElementByName("MaxShield"))->SetValue((int)playerShip->MaxShield());
	((UIFloatInput*)gearDesc->GetElementByName("ShieldRegen"))->SetValue(playerShip->shieldRegenRate);

	QueueGraphics(new GMAddUI(gearDesc, "lEditScreenShipStats", nullptr, UIFilter::Visible));
}

void InHangar::FillEditScreenEntries(String inList, Gear::Type type) {
	QueueGraphics(new GMClearUI(inList));
	/// Fill with column lists for each weapon.
	List<UIElement*> equippedGearEntries;
	UIElement * adjustEquippedItemButtonTemplate = UserInterface::LoadUIAsElement("gui/AdjustEquippedItemButton.gui");

	playerShip->UpdateGearFromVars();

	List<Gear> gearToDisplayInList = playerShip->Equipped(type);
	for (int i = 0; i < gearToDisplayInList.Size() + 1; ++i)
	{
		UIElement * adjustEquippedItemButton = adjustEquippedItemButtonTemplate->Copy();
		UIElement* adjustOrEquipNameLabel = adjustEquippedItemButton->GetElementByName("GearName");

		// For extra buttons to equip more.
		if (i >= gearToDisplayInList.Size()) {
			if (i >= playerShip->MaxGearForType(type))
				continue;
			adjustEquippedItemButton->name += toString(type);
			adjustEquippedItemButton->activationMessage = "OpenGearListEquipGear:" + toString(type) + ","+String(i);
			// TODO: Set text color of all.
			adjustEquippedItemButton->GetElementByName("GearSlot")->SetText(TextMan.GetText("GearSlot") + " " + String(i + 1));
			adjustOrEquipNameLabel->SetText(TextMan.GetText("AddEquipment"+ toString(type)));
			adjustEquippedItemButton->GetElementByName("GearIcon")->textureSource = Gear::TypeIcon(type);
			adjustEquippedItemButton->GetElementByName("GearIcon")->color = Vector4f(1,1,1,1) * 0.8f;
		}
		// Show entries 
		else {
			Gear gear = gearToDisplayInList[i];
			adjustEquippedItemButton->name += gear.name;
			adjustEquippedItemButton->activationMessage = "OpenGearListChangeGear:" + toString(type)+","+String(i);
			adjustEquippedItemButton->GetElementByName("GearSlot")->SetText(TextMan.GetText("GearSlot") + " " + String(i + 1));
			adjustOrEquipNameLabel->SetText(TextMan.GetText(gear.name));
			adjustEquippedItemButton->onHover = "SetHoverUpgrade:" + gear.name;
			adjustEquippedItemButton->GetElementByName("GearIcon")->textureSource = gear.Icon();
		}

		adjustOrEquipNameLabel->SetOnHoverTextColor(ssActiveTextColor);

		equippedGearEntries.Add(adjustEquippedItemButton);
	}
	QueueGraphics(new GMAddUI(equippedGearEntries, inList));

	// Re-navigate to nearby button if we just changed gear
	String hoverName;
	if (replaceGearIndex != -1) {
		if (replaceGearType == type) {
			hoverName = equippedGearEntries[replaceGearIndex]->name;
		}
	}
	// otherwise just hover to the first element by default
	else {
		assert(equippedGearEntries.Size() > 0);
		hoverName = equippedGearEntries[0]->name;
	}
	// Hover if relevant.
	if (hoverName.Length() > 0)
		QueueGraphics(new GMSetHoverUI(hoverName));
}

void InHangar::UpdateEditShipScreen() {

	FillEditScreenEntries("lWeapons", Gear::Type::Weapon);
	FillEditScreenEntries("lArmors", Gear::Type::Armor);
	FillEditScreenEntries("lShield", Gear::Type::Shield);
	UpdateShipStats();
}

void InHangar::UpdateUpgradeStatesInList()
{

	for (int i = 0; i < storeButtonData.Size(); ++i) {
		StoreButtonData data = storeButtonData[i];
		bool isOwned = false;
		int price = 0;
		String productName;
		if (data.isWeapon) {
			// Special treatment.
			isOwned = Weapon::PlayerOwns(data.weapon);
			price = data.weapon.cost;
			productName = data.weapon.name;
		}
		else {
			isOwned = Gear::Owns(data.gear);
			price = data.gear.price;
			productName = data.gear.name;
		}

		Vector4f textColor(1,1,1,1);
		String activationMessage = "ActiveUpgrade:" + productName;
		if (isOwned) {
			// textColor = Vector4f(0.9f, 0.9f, 0.9f, 1);
		}
		else {
			if (price > Money()) {
				textColor = Vector4f(1, 1, 1, 1) * 0.5f;
				activationMessage = "PlayErrorSFX TODO";
				// buyProductButton->activationMessage = "PlayErrorSFX TODO";
			}
		}

		QueueGraphics(new GMSetUIs(data.buttonName, GMUI::ACTIVATION_MESSAGE, activationMessage));
		QueueGraphics(new GMSetUIt(data.isOwnedLabelName, GMUI::TEXT, isOwned ? "Owned" : ""));
		QueueGraphics(new GMSetUIb(data.isOwnedLabelName, GMUI::VISIBILITY, isOwned));
		QueueGraphics(new GMSetUIv4f(data.productNameLabel, GMUI::TEXT_COLOR, textColor));
		QueueGraphics(new GMSetUIv4f(data.productPriceLabel, GMUI::TEXT_COLOR, textColor));
		QueueGraphics(new GMSetUIs(data.buttonName, GMUI::TEXTURE_SOURCE, isOwned ? "0x334466AA" : price > Money() ? "0x34AA" : "0x44AA"));
	}

	GraphicsMan.QueueMessage(new GMSetUIi("WorkshopMoney", GMUI::INTEGER_INPUT, Money()));

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
	String selectedStatsUIName = editingShip ? "EditShipSelectedGearStats" : "SelectedStats";
	QueueGraphics(new GMClearUI(selectedStatsUIName));
	Gear gear;
	bool isGear = Gear::Get(upgrade, gear);
	bool isArmor = false,
		isShield = false;
	if (isGear) {
		isArmor = gear.type == Gear::Type::Armor;
		isShield = gear.type == Gear::Type::Shield;
	}

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
	else if (isArmor) {
		gearDesc = UserInterface::LoadUIAsElement("gui/GearArmorDescription.gui");
		((UIStringInput*)gearDesc->GetElementByName("GearName"))->SetValue(TextMan.GetText(upgrade));
		((UIIntegerInput*)gearDesc->GetElementByName("GearPrice"))->SetValue(gear.price);
		((UIIntegerInput*)gearDesc->GetElementByName("GearHp"))->SetValue((int)gear.maxHP);
		((UIIntegerInput*)gearDesc->GetElementByName("GearRegeneration"))->SetValue((int)gear.armorRegen);
		((UIIntegerInput*)gearDesc->GetElementByName("GearToughness"))->SetValue(gear.Toughness());
		((UIIntegerInput*)gearDesc->GetElementByName("GearReactivity"))->SetValue(gear.Reactivity());
		gearDesc->GetElementByName("GearTextDescription")->SetText(TextMan.GetText(gear.name + "Desc"));
	}
	else if (isShield) {
		gearDesc = UserInterface::LoadUIAsElement("gui/GearShieldDescription.gui");
		((UIStringInput*)gearDesc->GetElementByName("GearName"))->SetValue(TextMan.GetText(upgrade));
		((UIIntegerInput*)gearDesc->GetElementByName("GearPrice"))->SetValue(gear.price);
		((UIIntegerInput*)gearDesc->GetElementByName("GearHp"))->SetValue((int)gear.maxShield);
		((UIIntegerInput*)gearDesc->GetElementByName("GearRegeneration"))->SetValue((int)gear.shieldRegen);
		gearDesc->GetElementByName("GearTextDescription")->SetText(TextMan.GetText(gear.name + "Desc"));
	}

	if (gearDesc != nullptr)
		QueueGraphics(new GMAddUI(gearDesc, selectedStatsUIName, nullptr, UIFilter::Visible));
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

void InHangar::UpdateUpgradesMoney()
{
	QueueGraphics(new GMSetUIi("WorkshopMoney", GMUI::INTEGER_INPUT, Money()));
}

void InHangar::BuySellToUpgrade(String upgrade) {
	Gear gear;
	bool ok = Gear::Get(upgrade, gear);
	if (ok) {
		if (Gear::Owns(gear)) {
			// Sell it!
			Money() += gear.price;
			Gear::SetOwned(gear, 0);
		}
		else {
			// Buy this one. Equip it also by default.
			Gear::SetOwned(gear);
			Money() -= gear.price;
		}
	}
	//UpdateUpgradesLists();
	UpdateUpgradeStatesInList();
}
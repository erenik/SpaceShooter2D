/// Emil Hedemalm
/// 2015-02-07
/// All code pertaining to updating UI... to separate it from the rest.

#include "SpaceShooter2D.h"
#include "File/SaveFile.h"
#include "Application/Application.h"

#include "Window/AppWindow.h"

#undef TEXT

#include "UI/UILists.h"
#include "UI/UIUtil.h"
#include "UI/UIInputs.h"
#include "UI/UIButtons.h"
#include "Text/TextManager.h"
#include "PlayingLevel.h"

#include "Window/AppWindowManager.h"

void LoadOptions();

void UpdateWeaponScriptUI();

/// Updates ui depending on mode.
void SpaceShooter2D::UpdateUI()
{
	/// Pop stuff.
//	List<String> uis("gui/MainMenu.gui", "gui/HUD.gui", "gui/Hangar.gui", "gui/Workshop.gui");
//	uis.AddItem("gui/WeaponScripts.gui");
	//for (int i = 0; i < uis.Size(); ++i)
	//{
	//	PopUI(uis[i]);
	//}
	String toPush;	
	// Reveal specifics?
	switch(mode)
	{
		case START_UP: break; // No UI, or splash screen?
		case MAIN_MENU: toPush = "gui/MainMenu.gui"; break;
		case EDITING_OPTIONS: toPush = "gui/Options.gui"; break;
		case NEW_GAME: toPush = "gui/NewGame.gui"; break;
		case LOAD_SAVES: toPush = "gui/LoadScreen.gui"; break;
		case GAME_OVER: 
		case PLAYING_LEVEL:	toPush = "gui/HUD.gui"; break;
		case LEVEL_CLEARED: toPush = "gui/LevelStats.gui"; break;
		case IN_LOBBY: toPush = "gui/Lobby.gui"; break;
		case IN_HANGAR: toPush = "gui/Hangar.gui"; break;
		case IN_WORKSHOP: toPush = "gui/Workshop.gui"; break;
		case EDIT_WEAPON_SWITCH_SCRIPTS: toPush = "gui/WeaponScripts.gui"; break;
		case BUYING_GEAR: toPush = "gui/Shop.gui"; break;
		default:
			assert(false);
//			uis.Add("MainMenu");
			break;
	}
	if (toPush.Length())
		MesMan.ProcessMessage("PushUI("+toPush+")");
	
	switch(mode)
	{
		case NEW_GAME:  LoadDefaultName(); break;
		case EDITING_OPTIONS: LoadOptions(); break;
		case LOAD_SAVES: 
			OpenLoadScreen(); 
			break;
		case BUYING_GEAR: 
			UpdateGearList(); 
			break;
		case IN_WORKSHOP:
			UpdateUpgradesLists();
			break;
		case EDIT_WEAPON_SWITCH_SCRIPTS:
			UpdateWeaponScriptUI();
			break;
	};
}

void LoadOptions()
{
	// Load settings.
}


// Update ui
void SpaceShooter2D::OnScoreUpdated()
{
	GraphicsMan.QueueMessage(new GMSetUIi("Scorei", GMUI::INTEGER_INPUT, LevelScore()->iValue));
}

void SpaceShooter2D::UpdateHUDSkill()
{
	PlayingLevel& pl = PlayingLevelRef();
	QueueGraphics(new GMSetUIs("Skill", GMUI::TEXT, pl.playerShip->skillName));
	QueueGraphics(new GMSetUIs("Skill", GMUI::TEXTURE_SOURCE, pl.playerShip->activeSkill != NO_SKILL? "0x00FF00FF" : "0x44AA"));
}

void SpaceShooter2D::LoadDefaultName()
{
	GraphicsMan.QueueMessage(new GMSetUIs("PlayerName", GMUI::STRING_INPUT_TEXT, playerName->strValue));
	GraphicsMan.QueueMessage(new GMSetUIi("Difficulty", GMUI::INTEGER_INPUT, difficulty->iValue));
}

void SpaceShooter2D::UpdateUpgradesLists()
{
	PlayingLevel& pl = PlayingLevelRef();
	QueueGraphics(new GMClearUI("lWeaponCategories"));
	/// Fill with column lists for each weapon.
	List<UIElement*> cls;
	for (int i = 0; i < WeaponType::MAX_TYPES; ++i)
	{
		UIColumnList * cl = new UIColumnList("Weapon"+String(i)+"UpgradeCL");
		cl->sizeRatioY = 0.1f;
		cl->padding = 0.01f;
		cls.AddItem(cl);
		UILabel * label = new UILabel(TextMan.GetText(i+10));
		label->sizeRatioX = 0.4f;
		cl->AddChild(nullptr, label);

		for (int j = 0; j < 10; ++j)
		{
			Weapon * weapon = Weapon::Get(i, j);
			if (!weapon && j != 0)
				continue;

			UIButton * bn = new UIButton(WeaponTypeLevelToString(i,j));
			bn->text = "";
			bn->hoverable = true;
			bn->onHover = "SetHoverUpgrade:"+bn->name;
			bn->activationMessage = "ActiveUpgrade:"+bn->name;
			bn->sizeRatioY = 0.6f;
			if (pl.playerShip->weapons[i]->level > j)
				bn->textureSource = "0x00FF00AA";
			else
				bn->textureSource = "0x44AA";
			bn->sizeRatioX = 0.05f;
			cl->AddChild(nullptr, bn);
		}
	}
	QueueGraphics(new GMAddUI(cls, "lWeaponCategories"));
	UpdateUpgradeStatesInList();
}

void SpaceShooter2D::UpdateUpgradesMoney()
{
	QueueGraphics(new GMSetUIi("WorkshopMoney", GMUI::INTEGER_INPUT, money->GetInt()));
}

void SpaceShooter2D::UpdateUpgradeStatesInList()
{

	PlayingLevel& pl = PlayingLevelRef();
	// Does not create, merely modifies.
	for (int i = 0; i < WeaponType::MAX_TYPES; ++i)
	{
		for (int j = 0; j < 10; ++j)
		{
			String buttonName = WeaponTypeLevelToString(i,j);
			String textureSource;
			Vector4f color(1,1,1,1);
			textureSource = "ui/SpaceShooterUpgrade_White";//textureSource = "0xFFFF00AA";
			if (j == 0)
			{
				if (pl.playerShip->weapons[i]->level > 0)
				{
	//				textureSource = "ui/SpaceShooterUpgrade_White";//textureSource = "0xFFFF00AA";
					color = Vector4f(1,1,0,1);
				}
				else 
				{
					color = Vector4f(1,0,0,1);
//					textureSource = "0xFF0000AA";
//				textureSource = "0x44AA";
				}
			}
			else 
			{
				if (pl.playerShip->weapons[i]->level >= j)
//					color = Vector4f(0,1,0,1);
					color = Color(102, 255, 0, 255);
				//	textureSource = "ui/SpaceShooterGreenUpgrade.png";
					//textureSource = "0x00FF00AA";
				else
					color = Vector4f(1,1,1,1) * 0.25f;
//					textureSource = "0x44AA";
			}
			QueueGraphics(new GMSetUIv4f(buttonName, GMUI::COLOR, color));
			QueueGraphics(new GMSetUIs(buttonName, GMUI::TEXTURE_SOURCE, textureSource));
		}
	}
}



List<UIElement*> tmpElements;

UILabel * BasicLabel(String text)
{
	UILabel * l = new UILabel(text);
	l->sizeRatioY = 0.2f;
	return l;
}

UILabel * BasicLabel(int textID)
{
	return BasicLabel(TextMan.GetText(textID));
}

void FillBasicInfo(String upgrade, String inElement)
{

	PlayingLevel& pl = PlayingLevelRef();
	auto playerShip = pl.playerShip;

	QueueGraphics(new GMClearUI(inElement));
	// Show basic info
	Vector2i tl = WeaponTypeLevelFromString(upgrade);
	int type = tl.x, level = tl.y;
	// Fetch type.
	Weapon * weapon = Weapon::Get(type, level);

	List<UIElement*> elements;
	UIList * l1 = new UIList(), * l2 = new UIList();
	elements.Add(l1, l2);
	l1->sizeRatioX = l2->sizeRatioX = 0.5f;
	l2->alignmentX = 0.75f;
	l1->alignmentX = 0.25f;
	/// o.o lvl 0?
	if (!weapon)
	{
		if (playerShip->weapons[type]->level > 0)
			tmpElements.AddItem(BasicLabel("Unequip?"));
		else
			tmpElements.AddItem(BasicLabel("Level 0."));
		l1->AddChildren(nullptr, tmpElements);
		tmpElements.Clear();
	}
	else // Weapon exists.
	{
		tmpElements.AddItem(BasicLabel("Name: "+weapon->name));
		tmpElements.AddItem(BasicLabel("Price: "+String(weapon->cost)));
		tmpElements.AddItem(BasicLabel("Cooldown: "+String(weapon->cooldown.Milliseconds())));
		tmpElements.AddItem(BasicLabel("Damage: "+String(weapon->damage, 1)));
		l1->AddChildren(nullptr, tmpElements);
		tmpElements.Clear();
		// Depending on type, add extra statistics too that might be interesting? ^^
		switch(type)
		{
			case BULLETS:
				tmpElements.AddItem(BasicLabel("Penetration: "+String(weapon->penetration,2)));
				tmpElements.AddItem(BasicLabel("Stability: "+String(weapon->stability,2)));
				break;
			case SMALL_ROCKETS:
				tmpElements.AddItem(BasicLabel("Burst: "+String(weapon->burstRounds)+"/"+String(weapon->burstRoundDelay.Milliseconds())));
				tmpElements.AddItem(BasicLabel("Homing: "+String(weapon->homingFactor, 2)));
				tmpElements.AddItem(BasicLabel("Explosion radius: "+String(weapon->explosionRadius, 1)));
				break;
			case BIG_ROCKETS:
				tmpElements.AddItem(BasicLabel("Homing: "+String(weapon->homingFactor, 2)));
				tmpElements.AddItem(BasicLabel("Explosion radius: "+String(weapon->explosionRadius, 1)));
				break;	
			case LIGHTNING:
				tmpElements.AddItem(BasicLabel("Max range: "+String(weapon->maxRange)));
				tmpElements.AddItem(BasicLabel("Max bounces: "+String(weapon->maxBounces)));
				break;	
			case LASER_BEAM:
			case LASER_BURST:
				break;
			case HEAT_WAVE:
				tmpElements.AddItem(BasicLabel("Max range: "+String(weapon->maxRange)));
				break;
			case ION_FLAK:
				tmpElements.AddItem(BasicLabel("Num Projectiles/Salvo: "+String(weapon->numberOfProjectiles)));
				tmpElements.AddItem(BasicLabel("Stability: "+String(weapon->stability,2)));
				break;			
		};
		l2->AddChildren(nullptr, tmpElements);
		tmpElements.Clear();
	}
	QueueGraphics(new GMAddUI(elements, inElement));
//	QueueGraphics(new GMSetUIs(inElement, GMUI::TEXT, upgrade));
}

void MoreStats(String upgrade, String inElement)
{
	PlayingLevel& pl = PlayingLevelRef();
	auto playerShip = pl.playerShip;
	QueueGraphics(new GMClearUI(inElement));
	// Show basic info
	Vector2i tl = WeaponTypeLevelFromString(upgrade);
	int type = tl.x, level = tl.y;
	// Fetch type.
//	Weapon * weapon = Weapon::Get(type, level);
	int playerLevel = playerShip->weapons[type]->level;
	// Diff in money upon purchase
	int cost = DiffCost(upgrade);
	String str;
	if (cost != 0)
	{
		str = (cost > 0? "Cost: " : "Sell value: ")+String(AbsoluteValue(cost));
		int lvlDiff = level - playerLevel;
		if (AbsoluteValue(lvlDiff) > 1)
			str += " ("+String(AbsoluteValue(lvlDiff))+" lvls)";
	}
	else if (playerLevel > 0)
		str = "Equipped. Current level.";
	UILabel * label = BasicLabel(str);
	Vector4f color;
	if (cost > 0)
		color = Vector4f(1,1,0,1);
	else if (cost < 0)
		color = Vector4f(0,1,1,1);
	else if (cost == 0)
		color = Vector4f(0,1,0,1);
	label->text.color = color;
	tmpElements.AddItem(label);
	QueueGraphics(new GMAddUI(tmpElements, inElement));
	tmpElements.Clear();
}


String hoverUpgrade;

void SpaceShooter2D::UpdateHoverUpgrade(String upgrade, bool force)
{
	if (hoverUpgrade == upgrade && !force)
		return; // Already set.
	hoverUpgrade = upgrade;
	FillBasicInfo(hoverUpgrade, "SelectedStats");
	MoreStats(hoverUpgrade, "MoreStats");
//	OnHoverActiveUpdated(true, false);
}

void SpaceShooter2D::UpdateActiveUpgrade(String upgrade)
{
}

/// Returns a list of save-files.
void SpaceShooter2D::OpenLoadScreen()
{
	mode = LOAD_SAVES;
	List<SaveFileHeader> headers;
	/// Returns list of all saves, in the form of their SaveFileHeader objects, which should include all info necessary to judge which save to load!
	headers = SaveFile::GetSaves(Application::name);
	// Clear old list.
	GraphicsMan.QueueMessage(new GMClearUI("SavesCList"));
	/// Sort saves by date?
	for (int i = 0; i < headers.Size(); ++i)
	{
		SaveFileHeader & header = headers[i];
		for (int j = i + 1; j < headers.Size(); ++j)
		{
			SaveFileHeader & header2 = headers[j];
			if (header2.dateSaved > header.dateSaved)
			{
				// Switch places.
				SaveFileHeader tmp = header2;
				header2 = header;
				header = tmp;
			}
		}
	}
	// List 'em.
	List<UIElement*> saves;
	for (int i = 0; i < headers.Size(); ++i)
	{
		SaveFileHeader & h = headers[i];
		UIList * list = new UIList();
		
		UILabel * label = new UILabel();
		label->text = h.customHeaderData;
		label->sizeRatioY = 0.5f;
		label->hoverable = false;
		label->textureSource = "NULL";
		list->AddChild(nullptr, label);

		list->highlightOnHover = true;
		list->sizeRatioX = 0.33f;
		list->activateable = true;
		list->highlightOnActive = true;
		list->activationMessage = "LoadGame("+h.saveName+")";
		list->textureSource = "0x3366";
		saves.Add(list);
	}
	GraphicsMan.QueueMessage(new GMAddUI(saves, "SavesCList"));
	/// Move cursor to first save in the list.
	if (saves.Size())
	{
		saves[0]->Hover();
	}
	else 
	{
		GraphicsMan.QueueMessage(new GMSetHoverUI("Back"));
	}
}

void SpaceShooter2D::UpdateGearList()
{
	GraphicsMan.QueueMessage(new GMSetUIs("wsMunny", GMUI::TEXT, "Money: "+String(money->iValue)));

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
		if (gear.price < money->iValue)
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
		list->onHover = "ShowGearDesc:"+gear.description;
		list->activationMessage = "BuyGear:"+gear.name;
		// First a label with the name.
		UILabel * label = new UILabel(gear.name);
		label->sizeRatioX = 0.3f;
		label->hoverable = false;
		list->AddChild(nullptr, label);
		// Add stats?
		switch(gearCategory)
		{
			// Weapons:
			case 0:
			{
				break;
			}
			// Shields
			case 1:
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
			case 2:
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


void SpaceShooter2D::OpenJumpDialog()
{
	// Create a string-input.
	static UIStringInput * jumpDialog = 0;
	if (!jumpDialog)
	{
		jumpDialog = new UIStringInput("JumpTo", "JumpToTime");
		jumpDialog->textureSource = "0x44AA";
		jumpDialog->onTrigger += "PopUI(JumpTo)&ResumeGame";
		jumpDialog->sizeRatioX = 0.5f;
		jumpDialog->sizeRatioY = 0.1f;
		jumpDialog->CreateChildren(nullptr);
		jumpDialog->input->BeginInput(); // Make its input active straight away.
		// Add it to the main UI.
		QueueGraphics(new GMAddUI(jumpDialog, "root"));
	}
	else {
		jumpDialog->input->BeginInput(); // Make its input active straight away.
		QueueGraphics(GMPushUI::ToUI(jumpDialog, MainWindow()->ui));
	}
		// Close it afterwards.
}



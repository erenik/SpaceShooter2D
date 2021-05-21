/// Emil Hedemalm
/// 2015-02-07
/// All code pertaining to updating UI... to separate it from the rest.

#include "SpaceShooter2D.h"
#include "File/SaveFile.h"
#include "Application/Application.h"
#include "Base/PlayerShip.h"
#include "Window/AppWindow.h"

#undef TEXT

#include "UI/UILists.h"
#include "UI/UIUtil.h"
#include "UI/UIInputs.h"
#include "UI/UIButtons.h"
#include "UI/Buttons/UIRadioButtons.h"
#include "Text/TextManager.h"
#include "PlayingLevel.h"

#include "Window/AppWindowManager.h"

void LoadOptions();

void UpdateWeaponScriptUI();

/// Updates ui depending on mode.
void SpaceShooter2D::UpdateUI()
{
	/// Pop stuff.
	String toPush;	
	// Reveal specifics?
	switch(mode)
	{
		case START_UP: break; // No UI, or splash screen?
		case MAIN_MENU: toPush = "gui/MainMenu.gui"; break;
		case EDITING_OPTIONS: toPush = "gui/Options.gui"; break;
		case NEW_GAME: 
		{
			UIElement * element = UserInterface::LoadUIAsElement(nullptr, "gui/NewGame.gui");
			UIStringInput * stringInput = (UIStringInput*) element->GetElement("PlayerName", UIType::STRING_INPUT);
			stringInput->SetValue(PlayerName());

			UIRadioButtons * difficultyButtons = (UIRadioButtons*)element->GetElement("NewGameDifficulty", UIType::RADIO_BUTTONS);
			difficultyButtons->SetValue(nullptr, difficulty->iValue);

			QueueGraphics(GMPushUI::ToUI(element));

			//GraphicsMan.QueueMessage(new GMSetUIs("PlayerName", GMUI::STRING_INPUT_TEXT, ));
			//GraphicsMan.QueueMessage(new GMSetUIi("Difficulty", GMUI::INTEGER_INPUT, difficulty->iValue));

			break;
		}
		case LOAD_SAVES: toPush = "gui/LoadScreen.gui"; break;
		case GAME_OVER: 
		case PLAYING_LEVEL:	toPush = "gui/HUD.gui"; break;
//		case LEVEL_CLEARED: toPush = "gui/LevelStats.gui"; break;
		case IN_LOBBY: toPush = "gui/Lobby.gui"; break;
		case IN_HANGAR: break;
		case LEVEL_EDITOR: break;
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
		case EDITING_OPTIONS: LoadOptions(); break;
		case LOAD_SAVES: 
			OpenLoadScreen(); 
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
	GraphicsMan.QueueMessage(new GMSetUIi("Scorei", GMUI::INTEGER_INPUT, PlayingLevelRef().score));
}

void SpaceShooter2D::UpdateHUDSkill()
{
	return;
	PlayingLevel& pl = PlayingLevelRef();
	QueueGraphics(new GMSetUIs("Skill", GMUI::TEXT, Skill::Name(pl.playerShip->skill)));
	QueueGraphics(new GMSetUIs("Skill", GMUI::TEXTURE_SOURCE, pl.playerShip->activeSkill != NO_SKILL? "0x00FF00FF" : "0x44AA"));
}

void SpaceShooter2D::LoadDefaultName()
{
	GraphicsMan.QueueMessage(new GMSetUIs("PlayerName", GMUI::STRING_INPUT_TEXT, PlayerName()));
	GraphicsMan.QueueMessage(new GMSetUIi("Difficulty", GMUI::INTEGER_INPUT, difficulty->iValue));
}

List<UIElement*> tmpElements;

/// Returns a list of save-files.
void SpaceShooter2D::OpenLoadScreen()
{
	mode = LOAD_SAVES;
	List<SaveFileHeader> headers;
	/// Returns list of all saves, in the form of their SaveFileHeader objects, which should include all info necessary to judge which save to load!
	headers = SaveFile::GetSaves(Application::name);
	// Clear old list.
	String savesList = "Saves";
	GraphicsMan.QueueMessage(new GMClearUI(savesList));
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

		UIElement * fromTemplate = UserInterface::LoadUIAsElement(nullptr, "gui/SaveEntry.gui");

		// h.saveName = filename, not so interesting.
		List<String> additionalData = h.customHeaderData.Tokenize("\n");

		String nameStr, scoreStr, saveDateStr;
		for (int i = 0; i < additionalData.Size(); ++i) {
			String s = additionalData[i];
			if (s.Contains("Name:"))
				nameStr = s - "Name:";
			else if (s.Contains("Score"))
				scoreStr = s;
			else if (s.Contains("Save"))
				saveDateStr = s;
		}
		if (nameStr == "")
			nameStr = "Autosave";

		Text text = nameStr +"  "+ h.dateSaved.ToString("Y/M/D H:m");
		fromTemplate->SetText(text);
		fromTemplate->activationMessage = "LoadGame(" + h.saveName + ")";
		
		saves.Add(fromTemplate);
	}
	GraphicsMan.QueueMessage(new GMAddUI(saves, savesList));
	/// Move cursor to first save in the list.
	if (saves.Size())
	{
		saves[0]->Hover(nullptr);
	}
	else 
	{
		GraphicsMan.QueueMessage(new GMSetHoverUI("Back"));
	}
}

void SpaceShooter2D::OpenJumpDialog()
{
	// Create a string-input.
	static UIStringInput * jumpDialog = 0;
	if (!jumpDialog)
	{
		jumpDialog = new UIStringInput("JumpTo", "JumpToTime");
		//jumpDialog->visuals.textureSource = "0x44AA";
		jumpDialog->onTrigger += "PopUI(JumpTo)&ResumeGame";
		jumpDialog->layout.sizeRatioX = 0.5f;
		jumpDialog->layout.sizeRatioY = 0.1f;
		jumpDialog->CreateChildren(nullptr);
		jumpDialog->input->BeginInput(nullptr); // Make its input active straight away.
		// Add it to the main UI.
		QueueGraphics(new GMAddUI(jumpDialog, "root"));
	}
	else {
		jumpDialog->input->BeginInput(nullptr); // Make its input active straight away.
		QueueGraphics(GMPushUI::ToUI(jumpDialog, MainWindow()->ui));
	}
		// Close it afterwards.
}



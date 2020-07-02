/// Emil Hedemalm
/// 2015-01-20
/// Space shooter.
/// For the Karl-Emil SpaceShooter project, mainly 2014-2015/

#include "Message/FileEvent.h"
#include "MovementPattern.h"

#include "SpaceShooter2D.h"
#include "Base/WeaponScript.h"

#include "Application/Application.h"
#include "StateManager.h"
#include "Level/SpawnGroup.h"

#include "Window/AppWindow.h"
#include "Viewport.h"

#include "OS/OSUtil.h"
#include "OS/Sleep.h"
#include "File/SaveFile.h"
#include "UI/UIUtil.h"

#include "Text/TextManager.h"
#include "Graphics/Messages/GMRenderPass.h"
#include "Render/RenderPass.h"

#include "Message/MathMessage.h"
#include "Base/Gear.h"
#include "PlayingLevel.h"

#include "SpaceShooter2D.h"

#include "Input/Action.h"
#include "Input/InputManager.h"

List<Weapon> Weapon::types;
List<ShipPtr> Ship::types;

Time SpaceShooter2D::startDate;

/// If true, queues up messages so the player automatically starts a new game with the default name and difficulty.
bool introTest = false;

void SetApplicationDefaults()
{
	Application::name = "SpaceShooter2D";
	TextFont::defaultFontSource = "img/fonts/font3.png";
	PhysicsProperty::defaultUseQuaternions = false;
	UIElement::defaultTextureSource = "0x22AA";
}

// Global variables.
SpaceShooter2D * spaceShooter = nullptr;
String SpaceShooter2D::levelToLoad = String();
bool SpaceShooter2D::paused = false;

PlayingLevel* playingLevel = nullptr;

float playingFieldPadding;
/// All ships, including player.
List< std::shared_ptr<Entity> > shipEntities;
List< std::shared_ptr<Entity> > projectileEntities;
String playerName;
/// o.o
String onDeath; // What happens when the player dies?
String previousActiveUpgrade;

String SSGameModeString(SSGameMode mode) {
	switch (mode) {
	case SSGameMode::MAIN_MENU: return "Main Menu";
	case SSGameMode::NEW_GAME: return "New Game";
	case SSGameMode::EDITING_OPTIONS: return "Options";
	default:
		break;
	}
	return "" + mode;
}

#include "PlayingLevel.h"

void RegisterStates()
{
	spaceShooter = new SpaceShooter2D();
	playingLevel = new PlayingLevel();
	StateMan.RegisterState(spaceShooter);
	StateMan.RegisterState(playingLevel);
	StateMan.QueueState(spaceShooter);
}

bool SpaceShooter2D::shipDataLoaded = false;

SpaceShooter2D::SpaceShooter2D()
{
	levelCamera = NULL;
	playingFieldPadding = 1.f;
	gearCategory = 0;
	previousMode = mode = SSGameMode::START_UP;
	// Default vel smoothing.
	PhysicsProperty::defaultVelocitySmoothing = 0.02f;
	PhysicsProperty::defaultLinearDamping = 1.f;
}

SpaceShooter2D::~SpaceShooter2D()
{
	Ship::types.Clear();
}

/// Function when entering this state, providing a pointer to the previous StateMan.
void SpaceShooter2D::OnEnter(AppState * previousState)
{

	QueuePhysics(new PMSeti(PT_AABB_SWEEPER_DIVISIONS, 1));// subdivisionsZ

	WeaponScript::CreateDefault();
	/// Enable Input-UI navigation via arrow-keys and Enter/Esc.
	InputMan.ForceNavigateUI(true);

	/// Create game variables.
	currentLevel = GameVars.CreateInt("currentLevel", 1);
	currentStage = GameVars.CreateInt("currentStage", 1);
	playerName = GameVars.CreateString("playerName", "Cytine");
	score = GameVars.CreateInt("score", 0);
	money = GameVars.CreateInt("money", 200);
	playTime = GameVars.CreateInt("playTime", 0);
	gameStartDate = GameVars.CreateTime("gameStartDate");
	difficulty = GameVars.CreateInt("difficulty", 1);

	AppWindow * w = MainWindow();
	assert(w);
	Viewport * vp = w->MainViewport();
	assert(vp);
	vp->renderGrid = false;	

	// Add custom render-pass to be used.
	RenderPass * rs = new RenderPass();
	rs->type = RenderPass::RENDER_APP_STATE;
	GraphicsMan.QueueMessage(new GMAddRenderPass(rs));

	// Set folder to use for saves.
	String homeFolder = OSUtil::GetHomeDirectory();
	homeFolder.Replace('\\', '/');
	SaveFile::saveFolder = homeFolder;

	// Remove overlay.
	// Set up ui.
//	if (!ui)
//		CreateUserInterface();
	// Set UI without delay.
//	GraphicsMan.ProcessMessage(new GMSetUI(ui));
	GraphicsMan.ProcessMessage(new GMSetOverlay(NULL));

	// Load Space Race integrator
	integrator = new SSIntegrator(0.f);
	PhysicsMan.QueueMessage(new PMSet(integrator));
	cd = new SpaceShooterCD();
	PhysicsMan.QueueMessage(new PMSet(cd));
	cr = new SpaceShooterCR();
	PhysicsMan.QueueMessage(new PMSet(cr));

	PhysicsMan.checkType = AABB_SWEEP;

	/// Enter main menu
//	OpenMainMenu();

	TextMan.LoadFromDir();
	TextMan.SetLanguage("English");

	// Run OnEnter.ini start script if such a file exists.
	if (firstTimeEntering) {
		// Run OnStartApp script.
		ScriptMan.PlayScript("scripts/OnStartApp.txt");

		Script* script = new Script();
		script->Load("OnEnter.ini");
		ScriptMan.PlayScript(script);
		firstTimeEntering = false;
	}


	// Remove initial cover screen.
	QueueGraphics(new GMSetOverlay(NULL));
}


Time now;
// int64 nowMs;
int timeElapsedMs;

/// Main processing function, using provided time since last frame.
void SpaceShooter2D::Process(int timeInMs)
{
	SleepThread(10);
//	std::cout<<"\nSS2D entities: "<<shipEntities.Size() + projectileEntities.Size() + 1;
//	if (playerShip) std::cout<<"\nPlayer position: "<<playerShip->position;

	now = Time::Now();
	timeElapsedMs = timeInMs;
	
}

/// Function when leaving this state, providing a pointer to the next StateMan.
void SpaceShooter2D::OnExit(AppState * nextState)
{
}

/// Creates the user interface for this state
void SpaceShooter2D::CreateUserInterface()
{
	if (ui)
		delete ui;
	ui = new UserInterface();
	ui->CreateRoot();
//	ui->Load("gui/MainMenu.gui");
}


void PrintEntityData(EntitySharedPtr entity)
{
	std::cout<<"\n\nEntity name: "<<entity->name
		<<"\nPosition: "<<entity->worldPosition
		<<" CC: "<<entity->physics->collisionCategory<<" CF: "<<entity->physics->collisionFilter
		<<" Type: "<<(entity->physics->type == PhysicsType::DYNAMIC? "Dynamic" : (entity->physics->type == PhysicsType::KINEMATIC? "Kinematic" : "Static"))
		<<"\nCollission enabled: "<<(entity->physics->collisionsEnabled? "Yes" : "No")<<", Physical radius: "<<entity->physics->physicalRadius
		<<", NoCollissionResolutions: "<<(entity->physics->noCollisionResolutions? "Yes" : "No");
	ShipProperty * sp = (ShipProperty*) entity->GetProperty(ShipProperty::ID());
	if (sp)
	{
		const ShipPtr ship = sp->GetShip();
		std::cout<<"\nSleeping: "<<(sp->sleeping? "Yes" : "No")
			<<", HP: "<<ship->hp<<", Allied: "<<(ship->allied? "Yes" : "No")
			<<"\nLastCollission: "<<ship->lastShipCollision.Seconds()<<", CollisionDmgCooldown: "<<ship->collisionDamageCooldown.Seconds()
			<<"\nMovement pattern: "<<ship->spawnGroup->mp.name<<" CurrMove: "<<ship->movements[ship->currentMovement].Name()
			<<"\nSGFormation: "<<GetName(ship->spawnGroup->formation)<<" SGAmount: "<<ship->spawnGroup->number;
	}
}

/// Callback function that will be triggered via the MessageManager when messages are processed.
void SpaceShooter2D::ProcessMessage(Message * message)
{
	String msg = message->msg;
	if (mode == SSGameMode::EDIT_WEAPON_SWITCH_SCRIPTS)
		ProcessMessageWSS(message);
	switch(message->type)
	{
		case MessageType::RAYCAST: 
		{
			Raycast * rayc = (Raycast*) message;
			std::cout<<"\nRaycast hit "<<rayc->isecs.Size()<<" entities/intersections.";
			for (int i = 0; i < rayc->isecs.Size(); ++i)
			{
				Intersection isec = rayc->isecs[i];
				PrintEntityData(isec.entity);
			}
			break;
		}
		case MessageType::FILE_EVENT:
		{
			FileEvent * fv = (FileEvent *) message;
			for (int i = 0; i < fv->files.Size(); ++i)
			{
				String file = fv->files[i];
				if (file.Contains(".srl"))
				{
					level.Load(file);
					level.OnEnter();
				}
				if (file.Contains(".csv"))
				{
					// Load stuff?
				}
			}
			break;
		}
		case MessageType::SET_STRING:
		{
			SetStringMessage * strMes = (SetStringMessage *) message;
			if (msg == "lall")
			{
				playerName->strValue = strMes->value;
			}
			if (msg == "JumpToTime")
			{
				playingLevel->JumpToTime(strMes->value);
			}

			break;
		}
		case MessageType::INTEGER_MESSAGE:
		{
			IntegerMessage * im = (IntegerMessage*) message;
			if (msg == "SetGearCategory")
			{
				gearCategory = im->value;
				UpdateGearList();
			}
			else if (msg == "SetDifficulty")
			{
				difficulty->iValue = im->value;
			}
			else if (msg == "SetMasterVolume")
			{
				QueueAudio(new AMSet(AT_MASTER_VOLUME, im->value * 0.01f));
			}
			break;
		}

		case MessageType::ON_UI_ELEMENT_HOVER:
		{
			if (msg.StartsWith("SetHoverUpgrade:"))
			{
				String upgrade = msg.Tokenize(":")[1];
				UpdateHoverUpgrade(upgrade);
			}
			break;
		}
		case MessageType::STRING:
		{
			msg.RemoveSurroundingWhitespaces();
			int found = msg.Find("//");
			if (found > 0)
				msg = msg.Part(0,found);

			break;
		}
	}
	ProcessGeneralMessage(message);
}


// Process messages which can be sent from any game state really.
void SpaceShooter2D::ProcessGeneralMessage(Message* message) {
	if (message->type == MessageType::STRING) {
		String msg = message->msg;
		msg.RemoveSurroundingWhitespaces();
		int found = msg.Find("//");
		if (found > 0)
			msg = msg.Part(0, found);

		if (msg == "OpenOptionsScreen")
		{
			SetMode(EDITING_OPTIONS);
		}
		if (msg == "ProceedMessage")
		{
			level.ProceedMessage();
		}
		if (msg == "NewGame")
			NewGame();
		if (msg == "GoToHangar")
		{
			SetMode(IN_HANGAR);
		}
		if (msg == "GoToEditWeaponSwitchScripts")
		{
			SetMode(EDIT_WEAPON_SWITCH_SCRIPTS);
		}
		if (msg == "GoToWorkshop")
		{
			SetMode(IN_WORKSHOP);
		}

		if (msg.StartsWith("SetOnDeath:"))
		{
			onDeath = msg - "SetOnDeath:";
		}
		if (msg.StartsWith("ActiveUpgrade:"))
		{
			String upgrade = msg.Tokenize(":")[1];
			//      if (previousActiveUpgrade == upgrade)
			BuySellToUpgrade(upgrade);
			UpdateHoverUpgrade(upgrade, true);
			//      UpdateActiveUpgrade(upgrade);
			previousActiveUpgrade = upgrade;
		}
		if (msg == "OpenJumpDialog")
		{
			Pause();
			OpenJumpDialog();
		}
		else if (msg.StartsWith("LevelToLoad:"))
		{
			String source = msg;
			source.Remove("LevelToLoad:");
			source.RemoveSurroundingWhitespaces();
			levelToLoad = source;
		}
		else if (msg.Contains("GoToLobby"))
		{
			SetMode(IN_LOBBY);
		}
		else if (msg == "OnReloadUI")
		{
			UpdateUI();
		}
		else if (msg == "ResetCamera")
		{
			ResetCamera();
		}

		if (msg.StartsWith("ActiveUpgrade:"))
		{
			String upgrade = msg.Tokenize(":")[1];
			//	if (previousActiveUpgrade == upgrade)
			BuySellToUpgrade(upgrade);
			UpdateHoverUpgrade(upgrade, true);
			//	UpdateActiveUpgrade(upgrade);
			previousActiveUpgrade = upgrade;
		}

		else if (msg == "Reload OnEnter")
		{
			// Run OnEnter.ini start script if such a file exists.
			Script* script = new Script();
			script->Load("OnEnter.ini");
			ScriptMan.PlayScript(script);
		}

		else if (msg.StartsWith("ShowGearDesc:"))
		{
			String text = msg;
			text.Remove("ShowGearDesc:");
			GraphicsMan.QueueMessage(new GMSetUIs("GearInfo", GMUI::TEXT, text));
		}
		else if (msg.StartsWith("BuyGear:"))
		{
			LogMain("Reimplement", INFO);
			/*
			String name = msg;
			name.Remove("BuyGear:");
			Gear gear = Gear::Get(name);
			switch(gear.type)
			{
				case Gear::SHIELD_GENERATOR:
					playerShip->shield = gear;
					break;
				case Gear::ARMOR:
					playerShip->armor = gear;
					break;
			}
			// Update stats.
			playerShip->UpdateStatsFromGear();
			/// Reduce munny
			money->iValue -= gear.price;
			/// Update UI to reflect reduced funds.
			UpdateGearList();
			// Play some SFX too?

			// Auto-save.
			MesMan.QueueMessages("AutoSave(silent)");
			*/
		}
		else if (msg.Contains("ExitToMainMenu"))
		{
			SetMode(MAIN_MENU);
		}
		else if (msg.Contains("AutoSave"))
		{
			bool silent = msg.Contains("(silent)");
			bool ok = SaveGame();
			if (ok)
			{
				if (silent)
					return;
				GraphicsMan.QueueMessage(new GMSetUIs("CenterText", GMUI::TEXT, "Auto-save: Progress saved"));
				ScriptMan.NewScript(List<String>("Wait(1500)", "ClearCenterText"));
			}
			else
			{
				GraphicsMan.QueueMessage(new GMSetUIs("CenterText", GMUI::TEXT, "Auto-save: Failed. Details: " + lastError));
				ScriptMan.NewScript(List<String>("Wait(6000)", "ClearCenterText"));
			}
		}
		else if (msg == "OpenLoadScreen")
		{
			OpenLoadScreen();
		}
		else if (msg.Contains("LoadGame("))
		{
			bool ok = LoadGame(msg.Tokenize("()")[1]);
			if (ok)
			{
				// Data loaded. Check which state we should enter?
				if (currentLevel->iValue < 4)
				{
					// Enter the next-level straight away.
					MesMan.QueueMessages("NextLevel");
				}
				else
				{
					SetMode(IN_LOBBY);
				}
			}
			else {
				GraphicsMan.QueueMessage(new GMSetUIs("CenterText", GMUI::TEXT, "Load failed. Details: " + lastError));
				ScriptMan.NewScript(List<String>("Wait(6000)", "ClearCenterText"));
			}
		}
		else if (msg == "OpenMainMenu" || msg == "Back" ||
			msg == "GoToMainMenu")
			OpenMainMenu();
		else if (msg == "OpenNewGameMenu")
			SetMode(NEW_GAME, false);
		else if (msg == "LoadDefaultName")
			LoadDefaultName();
		else if (msg == "GoToPreviousMode")
			SetMode(previousMode);
		else if (msg == "Pause/Break" || msg == "TogglePause")
		{
			TogglePause();
		}
		else if (msg == "ResumeGame")
			Resume();
		else if (msg == "ListEntitiesAndRegistrations")
		{
			std::cout << "\nGraphics entities " << GraphicsMan.RegisteredEntities() << " physics " << PhysicsMan.RegisteredEntities()
				<< " projectiles " << projectileEntities.Size() << " ships " << shipEntities.Size();
		}

	}
}

/// Creates default key-bindings for the state.
void SpaceShooter2D::CreateDefaultBindings()
{
	List<Binding*>& bindings = this->inputMapping.bindings;
#define BINDING(a,b) bindings.Add(new Binding(a,b));
	BINDING(Action::FromString("TogglePlayerInvulnerability"), KEY::I);
	BINDING(Action::CreateStartStopAction("MoveShipUp"), KEY::W);
	BINDING(Action::CreateStartStopAction("MoveShipDown"), KEY::S);
	BINDING(Action::CreateStartStopAction("MoveShipLeft"), KEY::A);
	BINDING(Action::CreateStartStopAction("MoveShipRight"), KEY::D);
	BINDING(Action::FromString("ReloadWeapon"), KEY::R);
	BINDING(Action::FromString("ToggleWeaponScript"), KEY::E);
	BINDING(Action::FromString("ActivateSkill"), KEY::Q);
	BINDING(Action::FromString("ResetCamera"), KEY::HOME);
	BINDING(Action::FromString("NewGame"), List<int>(KEY::N, KEY::G));
	BINDING(Action::FromString("ClearLevel"), List<int>(KEY::C, KEY::L));
	BINDING(Action::FromString("ListEntitiesAndRegistrations"), List<int>(KEY::L, KEY::E));
	BINDING(Action::FromString("ToggleBlackness"), List<int>(KEY::T, KEY::B));
	BINDING(Action::FromString("NextLevel"), List<int>(KEY::N, KEY::L));
	BINDING(Action::FromString("PreviousLevel"), List<int>(KEY::P, KEY::L));
	BINDING(Action::FromString("ToggleMenu"), KEY::ESCAPE);
	BINDING(Action::FromString("ToggleMute"), KEY::M);
#define BIND BINDING
	BIND(Action::FromString("AdjustMasterVolume(0.05)", ACTIVATE_ON_REPEAT), List<int>(KEY::CTRL, KEY::V, KEY::PLUS));
	BIND(Action::FromString("AdjustMasterVolume(-0.05)", ACTIVATE_ON_REPEAT), List<int>(KEY::CTRL, KEY::V, KEY::MINUS));

	for (int i = 0; i < WeaponType::MAX_TYPES; ++i)
	{
		/// Debug bindings for adjusting weapon levels mid-flight.
		BIND(Action::FromString("IncreaseWeaponLevel:" + String(i)), List<int>(KEY::PLUS, KEY::ONE + i));
		BIND(Action::FromString("DecreaseWeaponLevel:" + String(i)), List<int>(KEY::MINUS, KEY::ONE + i));
		BIND(Action::FromString("IncreaseWeaponLevel:" + String(i)), List<int>(KEY::ONE + i, KEY::PLUS));
		BIND(Action::FromString("DecreaseWeaponLevel:" + String(i)), List<int>(KEY::ONE + i, KEY::MINUS));
		BIND(Action::FromString("Weapon:" + String(i + 1)), KEY::ONE + i);
	}

	BIND(Action::FromString("ListPhysicalEntities"), List<int>(KEY::L, KEY::P));
	BIND(Action::FromString("Reload OnEnter"), List<int>(KEY::CTRL, KEY::O, KEY::E));
	/*
	BIND(Action::FromString("Weapon:1"), KEY::ONE);
	BIND(Action::FromString("Weapon:2"), KEY::TWO);
	BIND(Action::FromString("Weapon:3"), KEY::THREE);
	BIND(Action::FromString("Weapon:4"), KEY::FOUR);
	BIND(Action::FromString("Weapon:5"), KEY::FIVE);
	*/
	BIND(Action::CreateStartStopAction("Shooting"), KEY::SPACE);
	BIND(Action::FromString("OpenJumpDialog"), List<int>(KEY::CTRL, KEY::G));
	BIND(Action::FromString("ProceedMessage"), KEY::ENTER);

}


/// Callback from the Input-manager, query it for additional information as needed.
void SpaceShooter2D::KeyPressed(int keyCode, bool downBefore)
{

}


/// o.o
EntitySharedPtr SpaceShooter2D::OnShipDestroyed(ShipPtr ship)
{
		// Explode
//	EntitySharedPtr explosionEntity = spaceShooter->NewExplosion(owner->position, ship);
//	game->explosions.Add(explosionEntity);
	return NULL;
}

String SpaceShooter2D::GetLevelVarName(String levelPath, String name)
{
	if (levelPath == "current")
		levelPath = level.source;
	return "Level_"+ levelPath +"_"+ name;
}

#define GetVar(varName)\
	String name = GetLevelVarName(level, varName);\
	GameVar * gv = GameVars.Get(name);\
	if (!gv)\
		gv = GameVars.CreateInt(name, 0);\
	return gv;\

/// Level score. If -1, returns current.
GameVariable * SpaceShooter2D::LevelScore(String level)
{
	GetVar("score");
}

/// Level score. If -1, returns current.
GameVariable * SpaceShooter2D::LevelKills(String level)
{
	GetVar("kills");
}

GameVariable * SpaceShooter2D::LevelPossibleKills(String level)
{
	GetVar("possibleKills");
}

/// Resets all the above.
void SpaceShooter2D::ResetLevelStats()
{
	LevelScore()->iValue = 0;
	LevelKills()->iValue = 0;
	LevelPossibleKills()->iValue = 0;
}


void SpaceShooter2D::LoadShipData()
{

	/// Fetch file which dictates where to load weapons and ships from.
	List<String> lines = File::GetLines("ToLoad.txt");
	enum {
		MOVEMENTS, SHIPS, WEAPONS
	};
	int parseMode = SHIPS;
	for (int i = 0; i < lines.Size(); ++i)
	{
		String line = lines[i];
		if (line.StartsWith("//"))
			continue;
		if (line.Contains("Movements:"))
			parseMode = MOVEMENTS;
		else if (line.Contains("Ships:"))
			parseMode = SHIPS;
		else if (line.Contains("Weapons:"))
			parseMode = WEAPONS;
		else if (parseMode == MOVEMENTS)
			MovementPattern::LoadPatterns(line);
		else if (parseMode == SHIPS)
			Ship::LoadTypes(line);	
		else if (parseMode == WEAPONS)
			Weapon::LoadTypes(line);	
	}
	// Load shop-data.
	Gear::Load("data/ShopWeapons.csv");
	Gear::Load("data/ShopArmors.csv");
	Gear::Load("data/ShopShields.csv");
	shipDataLoaded = true;
}


/// Starts a new game. Calls LoadLevel
void SpaceShooter2D::NewGame()
{
	PopUI("NewGame");
	PopUI("MainMenu");
		
	startDate = Time::Now();

	// Reset scores.
	score->iValue = 0;
	// Set stage n level
	currentStage->iValue = 0;
	currentLevel->iValue = 0;

	StateMan.QueueState(playingLevel);
}



void SpaceShooter2D::Pause()
{
	paused = true;
	OnPauseStateUpdated();
}
void SpaceShooter2D::Resume()
{
	paused = false;
	OnPauseStateUpdated();
}

void SpaceShooter2D::TogglePause()
{
	paused = !paused;
	OnPauseStateUpdated();
}

void SpaceShooter2D::OnPauseStateUpdated()
{
	if (paused)
	{
		GraphicsMan.QueueMessage(new GraphicsMessage(GM_PAUSE_PROCESSING));
		PhysicsMan.Pause();
		QueueAudio(new AMGlobal(AM_PAUSE_PLAYBACK));
	}
	else {
		GraphicsMan.QueueMessage(new GraphicsMessage(GM_RESUME_PROCESSING));
		PhysicsMan.Resume();
		QueueAudio(new AMGlobal(AM_RESUME_PLAYBACK));
	}
}



void SpaceShooter2D::GameOver()
{
	if (mode != GAME_OVER)
	{
		SetMode(GAME_OVER);
		// Play script for animation or whatever.
		ScriptMan.PlayScript("scripts/GameOver.txt");
		// End script by going back to menu or playing a new game.
	}
}

void SpaceShooter2D::OnLevelCleared()
{
	if (mode != LEVEL_CLEARED)
	{
		SetMode(LEVEL_CLEARED);
		ScriptMan.PlayScript("scripts/LevelComplete.txt");
	}
}

/// Opens main menu.
void SpaceShooter2D::OpenMainMenu()
{
	SetMode(MAIN_MENU);
	/// Proceed straight away if test.
	if (introTest)
	{
		NewGame();
	}
}

void SpaceShooter2D::MouseClick(AppWindow * appWindow, bool down, int x, int y, UIElement * elementClicked)
{
	if (down)
	{
		Ray ray; 
		appWindow->GetRayFromScreenCoordinates(x,y, ray);
		PhysicsMan.QueueMessage(new PMRaycast(ray));
	}
}


/// Where the ship will be re-fitted and new gear bought.
void SpaceShooter2D::EnterShipWorkshop()
{
	SetMode(IN_WORKSHOP);
}

/// Saves current progress.
bool SpaceShooter2D::SaveGame()
{
	SaveFile save(Application::name, playerName->strValue + gameStartDate->ToString());
	String customHeaderData = "Name: "+playerName->strValue+"\nStage: "+currentStage->ToString()+" Level: "+currentLevel->ToString()+
		"\nScore: " + String(score->iValue) + 
		"\nSave date: " + Time::Now().ToString("Y-M-D") + 
		"\nStart date: " + startDate.ToString("Y-M-D");
	
	bool ok = save.OpenSaveFileStream(customHeaderData, true);
	if (!ok)
	{
		lastError = save.lastError;
		return false;
	}
	assert(ok);
	std::fstream & stream = save.GetStream();
	if (!GameVars.WriteTo(stream))
	{
		lastError = "Unable to save GameVariables to stream.";
		return false;
	}
	// Close the stream.
	stream.close();
	// Save custom data.
	return true;
}

/// Loads progress from target save.
bool SpaceShooter2D::LoadGame(String saveName)
{
	SaveFile save(Application::name, saveName);
	String cHeaderData;
	bool ok = save.OpenLoadFileStream();
	if (!ok)
	{
		lastError = save.lastError;
		return false;
	}
	std::fstream & stream = save.GetStream();
	if (!GameVars.ReadFrom(stream))
	{
		lastError = "Unable to read GameVariables from save.";
		return false;
	}
	// Close the stream.
	stream.close();
	return true;
}


void SpaceShooter2D::ResetCamera()
{
	levelCamera->projectionType = Camera::ORTHOGONAL;
	levelCamera->zoom = 15.f;
}

List<SSGameMode> previousModes;
/// Saves previousMode
void SpaceShooter2D::SetMode(SSGameMode newMode, bool updateUI)
{
	switch (newMode) {
	case SSGameMode::MAIN_MENU:
		StateMan.QueueState(spaceShooter);
		break;
	}

	if (mode == newMode) {
		LogMain("Already within mode " + SSGameModeString(newMode)+", updating UI just in case...", INFO);
		UpdateUI();
		return;
	}
	if (previousMode != mode)
	{
		previousMode = mode;
		previousModes.AddItem(mode); // List of modes for backing much?
	}
	LogMain("Entering game mode: " + SSGameModeString(newMode) + ", previously: " + SSGameModeString(mode), INFO);
	mode = newMode;
	// Update UI automagically?
	if (updateUI)
		UpdateUI();
}


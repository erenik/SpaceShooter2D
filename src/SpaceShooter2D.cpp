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

#include "Physics/Messages/CollisionCallback.h"
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

#include "Input/InputManager.h"

/// Global pause, used for pause/Break, etc. Most calculations should stop/halt while paused.
bool paused = false;

List<Weapon> Weapon::types;
List<Ship*> Ship::types;

bool shipDataLoaded = false;

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
PlayingLevel* playingLevel = nullptr;
Ship * playerShip = 0;
/// The level entity, around which the playing field and camera are based upon.
EntitySharedPtr levelEntity = NULL;
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

SpaceShooter2D::SpaceShooter2D()
{
//	playerShip = new Ship();
	levelCamera = NULL;
	SetPlayingFieldSize(Vector2f(30,20));
	levelEntity = NULL;
	playingFieldPadding = 1.f;
	gearCategory = 0;
	previousMode = mode = SSGameMode::START_UP;
	// Default vel smoothing.
	PhysicsProperty::defaultVelocitySmoothing = 0.02f;
	PhysicsProperty::defaultLinearDamping = 1.f;
}

SpaceShooter2D::~SpaceShooter2D()
{
	Ship::types.ClearAndDelete();
	delete playerShip;
}

/// Function when entering this state, providing a pointer to the previous StateMan.
void SpaceShooter2D::OnEnter(AppState * previousState)
{

	MovementPattern::Load();
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

	// Run OnStartApp script.
	ScriptMan.PlayScript("scripts/OnStartApp.txt");

	/// Enter main menu
//	OpenMainMenu();

	TextMan.LoadFromDir();
	TextMan.SetLanguage("English");

	NewPlayer();

	// Run OnEnter.ini start script if such a file exists.
	Script * script = new Script();
	script->Load("OnEnter.ini");
	ScriptMan.PlayScript(script);

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

/// Searches among actively spawned ships.
Ship * SpaceShooter2D::GetShip(EntitySharedPtr forEntity)
{
	for (int i = 0; i < level.ships.Size(); ++i)
	{
		Ship * ship = level.ships[i];
		if (ship->entity == forEntity)
			return ship;
	}
	return 0;
}

Ship * SpaceShooter2D::GetShipByID(int id)
{
	for (int i = 0; i < level.ships.Size(); ++i)
	{
		Ship * ship = level.ships[i];
		if (ship->ID() == id)
			return ship;
	}
	if (id == playerShip->ID())
		return playerShip;
	return 0;
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

bool playerInvulnerability = false;
void OnPlayerInvulnerabilityUpdated()
{
	
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
		const Ship * ship = sp->GetShip();
		std::cout<<"\nSleeping: "<<(sp->sleeping? "Yes" : "No")
			<<", HP: "<<ship->hp<<", Allied: "<<(ship->allied? "Yes" : "No")
			<<"\nLastCollission: "<<ship->lastShipCollision.Seconds()<<", CollisionDmgCooldown: "<<ship->collisionDamageCooldown.Seconds()
			<<"\nMovement pattern: "<<ship->spawnGroup->mp.name<<" CurrMove: "<<ship->movements[ship->currentMovement].Name()
			<<"\nSGFormation: "<<Formation::GetName(ship->spawnGroup->formation)<<" SGAmount: "<<ship->spawnGroup->number;
	}
}

/// Callback function that will be triggered via the MessageManager when messages are processed.
void SpaceShooter2D::ProcessMessage(Message * message)
{
	String msg = message->msg;
	if (mode == SSGameMode::EDIT_WEAPON_SWITCH_SCRIPTS)
		ProcessMessageWSS(message);
	level.ProcessMessage(message);
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
				level.JumpToTime(strMes->value);
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
			else if (msg == "SetActiveWeapon")
			{
				playerShip->SwitchToWeapon(im->value);
			}
			break;
		}
		case MessageType::COLLISSION_CALLBACK:
		{

			CollisionCallback * cc = (CollisionCallback*) message;
			EntitySharedPtr one = cc->one;
			EntitySharedPtr two = cc->two;
#define SHIP 0
#define PROJ 1
//			std::cout<<"\nColCal: "<<cc->one->name<<" & "<<cc->two->name;

			EntitySharedPtr shipEntity1 = NULL;
			EntitySharedPtr other = NULL;
			int oneType = (one == playerShip->entity || shipEntities.Exists(one)) ? SHIP : PROJ;
			int twoType = (two == playerShip->entity || shipEntities.Exists(two)) ? SHIP : PROJ;
			int types[5] = {0,0,0,0,0};
			++types[oneType];
			++types[twoType];
		//	std::cout<<"\nCollision between "<<one->name<<" and "<<two->name;
			if (oneType == SHIP)
			{
				ShipProperty * shipProp = (ShipProperty*)one->GetProperty(ShipProperty::ID());
				if (shipProp)
					shipProp->OnCollision(two);
			}
			else if (twoType == SHIP)
			{
				ShipProperty * shipProp = (ShipProperty*)two->GetProperty(ShipProperty::ID());
				if (shipProp)
					shipProp->OnCollision(one);
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
			if (msg == "ProceedMessage")
			{
				level.ProceedMessage();
			}
			if (msg == "NewGame")
				NewGame();
			if (msg == "TogglePlayerInvulnerability")
			{
				playerInvulnerability = !playerInvulnerability;
				OnPlayerInvulnerabilityUpdated();
			}
			if (msg == "ReloadWeapon")
			{
				if (playerShip->activeWeapon == nullptr)
					return;
				playerShip->activeWeapon->QueueReload();
			}
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
			if (msg == "ActivateSkill")
			{
				playerShip->ActivateSkill();
			}
//			std::cout<<"\n"<<msg;
			if (msg == "TutorialBaseGun")
			{
				playerShip->weapons.Clear(); // Clear old wepaons.
				playerShip->SetWeaponLevel(WeaponType::TYPE_0, 1);
				playerShip->activeWeapon = playerShip->weapons[0];
				UpdateHUDGearedWeapons();
			}
			if (msg == "TutorialLevel1Weapons")
			{
				playerShip->SetWeaponLevel(WeaponType::TYPE_0, 1);
				playerShip->SetWeaponLevel(WeaponType::TYPE_1, 1);
				playerShip->SetWeaponLevel(WeaponType::TYPE_2, 1);
				UpdateHUDGearedWeapons();
			}
			if (msg == "TutorialLevel3Weapons")
			{
				playerShip->SetWeaponLevel(WeaponType::TYPE_0, 3);
				playerShip->SetWeaponLevel(WeaponType::TYPE_1, 3);
				playerShip->SetWeaponLevel(WeaponType::TYPE_2, 3);			
				UpdateHUDGearedWeapons();
			}
			if (msg.StartsWith("DecreaseWeaponLevel:"))
			{
				List<String> parts = msg.Tokenize(":");
				int weaponIndex = parts[1].ParseInt();
				Weapon * weap = playerShip->GetWeapon(WeaponType::TYPE_0 + weaponIndex);
				int currLevel = weap->level;
				playerShip->SetWeaponLevel(WeaponType::TYPE_0 + weaponIndex, currLevel-1);				
				std::cout<<"\nWeapon "<<weap->type<<" set to level "<<weap->level<<": "<<weap->name;
			}
			if (msg.StartsWith("IncreaseWeaponLevel:"))
			{
				List<String> parts = msg.Tokenize(":");
				int weaponIndex = parts[1].ParseInt();
				Weapon * weap = playerShip->GetWeapon(WeaponType::TYPE_0 + weaponIndex);
				int currLevel = weap->level;
				playerShip->SetWeaponLevel(WeaponType::TYPE_0 + weaponIndex, currLevel+1);
				std::cout<<"\nWeapon "<<weap->type<<" set to level "<<weap->level<<": "<<weap->name;
			}
			else if (msg == "AllTheWeapons")
			{
				for (int i = 0; i < WeaponType::MAX_TYPES; ++i)
				{
					if (playerShip->weapons[i]->level <= 0)
						playerShip->SetWeaponLevel(i, 1);
					UpdateHUDGearedWeapons();
				}
			}
			if (msg == "ToggleWeaponScript")
			{
				if (playerShip->weaponScript == 0)
					playerShip->weaponScript = WeaponScript::LastEdited();
				playerShip->weaponScriptActive = !playerShip->weaponScriptActive;
			}
			if (msg == "ActivateWeaponScript")
			{
				playerShip->weaponScriptActive = true;
				if (playerShip->weaponScript == 0)
					playerShip->weaponScript = WeaponScript::LastEdited();
			}
			if (msg == "DeactivateWeaponScript")
			{
				playerShip->weaponScriptActive = false;
				if (!InputMan.KeyPressed(KEY::SPACE))
					playerShip->shoot = false;
			}
			if (msg.StartsWith("SetSkill"))
			{
				String skill = msg.Tokenize(":")[1];
				if (skill == "AttackFrenzy")
					playerShip->skill = ATTACK_FRENZY;
				if (skill == "SpeedBoost")
					playerShip->skill = SPEED_BOOST;
				if (skill == "PowerShield")
					playerShip->skill = POWER_SHIELD;
				playerShip->skillName = skill;
				UpdateHUDSkill();
			}
			if (msg == "TutorialSkillCooldowns")
			{
				playerShip->skillCooldownMultiplier = 0.1f;
			}
			if (msg == "DisablePlayerMovement")
			{
				playerShip->movementDisabled = true;
				UpdatePlayerVelocity();
			}
			if (msg == "EnablePlayerMovement")
			{
				playerShip->movementDisabled = false;
				UpdatePlayerVelocity();
			}
			if (msg.StartsWith("SetOnDeath:"))
			{
				onDeath = msg - "SetOnDeath:";
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
			if (msg == "OpenJumpDialog")
			{
				Pause();
				OpenJumpDialog();
			}
			if (msg == "SpawnTutorialBomb")
			{
				// Shoot.
				Color color = Vector4f(0.8f,0.7f,0.1f,1.f);
				Texture * tex = TexMan.GetTextureByColor(color);
				EntitySharedPtr projectileEntity = EntityMan.CreateEntity(name + " Projectile", ModelMan.GetModel("sphere.obj"), tex);
				Weapon weapon;
				weapon.damage = 750;
				ProjectileProperty * projProp = new ProjectileProperty(weapon, projectileEntity, true);
				projectileEntity->properties.Add(projProp);
				// Set scale and position.
				projectileEntity->localPosition = playerShip->entity->worldPosition + Vector3f(30,0,0);
				projectileEntity->SetScale(Vector3f(1,1,1) * 0.5f);
				projProp->color = color;
				projectileEntity->RecalculateMatrix();
				projProp->onCollisionMessage = "ResumeGameTime";
				// pew
				Vector3f dir(-1.f,0,0);
				Vector3f vel = dir * 5.f;
				PhysicsProperty * pp = projectileEntity->physics = new PhysicsProperty();
				pp->type = PhysicsType::DYNAMIC;
				pp->velocity = vel;
				pp->collisionCallback = true;	
				pp->maxCallbacks = 1;
				// Set collision category and filter.
				pp->collisionCategory = CC_ENEMY_PROJ;
				pp->collisionFilter = CC_PLAYER;
				// Add to map.
				MapMan.AddEntity(projectileEntity);
				projectileEntities.Add(projectileEntity);
			}
			if (msg == "UpdateHUDGearedWeapons")
				UpdateHUDGearedWeapons();
			else if (msg.StartsWith("Weapon:"))
			{
				int weaponIndex = msg.Tokenize(":")[1].ParseInt();
				weaponIndex -= 1;
				if (weaponIndex < 0)
					weaponIndex = 9;
				playerShip->SwitchToWeapon(weaponIndex);
			}
			else if (msg == "StartShooting")
			{
				playerShip->shoot = true;
			}
			else if (msg == "Reload OnEnter")
			{
				// Run OnEnter.ini start script if such a file exists.
				Script * script = new Script();
				script->Load("OnEnter.ini");
				ScriptMan.PlayScript(script);
			}
			else if (msg == "StopShooting")
			{
				playerShip->shoot = false;
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
					GraphicsMan.QueueMessage(new GMSetUIs("CenterText", GMUI::TEXT, "Auto-save: Failed. Details: "+lastError));	
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
					GraphicsMan.QueueMessage(new GMSetUIs("CenterText", GMUI::TEXT, "Load failed. Details: "+lastError));	
					ScriptMan.NewScript(List<String>("Wait(6000)", "ClearCenterText"));
				}
			}
			else if (msg == "OpenMainMenu" || msg == "Back" ||
				msg == "GoToMainMenu")
				OpenMainMenu();
			else if (msg == "LoadDefaultName")
			{
				LoadDefaultName();
			}
			else if (msg == "GoToPreviousMode")
			{
				SetMode(previousMode);
			}
			else if (msg == "OpenOptionsScreen")
			{
				SetMode(EDITING_OPTIONS);
			}
			else if (msg == "Pause/Break" || msg == "TogglePause")
			{
				TogglePause();
			}
			else if (msg == "ResumeGame")
				Resume();
			else if (msg == "ListEntitiesAndRegistrations")
			{
				std::cout<<"\nGraphics entities "<<GraphicsMan.RegisteredEntities()<<" physics "<<PhysicsMan.RegisteredEntities()
					<<" projectiles "<<projectileEntities.Size()<<" ships "<<shipEntities.Size();
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
			else if (msg.StartsWith("ShowGearDesc:"))
			{
				String text = msg;
				text.Remove("ShowGearDesc:");
				GraphicsMan.QueueMessage(new GMSetUIs("GearInfo", GMUI::TEXT, text));
			}
			else if (msg.StartsWith("BuyGear:"))
			{
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
			}
			else if (msg.Contains("ExitToMainMenu"))
			{
				SetMode(MAIN_MENU);
			}
			if (msg.StartsWith("DisplayCenterText"))
			{
				String text = msg;
				if (text.Contains("$-L"))
				{
					text.Replace("$-L", currentStage->ToString()+"-"+currentLevel->ToString());
				}
				text.Replace("Stage $", "Stage "+currentStage->ToString());
				text.Remove("DisplayCenterText");
				text.RemoveInitialWhitespaces();
				GraphicsMan.QueueMessage(new GMSetUIs("CenterText", GMUI::TEXT, text));
			}
			else if (msg == "ClearCenterText")
				GraphicsMan.QueueMessage(new GMSetUIs("CenterText", GMUI::TEXT, Text()));
			else if (msg == "OnReloadUI")
			{
				UpdateUI();
			}
			else if (msg == "ResetCamera")
			{
				ResetCamera();
			}
			if (msg.Contains("StartMoveShip"))
			{
				String dirStr = msg - "StartMoveShip";
				int dir = Direction::Get(dirStr);
				movementDirections.Add(dir);
				UpdatePlayerVelocity();
			}
			else if (msg.Contains("StopMoveShip"))
			{
				String dirStr = msg - "StopMoveShip";
				int dir = Direction::Get(dirStr);
				while(movementDirections.Remove(dir));
				UpdatePlayerVelocity();
			}
			break;
		}
	}
}

/// Callback from the Input-manager, query it for additional information as needed.
void SpaceShooter2D::KeyPressed(int keyCode, bool downBefore)
{

}


/// Called from the render-thread for every viewport/AppWindow, after the main rendering-pipeline has done its job.
void SpaceShooter2D::Render(GraphicsState * graphicsState)
{
	switch(mode)
	{
		case PLAYING_LEVEL:	
			if (!levelEntity)
				return;
			RenderInLevel(graphicsState);
			break;
		default:
			return;
	}
}

/// o.o
EntitySharedPtr SpaceShooter2D::OnShipDestroyed(Ship * ship)
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
		SHIPS, WEAPONS
	};
	int parseMode = SHIPS;
	for (int i = 0; i < lines.Size(); ++i)
	{
		String line = lines[i];
		if (line.StartsWith("//"))
			continue;
		if (line.Contains("Ships:"))
			parseMode = SHIPS;
		else if (line.Contains("Weapons:"))
			parseMode = WEAPONS;
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
		
	// Create player.
	NewPlayer();
	startDate = Time::Now();

	// Reset scores.
	score->iValue = 0;
	// Set stage n level
	currentStage->iValue = 0;
	currentLevel->iValue = 0;

	StateMan.QueueState(playingLevel);
}

/// o.o
void SpaceShooter2D::NewPlayer()
{
	if (!shipDataLoaded)
		LoadShipData();

	SAFE_DELETE(playerShip);
	// Reset player-ship.
	if (playerShip == 0)
	{
		playerShip = Ship::New("Default");
		playerShip->enemy = false;
		playerShip->allied = true;
	}
	playerShip->weapon = Gear::StartingWeapon();
	playerShip->armor = Gear::StartingArmor();
	playerShip->shield = Gear::StartingShield();
	playerShip->UpdateStatsFromGear();

	for (int i = 0; i < WeaponType::MAX_TYPES; ++i)
	{
		playerShip->SetWeaponLevel(i, 0);
	}
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



void SpaceShooter2D::UpdatePlayerVelocity()
{
	Vector3f totalVec;
	for (int i = 0; i < movementDirections.Size(); ++i)
	{
		Vector3f vec = Direction::GetVector(movementDirections[i]);
		totalVec += vec;
	}
	totalVec.Normalize();
	totalVec *= playerShip->Speed();
	totalVec *= playerShip->movementDisabled? 0 : 1;
	//totalVec += level.BaseVelocity();

	// Set player speed.
	if (playerShip->entity)
	{
		PhysicsMan.QueueMessage(new PMSetEntity(playerShip->entity, PT_VELOCITY, totalVec));
	}
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
	switch (mode) {
	case SSGameMode::PLAYING_LEVEL:
		StateMan.QueueState(playingLevel);
		return;
	}
	// Update UI automagically?
	if (updateUI)
		UpdateUI();
}


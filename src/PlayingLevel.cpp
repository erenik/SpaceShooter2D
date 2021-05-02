/// Emil Hedemalm
/// 2015-02-07
/// All code pertaining to playing of the actual level.

#include "SpaceShooter2D.h"
#include "PlayingLevel.h"

#include "UI/UIUtil.h"
#include "Window/AppWindowManager.h"
#include "OS/Sleep.h"
#include "Graphics/GraphicsManager.h"
#include "Viewport.h"
#include "Text/TextManager.h"
#include "PlayingLevel/HUD.h"
#include "Physics/Messages/CollisionCallback.h"
#include "Base/WeaponScript.h"
#include "Input/InputManager.h"
#include "Message/MathMessage.h"
#include <File\LogFile.h>
#include <PlayingLevel\HandleCollision.h>
#include "PlayingLevel/SpaceStars.h"
#include "Properties/LevelProperty.h"
#include "Input/Gamepad/GamepadMessage.h"
#include "Application/Application.h"
#include "Level/LevelMessage.h"
#include "Level/SpawnGroup.h"
#include "File/SaveFile.h"
#include "Missions.h"
#include "Base/PlayerShip.h"
#include "Util/String/StringUtil.h"
#include "Input/Binding.h"
#include "Input/InputMapping.h"
#include "Input/Action.h"

#include "LevelEditor/LevelEditor.h"

#include "Test/TutorialTests.h"

SpawnGroup testGroup;
List<SpawnGroup> storedTestGroups;

/// Each other being original position, clamped position, orig, clamp, orig3, clamp3, etc.
List<Vector3f> renderPositions;
bool playerInvulnerability = false;
void OnPlayerInvulnerabilityUpdated()
{

}

Ship* GetPlayerShip() {
	return PlayingLevel::playerShip;
}
Entity* PlayerShipEntity() {
	auto playerShip = GetPlayerShip();
	if (playerShip == nullptr)
		return nullptr;
	return GetPlayerShip()->entity;
}
PlayingLevel& PlayingLevelRef() {
	return *PlayingLevel::singleton;
}


PlayerShip* PlayingLevel::playerShip = nullptr;
Entity* PlayingLevel::levelEntity = nullptr;

GameVariable* SpaceShooter2D::currentLevel = nullptr,
* SpaceShooter2D::currentStage = nullptr,
* SpaceShooter2D::score = nullptr,
* SpaceShooter2D::playTime = nullptr,
* SpaceShooter2D::playerNameVar = nullptr,
* SpaceShooter2D::gameStartDate = nullptr,
* SpaceShooter2D::difficulty = nullptr;


PlayingLevel* PlayingLevel::singleton = nullptr;


//Vector2f PlayingLevel::playingFieldHalfSize = Vector2f(),
//PlayingLevel::playingFieldSize = Vector2f();

PlayingLevel::PlayingLevel()
	: SpaceShooter2D()
	, lastSpawnGroup(nullptr)
	, currentMission(nullptr)
{
	assert(singleton == nullptr);
	singleton = this;
}

Ship* PlayingLevel::GetShipByID(int id)
{
	for (int i = 0; i < level.ships.Size(); ++i)
	{
		Ship* ship = level.ships[i];
		if (ship->ID() == id)
			return ship;
	}
	if (id == playerShip->ID())
		return playerShip;
	return 0;
}

/// UI stuffs. All implemented in UIHandling.cpp
void PlayingLevel::UpdateUI() {
	HUD::Get()->UpdateUI();
}

// Inherited via AppState
void PlayingLevel::OnEnter(AppState* previousState) {

	String testLevel = "";
	if (previousState->GetID() == SSGameMode::LEVEL_EDITOR) {
		auto levelEditor = (LevelEditor*)previousState;
		testLevel = levelEditor->LevelToTest();
		playtestingEditorLevel = true;
	}

	assert(PlayerName().Length() > 0);

	testGroup.number = 1;

	/// Enable Input-UI navigation via arrow-keys and Enter/Esc.
	InputMan.SetNavigateUI(false);
	InputMan.SetForceNavigateUI(false);

	HUD::Get()->Show();

	eventsTriggered.Clear();

	NewPlayer();
	LoadWeapons();

	// New global sparks system.
	sparks = (new Sparks(true));
	Graphics.QueueMessage(new GMRegisterParticleSystem(sparks, true));

	// Check if we are playing a campaign mission
	String levelToLoad;
	if (playtestingEditorLevel) {
		levelToLoad = testLevel;
		currentMission = nullptr;

		// Boost up more weapons apt for play-testing the level.
		playerShip->EquipTutorialLevel1Weapons();
		LoadWeapons();
	}
	else {
		GameVar * currentMissionVar = GameVars.GetString("CurrentMission");
		assert(currentMissionVar);
		Mission * currentMission = MissionsMan.GetMissionByName(currentMissionVar->strValue);
		assert(currentMission);
		levelToLoad = currentMission->levelFilePath;
	}	
	LoadLevel(levelToLoad, currentMission);

	// Play script for animation or whatever.
	ScriptMan.PlayScript("scripts/NewGame.txt");
	// Resume physics/graphics if paused.
	Resume();

	TextMan.LoadFromDir();
	TextMan.SetLanguage("English");

	auto playTutorialVar = GameVars.GetInt("PlayTutorial");
	if (playTutorialVar && !playTutorialVar->iValue == 1) {
		LevelMessage * message = level.GetMessageWithTextId("TutorialConcluded");
		if (message != nullptr)
			JumpToAfterMessage(message);
	}

};

void PlayingLevel::CreateDefaultBindings() {
	List<Binding*>& bindings = this->inputMapping.bindings;
#define BIND(a,b) bindings.Add(new Binding(a,b));
	BIND(Action::FromString("AutoAim"), List<int>(KEY::CTRL, KEY::A));
	BIND(Action::FromString("SpeedUp"), List<int>(KEY::CTRL, KEY::S, KEY::PLUS));
	BIND(Action::FromString("SpeedDown"), List<int>(KEY::CTRL, KEY::S, KEY::MINUS));
	BIND(Action::FromString("TogglePlayerInvulnerability"), KEY::I);
	BIND(Action::CreateStartStopAction("MoveShipUp"), KEY::W);
	BIND(Action::CreateStartStopAction("MoveShipDown"), KEY::S);
	BIND(Action::CreateStartStopAction("MoveShipLeft"), KEY::A);
	BIND(Action::CreateStartStopAction("MoveShipRight"), KEY::D);
	BIND(Action::FromString("ReloadWeapon"), KEY::R);
	BIND(Action::FromString("ToggleWeaponScript"), KEY::E);
	BIND(Action::FromString("ActivateSkill"), KEY::Q);
	BIND(Action::FromString("ResetCamera"), KEY::HOME);
	BIND(Action::FromString("ZoomIn"), KEY::PG_UP);
	BIND(Action::FromString("ZoomOut"), KEY::PG_DOWN);
	BIND(Action::FromString("NewGame"), List<int>(KEY::N, KEY::G));
	BIND(Action::FromString("ClearLevel"), List<int>(KEY::C, KEY::L));
	BIND(Action::FromString("ListEntitiesAndRegistrations"), List<int>(KEY::L, KEY::E));
	BIND(Action::FromString("ToggleBlackness"), List<int>(KEY::T, KEY::B));
	BIND(Action::FromString("NextLevel"), List<int>(KEY::N, KEY::L));
	BIND(Action::FromString("PreviousLevel"), List<int>(KEY::P, KEY::L));
	BIND(Action::FromString("ToggleMenu"), KEY::ESCAPE);
	BIND(Action::CreateStartStopAction("Shooting"), KEY::SPACE);
	BIND(Action::FromString("OpenJumpDialog"), List<int>(KEY::CTRL, KEY::G));

	SpaceShooter2D::CreateDefaultBindings();
}

void PlayingLevel::Process(int timeInMs) {

	timeInMs = int(timeInMs * gameSpeed);

	now = Time::Now();
	timeElapsedMs += timeInMs;
	hudUpdateMs += timeInMs;

	SleepThread(5); // Updates 200 times a sec max?

	TutorialTests::Update(timeInMs);

	if (autoProceedMessages) {
		static Random random;
		if (level.activeLevelMessage != nullptr && random.Randf(1.0f) > 0.95f) {
			MesMan.QueueMessages("ProceedMessage");
			SleepThread(50); // spam less.
		}
	}

	if (paused)
		return;

	Cleanup();

	// Check for game over.
	if (CheckForGameOver(timeInMs))
		return;

	level.Process(*this, timeInMs);

	if (level.LevelCleared(levelTime, shipEntities, PlayerShips())) {
		if (playtestingEditorLevel)
			SetMode(SSGameMode::LEVEL_EDITOR);
		else {
			/// Clearing the level
			MesMan.QueueMessages("GoToHangar");
			return;
		}
	}

	// Update > 30 fps if possible
	if (hudUpdateMs > 10) {
		HUD* hud = HUD::Get();
		hud->UpdateActiveWeapon();
		hud->UpdateCooldowns(); // Update HUD 10 times a sec.
		// Update Debug once every 10 HUD updates?
		++hudUpdates;
		if (hudUpdates > 10) {
			hud->UpdateDebug();
			hudUpdates = 0;
		}
		hudUpdateMs = 0;
	}
	UpdateRenderArrows();

}

List<Ship*> PlayingLevel::PlayerShips()
{
	List<Ship*> playerShips;
	playerShips.AddItem(playerShip);
	return playerShips;
}


// If true, game over is pending/checking respawn conditions, don't process other messages.
bool PlayingLevel::CheckForGameOver(int timeInMs) {
	if (!playerShip->destroyed)
		return false;

	timeDeadMs += timeInMs;
	if (timeDeadMs < 2000)
		return true;

	LogMain("Game over! Player HP 0", INFO);

	if (playtestingEditorLevel) {
		SetMode(SSGameMode::LEVEL_EDITOR);
		return true;
	}

	// Game OVER!
	if (onDeath.Length() == 0) {
		spaceShooter->GameOver();
	}
	else if (onDeath.StartsWith("RespawnAt"))
	{
		playerShip->hp = (float)playerShip->maxHP;
		SpawnPlayer();
		// Reset level-time.
		String timeStr = onDeath.Tokenize("()")[1];
		Time time;
		if (timeStr == "RewindPoint")
			time = rewindPoint;
		else
			time = ParseTimeFrom(timeStr);
		SetTime(time);
	}
	else
		std::cout << "\nBad Game over (onDeath) critera.";
	return true;
}

void PlayingLevel::OnPlayerDied() {
	QueueGraphics(new GMSetCamera(levelCamera, CT_POSITION, Vector3f(0,0,10) + playerShip->entity->worldPosition));
	QueueGraphics(new GMSetCamera(levelCamera, CT_ENTITY_TO_TRACK, (Entity*) nullptr));
	QueueGraphics(new GMSetCamera(levelCamera, CT_SMOOTHING, 0.95f));
	failedToSurvive = true;
}

void PlayingLevel::OnPlayerSpawned() {
	levelCamera->trackingPositionOffset = Vector3f(10.f, 0, 0);
	QueueGraphics(new GMSetCamera(levelCamera, CT_POSITION, Vector3f(0,0,10)));
	QueueGraphics(new GMSetCamera(levelCamera, CT_ENTITY_TO_TRACK, playerShip->entity));
	QueueGraphics(new GMSetCamera(levelCamera, CT_SMOOTHING, 0.05f));

	timeDeadMs = 0;
}

/// Called from the render-thread for every viewport/AppWindow, after the main rendering-pipeline has done its job.
void PlayingLevel::Render(GraphicsState* graphicsState)
{
	if (!levelEntity)
		return;
	RenderInLevel(graphicsState);
}


void PlayingLevel::OnExit(AppState* nextState) {

	// Mark the level as completed. Mark times completed as increased if done multiple times.
	if (currentMission != nullptr) {
		int& timesCompleted = TimesCompleted(currentMission->name);
		if (timesCompleted == 0) {
			FirstTimeCompletion() = currentMission->name;
			RepeatCompletion() = "";
		}
		else {
			FirstTimeCompletion() = "";
			RepeatCompletion() = currentMission->name;
		}
		++timesCompleted;
		FlyTime() += this->flyTime;
		SetHighscore(currentMission->name, score);
		// Save weapons based on currently equipped ones (including damage taken, stats or other stuff)
		playerShip->SaveGearToVars();
		// Autosave!
		SaveFile::AutoSave(Application::name, PlayerName() + " " + DifficultyString(difficulty->GetInt()) + " " + FlyTime().ToString("H:m"));
	}


	level.Clear(*this);

	MapMan.DeleteAllEntities();

	HUD::Get()->Hide();

	levelEntity = NULL;
	playerShip = nullptr;

	SleepThread(50);
	// Register it for rendering.
	Graphics.QueueMessage(new GMUnregisterParticleSystem(sparks, true));
	ClearStars();
	MapMan.DeleteAllEntities();
	SleepThread(100);
}


/// Callback function that will be triggered via the MessageManager when messages are processed.
void PlayingLevel::ProcessMessage(Message* message)
{
	String msg = message->msg;
	if (mode == SSGameMode::EDIT_WEAPON_SWITCH_SCRIPTS)
		ProcessMessageWSS(message);
	level.ProcessMessage(*this, message);

	HUD::Get()->ProcessMessage(message);

	switch (message->type)
	{
	case MessageType::GAMEPAD_MESSAGE: {
		GamepadMessage * gamepadMessage = (GamepadMessage*)message;
		Gamepad state = gamepadMessage->gamepadState;

		if (gamepadMessage->aButtonPressed)
			level.ProceedMessage();

		if (!playerShip->weaponScriptActive)
			playerShip->shoot = state.rightTrigger > 0.5f;
		if (state.leftStick.MaxPart() < 0.15f) // Make it 0 if near it to avoid unwanted drift
			state.leftStick = Vector2f(0,0);
		if (state.leftStick.Length() > 0.90) // Make it 1.0 if near it
			state.leftStick.Normalize();
		requestedMovement = state.leftStick;
		UpdatePlayerVelocity();
		if (gamepadMessage->xButtonPressed)
			playerShip->weaponScriptActive = !playerShip->weaponScriptActive;
		if (gamepadMessage->yButtonPressed)
			playerShip->ActivateSkill();
		if (gamepadMessage->leftButtonPressed)
			playerShip->SwitchToWeapon(playerShip->CurrentWeaponIndex() - 1);
		if (gamepadMessage->rightButtonPressed)
			playerShip->SwitchToWeapon(playerShip->CurrentWeaponIndex() + 1);

		// Only with menu open.
//		if (HUD::Get()->InGameMenuOpen()) {
	//	}
		// Only with menu closed.
		//else {
		
		//}

		if (gamepadMessage->gamepadState.menuButtonPressed && !gamepadMessage->previousState.menuButtonPressed)
			ToggleInGameMenu();

		SpaceShooter2D::ProcessMessage(message);
		break;
	}
	case MessageType::COLLISSION_CALLBACK:
	{
		HandleCollision(playerShip, shipEntities, (CollisionCallback*)message);
		break;
	}
	case MessageType::INTEGER_MESSAGE:
	{
		IntegerMessage* im = (IntegerMessage*)message;
		if (msg == "SetActiveWeapon")
		{
			playerShip->SwitchToWeapon(im->value);
		}
		if (msg == "SetTestEnemiesAmount")
		{
			testGroup.number = im->value;
		}
		break;
	}

	case MessageType::SET_STRING:
	{
		String value = ((SetStringMessage*)message)->value;
		if (msg == "DropDownMenuSelection:ShipTypeToSpawn")
		{
			testGroup.shipType = value;
		}
		else if (msg == "DropDownMenuSelection:SpawnFormation")
		{
			testGroup.formation = GetFormationByName(value);
		}
		else if (msg == "JumpToTime") {
			JumpToTime(value);
		}
		break;
	}
	case MessageType::VECTOR_MESSAGE:
	{
		VectorMessage * vm = (VectorMessage*)message;
		if (msg == "SetFormationSize")
		{
			testGroup.size = vm->GetVector2f();
		}
		else if (msg == "SetSpawnLocation")
		{
			testGroup.position = vm->GetVector3f();
		}
		break;
	}
	case MessageType::STRING:
	{
		msg.RemoveSurroundingWhitespaces();
		int found = msg.Find("//");
		if (found > 0)
			msg = msg.Part(0, found);
		
		if (false) {}
		else if (msg == "AutoAim") {
			bool wasOn = playerShip->AutoAim();
			playerShip->SetAutoAim(!wasOn);
			autoProceedMessages = !wasOn;
		}
		else if (msg == "AutoProceedMessages") {
			autoProceedMessages = !autoProceedMessages;
		}
		else if (msg == "SpeedUp") {
			gameSpeed += 1;
			QueuePhysics(new PMSet(PT_SIMULATION_SPEED, gameSpeed));
		}
		else if (msg == "SpeedDown") {
			gameSpeed -= 1;
			if (gameSpeed < 1.0f)
				gameSpeed = 1.0f;
			QueuePhysics(new PMSet(PT_SIMULATION_SPEED, gameSpeed));
		}
		else if (msg.StartsWith("SetInt")) {
			String name = msg.Tokenize("(,)")[1];
			String value = msg.Tokenize("(,)")[2];
			GameVars.SetInt(name, value.ParseInt());
		}
		else if (msg.StartsWith("SetMessagesPauseGameTime")) {
			bool arg = msg.Tokenize("()")[1].ParseBool();
			level.messagesPauseGameTime = arg;
		}
		else if (msg.StartsWith("SetSpawnGroupsPauseGameTime")) {
			bool arg = msg.Tokenize("()")[1].ParseBool();
			level.spawnGroupsPauseGameTime = arg;
		}
		else if (msg == "ZoomIn") {
			if (levelCamera->TargetZoom() <= levelCamera->trackingPositionOffset.Length())
				return;
			QueueGraphics(new GMSetCamera(levelCamera, CT_ZOOM, levelCamera->TargetZoom() - 3.0f));
		}
		else if (msg == "ZoomOut") {
			if (levelCamera->TargetZoom() > 30)
				return;
			QueueGraphics(new GMSetCamera(levelCamera, CT_ZOOM, levelCamera->TargetZoom() + 3.0f));
		}
		else if (msg.StartsWith("SetLevelTimeToMessage")) {
			String msgName = msg.Tokenize(":")[1];
			List<String> messageNames;
			for (int i = 0; i < level.Messages().Size(); ++i) {
				auto msg = level.Messages()[i];
				if (msg->name.Length() > 0)
					messageNames.Add("- "+ msg->name);
				if (msg->name == msgName) {
					SetTime(msg->startTime);
					return;
				}
			}
			LogMain("No level message with name to jump to: " + msgName+"\nFull list of level messages: "+MergeLines(messageNames), ERROR);
			assert(false && "No level message by name");
		}
		else if (msg == "SetRewindPoint") {
			rewindPoint = levelTime;
		}
		else if (msg == "RestartLevel") {
			levelTime = flyTime = Time(TimeType::MILLISECONDS_NO_CALENDER, 0);
			level.OnLevelTimeAdjusted(levelTime);
			// Reload weapons
			LoadWeapons();
		}
		else if (msg == "AbortMission") {
			SetMode(SSGameMode::IN_HANGAR);
		}
		else if (msg.StartsWith("AddLevelScoreToTotal")) {
			// Add level score to total upon showing level stats. o.o
			// score += LevelScore()->iValue;
		}
		else if (msg == "SetupForTesting")
		{
			File::ClearFile(SPAWNED_ENEMIES_LOG);
			// Disable game-over/dying/winning
			level.Clear(*this);
			levelTime.intervals = 0;
		}
		else if (msg == "SpawnTestEnemies")
		{
			storedTestGroups.AddItem(testGroup);
			//				QueueGraphics(new GMSetUIs("StoreTestEnemies", GMUI::TEXT, "Store for spawning", ));
			for (int i = 0; i < storedTestGroups.Size(); ++i)
			{
				SpawnGroup sg = storedTestGroups[i];
				String str = sg.GetLevelCreationString(flyTime);
				File::AppendToFile(SPAWNED_ENEMIES_LOG, str);
				LogMain(str, INFO);
				sg.Spawn(levelTime, playerShip, nullptr);
			}
			storedTestGroups.Clear();
		}
		else if (msg.StartsWith("ShipTypeToSpawn:"))
		{
			testGroup.shipType = msg.Tokenize(":")[1];
		}
		else if (msg == "StoreTestEnemies")
		{
			storedTestGroups.AddItem(testGroup);
			//			QueueGraphics(new GMSetUIs("StoreTestEnemies", GMUI::TEXT, "Store for spawning ("+String(storedTestGroups.Size())+")"));
		}

		else if (msg == "OnReloadUI") {
			CreateUserInterface();
			HUD::Get()->Show();
		}
		else if (msg == "ProceedMessage")
		{
			level.ProceedMessage();
		}
		else if (msg == "SpeedDebrisCollected") {
			++spaceDebrisCollected;
		}

		if (msg == "TogglePlayerInvulnerability")
		{
			playerInvulnerability = !playerInvulnerability;
			OnPlayerInvulnerabilityUpdated();
		}
		if (msg.StartsWith("SetOnDeath:"))
		{
			onDeath = msg - "SetOnDeath:";
		}
		if (msg == "OpenJumpDialog")
		{
			Pause();
			OpenJumpDialog();
		}
		if (msg == "LoadWeapons") {
			LoadWeapons();
		}
		if (msg == "ReloadWeapon")
		{
			if (playerShip->activeWeapon == nullptr)
				return;
			playerShip->activeWeapon->QueueReload();
		}
		else if (msg == "ActiveWeaponsShown")
		{
			HUD::Get()->activeWeaponsShown = true;
			HUD::Get()->UpdateHUDGearedWeapons();
		}
		else if (msg == "ActiveWeaponsHidden")
			HUD::Get()->activeWeaponsShown = false;
		if (msg.Contains("StartMoveShip"))
		{
			String dirStr = msg - "StartMoveShip";
			Direction dir = GetDirection(dirStr);
			Vector3f dirVec = GetVector(dir);
			switch (dir)
			{
			case Direction::UP: case Direction::DOWN:
				requestedMovement.y = dirVec.y;
				break;
			case Direction::LEFT: case Direction::RIGHT:
				requestedMovement.x = dirVec.x;
				break;
			default:
				assert(false);
			}
			UpdatePlayerVelocity();
		}
		else if (msg.Contains("StopMoveShip"))
		{
			String dirStr = msg - "StopMoveShip";
			Direction dir = GetDirection(dirStr);
			switch (dir)
			{
			case Direction::UP: case Direction::DOWN:
				requestedMovement.y = 0;
				break;
			case Direction::LEFT: case Direction::RIGHT:
				requestedMovement.x = 0;
				break;
			default:
				assert(false);
			}
			UpdatePlayerVelocity();
		}
		else if (msg == "ActivateSkill")
		{
			playerShip->ActivateSkill();
		}
		//			std::cout<<"\n"<<msg;
		if (msg == "TutorialBaseGun")
		{
			playerShip->UnequipWeapons();
			playerShip->Equip(Gear::StartingWeapon());
			LoadWeapons();
		}
		if (msg == "TutorialLevel1Weapons")
		{
			playerShip->EquipTutorialLevel1Weapons();
			LoadWeapons();

//			playerShip->SetWeaponLevel(Weapon::Type::MachineGun, 1);
	//		playerShip->SetWeaponLevel(Weapon::Type::SmallRockets, 1);
		//	playerShip->SetWeaponLevel(Weapon::Type::BigRockets, 1);
			//HUD::Get()->UpdateHUDGearedWeapons();
		}
		if (msg == "TutorialLevel3Weapons")
		{
			playerShip->EquipTutorialLevel3Weapons();
			LoadWeapons();

//			playerShip->SetWeaponLevel(Weapon::Type::MachineGun, 3);
	///		playerShip->SetWeaponLevel(Weapon::Type::SmallRockets, 3);
		//	playerShip->SetWeaponLevel(Weapon::Type::BigRockets, 3);
			//HUD::Get()->UpdateHUDGearedWeapons();
		}
		if (msg.StartsWith("DecreaseWeaponLevel:"))
		{
			assert(false);

			//List<String> parts = msg.Tokenize(":");
			//int weaponIndex = parts[1].ParseInt();
			//Weapon* weap = playerShip->GetWeapon(WeaponType::TYPE_0 + weaponIndex);
			//int currLevel = weap->level;
			//playerShip->SetWeaponLevel(WeaponType::TYPE_0 + weaponIndex, currLevel - 1);
			//std::cout << "\nWeapon " << weap->type << " set to level " << weap->level << ": " << weap->name;
		}
		if (msg.StartsWith("IncreaseWeaponLevel:"))
		{
			assert(false);
			//List<String> parts = msg.Tokenize(":");
			//int weaponIndex = parts[1].ParseInt();
			//Weapon* weap = playerShip->GetWeapon(WeaponType::TYPE_0 + weaponIndex);
			//int currLevel = weap->level;
			//playerShip->SetWeaponLevel(WeaponType::TYPE_0 + weaponIndex, currLevel + 1);
			//std::cout << "\nWeapon " << weap->type << " set to level " << weap->level << ": " << weap->name;
		}
		else if (msg == "AllTheWeapons")
		{
			assert(false);

			//for (int i = 0; i < WeaponType::MAX_TYPES; ++i)
			//{
			//	if (playerShip->weapons[i]->level <= 0)
			//		playerShip->SetWeaponLevel(i, 1);
			//}
		}
		if (msg == "ToggleWeaponScript")
		{
			if (playerShip->weaponScript == 0)
				playerShip->weaponScript = WeaponScript::LastEdited();
			playerShip->weaponScriptActive = !playerShip->weaponScriptActive;
			if (!playerShip->weaponScriptActive) {
				playerShip->shoot = false;
			}
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
		if (msg == "SpawnTutorialBomb")
		{
			// Shoot.
			Color color = Vector4f(0.8f, 0.7f, 0.1f, 1.f);
			Texture* tex = TexMan.GetTextureByColor(color);
			Entity* projectileEntity = EntityMan.CreateEntity(name + " Projectile", ModelMan.GetModel("sphere.obj"), tex);
			Weapon weapon;
			weapon.damage = 500;
			weapon.lifeTimeMs = 100000;
			ProjectileProperty* projProp = new ProjectileProperty(weapon, projectileEntity, true);
			projectileEntity->properties.Add(projProp);
			// Set scale and position.
			projectileEntity->localPosition = playerShip->entity->worldPosition + Vector3f(30, 0, 0);
			projectileEntity->SetScale(Vector3f(1, 1, 1) * 0.5f);
			projProp->color = color;
			projectileEntity->RecalculateMatrix();
			projProp->onCollisionMessage = "ResumeGameTime";
			// pew
			Vector3f dir(-1.f, 0, 0);
			Vector3f vel = dir * 5.f;
			PhysicsProperty* pp = projectileEntity->physics = new PhysicsProperty();
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
		else if (msg.StartsWith("Weapon:"))
		{
			int weaponIndex = msg.Tokenize(":")[1].ParseInt();
			weaponIndex -= 1;
			if (weaponIndex < 0)
				weaponIndex = 9;
			playerShip->SwitchToWeapon(weaponIndex);
		}
		else if (msg == "UpdateHUDGearedWeapons")
			HUD::Get()->UpdateHUDGearedWeapons();
		else if (msg == "StartShooting")
		{
			playerShip->shoot = true;
			HUD::Get()->UpdateHUDGearedWeaponsIfNeeded();
		}
		else if (msg == "StopShooting")
		{
			playerShip->shoot = false;
		}

		else if (msg.StartsWith("LevelToLoad:"))
		{
			String source = msg;
			source.Remove("LevelToLoad:");
			source.RemoveSurroundingWhitespaces();
			levelToLoad = source;
		}
		else if (msg.StartsWith("LoadLevel:"))
		{
			String source = msg;
			source.Remove("LoadLevel:");
			source.RemoveSurroundingWhitespaces();
			if (!source.Contains("Levels"))
				source = "Levels/" + source;
			if (!source.Contains(".srl"))
				source += ".srl";
			LoadLevel(source, nullptr);
		}

		else if (msg == "ReloadLevel")
		{
			LoadLevel(level.source, currentMission);
		}
		else if (msg == "ClearLevel")
		{
			for (int i = 0; i < level.ships.Size(); ++i)
			{
				level.ships[i]->spawned = true;
				level.ships[i]->destroyed = true;
			}
			MapMan.DeleteEntities(shipEntities);
			// Move the level-entity, the player will follow.
			LevelEntity->MoveTo(Vector3f(level.goalPosition - 5.f, 10, 0));
			//				GraphicsMan.QueueMessage(new GMSetCamera(levelCamera, CT_POSITION, Vector3f(level.goalPosition - 5.f, 10, 0)));
		}
		else if (msg == "FinishStage")
		{
			ScriptMan.PlayScript("scripts/FinishStage.txt");
		}
		if (msg.StartsWith("DisplayCenterText"))
		{
			String text = msg;
			text.Remove("DisplayCenterText");
			text.RemoveInitialWhitespaces();
			GraphicsMan.QueueMessage(new GMSetUIs("CenterText", GMUI::TEXT, text));
		}
		else if (msg == "ClearCenterText")
			GraphicsMan.QueueMessage(new GMSetUIs("CenterText", GMUI::TEXT, Text()));

		if (msg == "Continue")
		{
			assert(false);
			/*
			if (levelToLoad.Length())
			{
				LoadLevel(levelToLoad);
				return;
			}
			LoadLevel();
			ScriptMan.PlayScript("scripts/NewGame.txt");
			Resume();
			*/
		}
		else if (msg == "InGameMenuPopped") {
			Resume();
		}
		else if (msg == "ToggleMenu")
		{
			ToggleInGameMenu();
		}
		else if (msg == "ToggleBlackness")
		{
			LevelEntity->ToggleBlackness();
		}

	}
	}
	// If not returned, process general messages from parent app state.
	ProcessGeneralMessage(message);
}

void PlayingLevel::LoadWeapons() {
	// Um.. load the weapons..?
	List<Gear> equippedWeapons = playerShip->EquippedWeapons();
	playerShip->weaponSet.ClearAndDelete();
	for (int i = 0; i < equippedWeapons.Size(); ++i) {
		Gear equippedWeapon = equippedWeapons[i];
		Weapon * weapon = new Weapon();
		bool ok = Weapon::Get(equippedWeapon.name, weapon);
		if (!ok) {
			LogMain("Failed to load weapon by name: " + equippedWeapon.name, ERROR);
			delete weapon;
			continue;
		}
		playerShip->weaponSet.Add(weapon);
	}
	LogMain("Weapons locked and loaded", INFO);
	HUD::Get()->UpdateHUDGearedWeapons();

}

void PlayingLevel::ToggleInGameMenu() {
	// Pause the game.
	if (!paused)
	{
		if (playtestingEditorLevel) {
			// Return back to editor straight away.
			SetMode(SSGameMode::LEVEL_EDITOR);
			return;
		}
		Pause();
		// Bring up the in-game menu.
		HUD::Get()->OpenInGameMenu();
		InputMan.SetNavigateUI(true);
	}
	else
	{
		HUD::Get()->CloseInGameMenu();
		InputMan.SetNavigateUI(false);
		UpdateUI();
		Resume();
	}
}

void PlayingLevel::Cleanup()
{
	float despawnDown = LevelEntity->DespawnDown();
	float despawnUp = LevelEntity->DespawnUp();
	/// Remove projectiles which have been passed by.
	for (int i = 0; i < projectileEntities.Size(); ++i)
	{
		Entity* proj = projectileEntities[i];
		ProjectileProperty* pp = (ProjectileProperty*)proj->GetProperty(ProjectileProperty::ID());
		if (pp->sleeping ||
			(proj->worldPosition[0] < despawnPositionLeft ||
				proj->worldPosition[0] > despawnPositionRight ||
				proj->worldPosition.y < despawnDown ||
				proj->worldPosition.y > despawnUp
				)
			)
		{
			if (pp->enemy) {
				projectilesDodged.Add(true);
				UpdateEnemyProjectilesDodgedString();
			}
			MapMan.DeleteEntity(proj);
			projectileEntities.Remove(proj);
			--i;
		}
	}

	/// Clean ships.
	for (int i = 0; i < level.ships.Size(); ++i)
	{
		Ship* ship = level.ships[i];
		if (ship->destroyed || !ship->spawned)
		{
			level.ships.RemoveItem(ship); // Remove it. Remove links to spawn-group too.
			ship = nullptr;
			continue;
		}
		if (!ship->spawned)
			continue;
		if (!ship->entity)
			continue;
		// any with fucked up position? remove it.
		if (ship->entity->worldPosition.x != ship->entity->worldPosition.x)
		{
			std::cout << "Fuuucked up";
			PrintEntityData(ship->entity);
			// Remove?
			ship->Despawn(*this, false);
			continue;
		}
		// Check if it should de-spawn.
		if (ship->despawnOutsideFrame && ship->entity->worldPosition[0] < despawnPositionLeft && ship->parent == NULL)
		{
			ship->Despawn(*this, false);
		}
	}
}

void PlayingLevel::SetPlayerMovement(Vector3f inDirection) {
	requestedMovement = inDirection;
	UpdatePlayerVelocity();
}

void PlayingLevel::UpdatePlayerVelocity()
{
	Vector3f totalVec = requestedMovement;
	Vector3f requestedTotalVecBeforeLevelSpeed = totalVec;

	// Don't normalize if length is [0,1] for gamepad nuance.
	if (totalVec.LengthSquared() > 1)
		totalVec.Normalize();
	totalVec *= playerShip->Speed();
	totalVec *= (float) (playerShip->movementDisabled ? 0 : 1);
	totalVec += LevelEntity->Velocity();

	// Set player speed.
	if (playerShip->entity)
	{
		PhysicsMan.QueueMessage(new PMSetEntity(playerShip->entity, PT_VELOCITY, totalVec));
		if (requestedTotalVecBeforeLevelSpeed.LengthSquared() > 0)
			PhysicsMan.QueueMessage(new PMSetEntity(playerShip->entity, PT_LINEAR_DAMPING, 0.85f));
		else
			PhysicsMan.QueueMessage(new PMSetEntity(playerShip->entity, PT_LINEAR_DAMPING, 0.999f));
	}
}


/// Searches among actively spawned ships.
Ship* PlayingLevel::GetShip(Entity* forEntity) {
	for (int i = 0; i < level.ships.Size(); ++i)
	{
		Ship* ship = level.ships[i];
		if (ship->entity == forEntity)
			return ship;
	}
	return 0;
}

/// o.o
void PlayingLevel::NewPlayer()
{
	playerShip = new PlayerShip();
}

/// Loads target level. The source and separate .txt description have the same name, just different file-endings, e.g. "Level 1.png" and "Level 1.txt"
void PlayingLevel::LoadLevel(String fromSource, Mission * forMission)
{
	currentMission = forMission;
	lastSpawnGroup = nullptr;
	levelTime = flyTime = Time(TimeType::MILLISECONDS_NO_CALENDER, 0);

	// Reset stats for this specific level.
	LevelKills()->iValue = 0;
	score = 0;
	spaceDebrisCollected = 0;

	QueueGraphics(new GMSetUIb("LevelMessage", GMUI::VISIBILITY, false));


	// Delete all entities.
	MapMan.DeleteAllEntities();
	shipEntities.Clear();
	projectileEntities.Clear();

	GraphicsMan.PauseRendering();
	PhysicsMan.Pause();

	SleepThread(50);
	// Await all unregistrations..?

	HUD::Get()->HideLevelStats();
	HUD::Get()->CloseInGameMenu();

	String tutorialLevel = "Levels/Tutorial.srl";

	levelSource = fromSource;
	level = Level(); // Reset all vars
	bool success = level.Load(levelSource);
	if (!success) {
		LogMain("Unable to load level from source " + fromSource + ".", ERROR);
		return;
	}

	level.SetupCamera();
	if (!playerShip)
		NewPlayer();

	// Reset player cooldowns if needed.
	if (playerShip)
		playerShip->RandomizeWeaponCooldowns();


	/// Add entity to track for both the camera, blackness and player playing field.
	if (levelEntity == nullptr)
	{
		levelEntity = LevelEntity->Create(level.playingFieldSize, playingFieldPadding, true, &level);
		LevelEntity->SetVelocity(level.BaseVelocity());
	}

	/// Add emitter for stars at player start.
	ClearStars();
	NewStars(level.starSpeed.NormalizedCopy(), level.starSpeed.Length(), 0.2f, level.starColor);
	LetStarsTrack(levelEntity, Vector3f(level.playingFieldSize.x + 10.f, 0, 0));


	// Reset position of level entity if already created.
	// PhysicsMan.QueueMessage(new PMSetEntity(levelEntity, PT_POSITION, initialPosition));
	// Set velocity of the game.
	// Reset position of player!
	//	PhysicsMan.QueueMessage(new PMSetEntity(playerShip->entity, PT_POSITION, initialPosition));
	SpawnPlayer();

	// Reset player stats.
	playerShip->hp = (float) ( playerShip->maxHP);
	playerShip->shieldValue = playerShip->maxShieldValue;
	playerShip->entity->localPosition = Vector3f(-50, 0, 0);

	sparks->SetAlphaDecay(DecayType::QUADRATIC);

	GraphicsMan.ResumeRendering();
	PhysicsMan.Resume();
	level.OnEnter();
	// Run start script.
	ScriptMan.PlayScript("scripts/OnLevelStart.txt");

	// Update all UI now that a player is there.
	HUD::Get()->UpdateUI();

	// o.o
	this->Resume();

	QueueGraphics(new GMRecompileShaders()); // Shouldn't be needed...

	if (this->levelSource == tutorialLevel) {
		if (GameVars.Get("PlayTutorial")->iValue == 0) {
			// Jump in time..!
			LevelMessage * levelMessage = level.GetMessageWithTextId("TutorialConcluded");
			assert(levelMessage);
			JumpToAfterMessage(levelMessage);
		}
	}

}

void PlayingLevel::SpawnPlayer() {
	level.SpawnPlayer(*this, playerShip, Vector3f(levelEntity->worldPosition.x - level.playingFieldHalfSize.x, 0, 0));

}


void PlayingLevel::UpdateRenderArrows()
{
	Vector2f minField = levelEntity->worldPosition - level.playingFieldHalfSize - Vector2f(1,1);
	Vector2f maxField = levelEntity->worldPosition + level.playingFieldHalfSize + Vector2f(1,1);

	List<Vector3f> newPositions;
	for (int i = 0; i < shipEntities.Size(); ++i)
	{	
		// Grab the position
		Entity* e = shipEntities[i];
		Vector2f pos = e->worldPosition;
		// Check if outside boundary.
		if (pos > minField && pos < maxField)
		{
			continue; // Skip already visible ships.
		}
		newPositions.AddItem(pos);
		// Clamp the position.
		pos.Clamp(minField, maxField);
		newPositions.AddItem(pos);
	}
	QueueGraphics(new GMSetData(&renderPositions, newPositions)); 
}

//// Renders data updated via Render-thread.
void PlayingLevel::RenderInLevel(GraphicsState * graphicsState)
{
	Vector2f minField = levelEntity->worldPosition - level.playingFieldHalfSize - Vector2f(1,1);
	Vector2f maxField = levelEntity->worldPosition + level.playingFieldHalfSize + Vector2f(1,1);

	// Load default shader?
	ShadeMan.SetActiveShader(graphicsState, NULL);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(graphicsState->projectionMatrixD.getPointer());
	glMatrixMode(GL_MODELVIEW);
	Matrix4d modelView = graphicsState->viewMatrixD * graphicsState->modelMatrixD;
	glLoadMatrixd(modelView.getPointer());
	// Enable blending
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	float z = -4;
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	graphicsState->currentTexture = NULL;
	// Disable lighting
	glDisable(GL_LIGHTING);
	glDisable(GL_COLOR_MATERIAL);
	// Ignore previous stuff there.
	glDisable(GL_DEPTH_TEST);
	// Specifies how the red, green, blue and alpha source blending factors are computed
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	/// o.o
	for (int i = 0; i < renderPositions.Size(); i += 2)
	{	
		Vector3f shipPos = renderPositions[i];
		Vector3f clampedPos = renderPositions[i+1];

		// wat.
		clampedPos.Clamp(minField, maxField);

		// Check direction from this position to the entity's actual position.
		Vector3f to = (shipPos - clampedPos);
		float dist = to.Length();
		Vector3f dir = to.NormalizedCopy();
		Vector3f a,b,c;
		// Move the position a bit out...?
		Vector3f center = clampedPos;
		center.z = 7.f;
		// Center.
		a = b = c = center;
		// Move A away from the dir.
		a += dir * 0.7f;
		// Get side-dirs.
		Vector3f side = dir.CrossProduct(Vector3f(0,0,1)).NormalizedCopy();
		side *= 0.5f;
		b += side;
		c -= side;

		// Set color based on distance.
		float alpha = (1.f / dist) + 0.5f;
		glColor4f(1,1,1,alpha);

		// Draw stuff
		glBegin(GL_TRIANGLES);
	#define DRAW(a) glVertex3f(a.x,a.y,a.z)
			DRAW(a);
			DRAW(b);
			DRAW(c);
		glEnd();
	}
	glEnable(GL_DEPTH_TEST);
	CheckGLError("SpaceShooter2D::Render");
}


void PlayingLevel::JumpToAfterMessage(LevelMessage * message) {
	Time time = message->startTime;
	time.AddSeconds(1);
	JumpToTime(time);
}

void PlayingLevel::JumpToTime(String timeString)
{
	Time time(TimeType::MILLISECONDS_NO_CALENDER);
	time.ParseFrom(timeString);
	JumpToTime(time);
}

void PlayingLevel::JumpToTime(Time time) {
	// Jump to target level-time. Adjust position if needed.
	levelTime = time;
	HideLevelMessage();
	level.SetSpawnGroupsFinishedAndDefeated(levelTime);
	level.OnLevelTimeAdjusted(levelTime);
	level.activeLevelMessage = nullptr;
	playerShip->movementDisabled = false;
	gameTimePausedDueToScriptableMessage = false;
}


Time PlayingLevel::SetTime(Time time) {
	assert(time.Type() != TimeType::UNDEFINED);
	levelTime = time;
	level.OnLevelTimeAdjusted(levelTime);
	return levelTime;
}

void PlayingLevel::HideLevelMessage() {
	level.HideLevelMessage(nullptr);
}

#include "Level/SpawnGroup.h"

bool PlayingLevel::GameTimePausedDueToActiveSpawnGroup() {
	return level.spawnGroupsPauseGameTime && level.SpawnGroupsActive() > 0;
}

bool PlayingLevel::GameTimePausedDueToActiveLevelMessage() {
	return level.messagesPauseGameTime && level.activeLevelMessage != nullptr;
}

bool PlayingLevel::GameTimePaused() {
	return GameTimePausedDueToActiveSpawnGroup() || GameTimePausedDueToActiveLevelMessage() || gameTimePausedDueToScriptableMessage;
}

bool PlayingLevel::DefeatedAllEnemiesInTheLastSpawnGroup() {
	if (lastSpawnGroup == nullptr)
		return true;
	if (lastSpawnGroup->ShipsDefeated())
		return true;
	return false;
}

void PlayingLevel::SetLastSpawnGroup(SpawnGroup * sg) {
	lastSpawnGroup = sg;
}

void PlayingLevel::UpdateEnemyProjectilesDodgedString() {
	if (projectilesDodged.Size() > 100) {
		projectilesDodged.RemoveIndex(0, ListOption::RETAIN_ORDER);
	}
	int dodged = 0;
	for (int i = 0; i < projectilesDodged.Size(); ++i)
		dodged += projectilesDodged[i] ? 1 : 0;
	int total = projectilesDodged.Size();
	enemyProjectilesDodgedString = String(dodged) + " / " + String(total);
}




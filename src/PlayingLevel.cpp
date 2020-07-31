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

#include "Level/SpawnGroup.h"

SpawnGroup testGroup;
List<SpawnGroup> storedTestGroups;


/// Each other being original position, clamped position, orig, clamp, orig3, clamp3, etc.
List<Vector3f> renderPositions;
bool playerInvulnerability = false;
void OnPlayerInvulnerabilityUpdated()
{

}

ShipPtr PlayerShip() {
	return PlayingLevel::playerShip;
}
EntitySharedPtr PlayerShipEntity() {
	return PlayerShip()->entity;
}
PlayingLevel& PlayingLevelRef() {
	return *PlayingLevel::singleton;
}


ShipPtr PlayingLevel::playerShip = nullptr;
EntitySharedPtr PlayingLevel::levelEntity = nullptr;

GameVariable* SpaceShooter2D::currentLevel = nullptr,
* SpaceShooter2D::currentStage = nullptr,
* SpaceShooter2D::score = nullptr,
* SpaceShooter2D::money = nullptr,
* SpaceShooter2D::playTime = nullptr,
* SpaceShooter2D::playerName = nullptr,
* SpaceShooter2D::gameStartDate = nullptr,
* SpaceShooter2D::difficulty = nullptr;


PlayingLevel* PlayingLevel::singleton = nullptr;


Vector2f PlayingLevel::playingFieldHalfSize = Vector2f(),
PlayingLevel::playingFieldSize = Vector2f();

PlayingLevel::PlayingLevel()
	: SpaceShooter2D()
	, lastSpawnGroup(nullptr)
{
	assert(singleton == nullptr);
	singleton = this;
}

ShipPtr PlayingLevel::GetShipByID(int id)
{
	for (int i = 0; i < level.ships.Size(); ++i)
	{
		ShipPtr ship = level.ships[i];
		if (ship->ID() == id)
			return ship;
	}
	if (id == playerShip->ID())
		return playerShip;
	return 0;
}


void PlayingLevel::SetPlayingFieldSize(Vector2f newSize)
{
	playingFieldSize = newSize;
	playingFieldHalfSize = newSize * .5f;
}

/// UI stuffs. All implemented in UIHandling.cpp
void PlayingLevel::UpdateUI() {
	HUD::Get()->UpdateUI();
}

// Inherited via AppState
void PlayingLevel::OnEnter(AppState* previousState) {

	assert(playerName.Length() > 0);

	testGroup.number = 1;

	/// Enable Input-UI navigation via arrow-keys and Enter/Esc.
	InputMan.ForceNavigateUI(false);


	SetPlayingFieldSize(Vector2f(50, 30));

	HUD::Get()->Show();

	if (mode == PLAYING_LEVEL)
		OpenSpawnWindow();
	else
		CloseSpawnWindow();

	NewPlayer();

	// Create.. the sparks! o.o
// New global sparks system.
	sparks = (new Sparks(true))->GetSharedPtr();
	// Register it for rendering.
	Graphics.QueueMessage(new GMRegisterParticleSystem(sparks, true));

	// Load level upon entering.
	LoadLevel();

	// Play script for animation or whatever.
	ScriptMan.PlayScript("scripts/NewGame.txt");
	// Resume physics/graphics if paused.
	Resume();

	TextMan.LoadFromDir();
	TextMan.SetLanguage("English");

};

void PlayingLevel::Process(int timeInMs) {

	now = Time::Now();
	timeElapsedMs += timeInMs;
	hudUpdateMs += timeInMs;

	SleepThread(5); // Updates 200 times a sec max?

	if (paused)
		return;

	Cleanup();

	// Check for game over.
	if (CheckForGameOver(timeInMs))
		return;

	level.Process(*this, timeInMs);
	// Update > 30 fps if possible
	if (hudUpdateMs > 10) {
		HUD* hud = HUD::Get();
		hud->UpdateActiveWeapon();
		hud->UpdateCooldowns(); // Update HUD 10 times a sec.
		hud->UpdateDebug();
		hudUpdateMs = 0;
	}
	UpdateRenderArrows();

}

// If true, game over is pending/checking respawn conditions, don't process other messages.
bool PlayingLevel::CheckForGameOver(int timeInMs) {
	if (!playerShip->destroyed)
		return false;

	timeDeadMs += timeInMs;
	if (timeDeadMs < 2000)
		return true;

	LogMain("Game over! Player HP 0", INFO);
	// Game OVER!
	if (onDeath.Length() == 0)
		spaceShooter->GameOver();
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
	QueueGraphics(new GMSetCamera(levelCamera, CT_ENTITY_TO_TRACK, (EntitySharedPtr) nullptr));
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

	HUD::Get()->Hide();

	levelEntity = NULL;
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
		else if (msg == "SetRewindPoint") {
			rewindPoint = levelTime;
		}
		else if (msg == "RestartLevel") {
			levelTime = flyTime = Time(TimeType::MILLISECONDS_NO_CALENDER, 0);
			level.OnLevelTimeAdjusted(levelTime);
		}
		else if (msg.StartsWith("AddLevelScoreToTotal")) {
			// Add level score to total upon showing level stats. o.o
			score->iValue += LevelScore()->iValue;
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
				sg.Spawn(PlayingLevelRef());
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
			movementDirections.Add(dir);
			UpdatePlayerVelocity();
		}
		else if (msg.Contains("StopMoveShip"))
		{
			String dirStr = msg - "StopMoveShip";
			Direction dir = GetDirection(dirStr);
			while (movementDirections.Remove(dir));
			UpdatePlayerVelocity();
		}
		else if (msg == "ActivateSkill")
		{
			playerShip->ActivateSkill();
		}
		//			std::cout<<"\n"<<msg;
		if (msg == "TutorialBaseGun")
		{
			playerShip->SetLevelOfAllWeaponsTo(0);
			playerShip->activeWeapon = playerShip->SetWeaponLevel(Weapon::Type::MachineGun, 1);
		}
		if (msg == "TutorialLevel1Weapons")
		{
			playerShip->SetWeaponLevel(Weapon::Type::MachineGun, 1);
			playerShip->SetWeaponLevel(Weapon::Type::SmallRockets, 1);
			playerShip->SetWeaponLevel(Weapon::Type::BigRockets, 1);
		}
		if (msg == "TutorialLevel3Weapons")
		{
			playerShip->SetWeaponLevel(Weapon::Type::MachineGun, 3);
			playerShip->SetWeaponLevel(Weapon::Type::SmallRockets, 3);
			playerShip->SetWeaponLevel(Weapon::Type::BigRockets, 3);
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
			EntitySharedPtr projectileEntity = EntityMan.CreateEntity(name + " Projectile", ModelMan.GetModel("sphere.obj"), tex);
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
			LoadLevel(source);
		}

		else if (msg == "ReloadLevel")
		{
			LoadLevel(level.source);
		}

		else if (msg == "NextLevel")
		{
			// Next level? Restart?
			++currentLevel->iValue;
			if (currentLevel->iValue > 4)
			{
				currentLevel->iValue = 4;
			}
			LoadLevel();
		}
		else if (msg == "NextStage")
		{
			if (currentStage->iValue >= 8)
				return;
			++currentStage->iValue;
			currentLevel->iValue = 1;
			if (currentLevel->iValue > 8)
			{
				currentLevel->iValue = 8;
			}
			LoadLevel();
		}
		else if (msg == "PreviousLevel")
		{
			--currentLevel->iValue;
			if (currentLevel->iValue < 1)
			{
				std::cout << "\nCannot go to previous level, already at level 1. Try switching stage.";
				currentLevel->iValue = 1;
			}
			LoadLevel();
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
			/// Add munny o.o
			money->iValue += 1000 + (currentStage->iValue - 1) * 2000;
			ScriptMan.PlayScript("scripts/FinishStage.txt");
		}
		if (msg.StartsWith("DisplayCenterText"))
		{
			String text = msg;
			if (text.Contains("$-L"))
			{
				text.Replace("$-L", currentStage->ToString() + "-" + currentLevel->ToString());
			}
			text.Replace("Stage $", "Stage " + currentStage->ToString());
			text.Remove("DisplayCenterText");
			text.RemoveInitialWhitespaces();
			GraphicsMan.QueueMessage(new GMSetUIs("CenterText", GMUI::TEXT, text));
		}
		else if (msg == "ClearCenterText")
			GraphicsMan.QueueMessage(new GMSetUIs("CenterText", GMUI::TEXT, Text()));

		if (msg == "Continue")
		{
			if (levelToLoad.Length())
			{
				LoadLevel(levelToLoad);
				return;
			}
			// Set stage n level
			if (currentStage->iValue == 0)
			{
				currentStage->iValue = 1;
				currentLevel->iValue = 1;
			}
			else {
				currentLevel->iValue += 1;
				if (currentLevel->iValue == 4)
				{
					currentStage->iValue += 1;
					currentLevel->iValue = 1;
				}
			}
			// Load weapons?

			// And load it.
			LoadLevel();
			// Play script for animation or whatever.
			ScriptMan.PlayScript("scripts/NewGame.txt");
			// Resume physics/graphics if paused.
			Resume();
		}
		else if (msg == "InGameMenuPopped") {
			Resume();
		}
		else if (msg == "ToggleMenu")
		{
			// Pause the game.
			if (!paused)
			{
				Pause();
				// Bring up the in-game menu.
				HUD::Get()->OpenInGameMenu();
			}
			else
			{
				UpdateUI();
				Resume();
			}
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

void PlayingLevel::Cleanup()
{
	float despawnDown = LevelEntity->DespawnDown();
	float despawnUp = LevelEntity->DespawnUp();
	/// Remove projectiles which have been passed by.
	for (int i = 0; i < projectileEntities.Size(); ++i)
	{
		EntitySharedPtr proj = projectileEntities[i];
		ProjectileProperty* pp = (ProjectileProperty*)proj->GetProperty(ProjectileProperty::ID());
		if (pp->sleeping ||
			(proj->worldPosition[0] < despawnPositionLeft ||
				proj->worldPosition[0] > spawnPositionRight ||
				proj->worldPosition.y < despawnDown ||
				proj->worldPosition.y > despawnUp
				)
			)
		{
			MapMan.DeleteEntity(proj);
			projectileEntities.Remove(proj);
			--i;
		}
	}

	/// Clean ships.
	for (int i = 0; i < level.ships.Size(); ++i)
	{
		ShipPtr ship = level.ships[i];
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

void PlayingLevel::UpdatePlayerVelocity()
{
	Vector3f totalVec;
	for (int i = 0; i < movementDirections.Size(); ++i)
	{
		Vector3f vec = GetVector(movementDirections[i]);
		totalVec += vec;
	}
	Vector3f requestedTotalVecBeforeLevelSpeed = totalVec;

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
ShipPtr PlayingLevel::GetShip(EntitySharedPtr forEntity) {
	for (int i = 0; i < level.ships.Size(); ++i)
	{
		ShipPtr ship = level.ships[i];
		if (ship->entity == forEntity)
			return ship;
	}
	return 0;
}

/// o.o
void PlayingLevel::NewPlayer()
{
	if (!shipDataLoaded)
		LoadShipData();

	/// Creates new ship of specified type.
	

	playerShip = Ship::New("Default");

	playerShip->enemy = false;
	playerShip->allied = true;

	playerShip->weapons = WeaponSet();
	const Weapon * starterWeapon = Weapon::Get(Weapon::Type::MachineGun, 1);
	Weapon * weapon = new Weapon();
	*weapon = *starterWeapon;
	assert(starterWeapon != nullptr);
	playerShip->weapons.Add(weapon);

	playerShip->armor = Gear::StartingArmor();
	playerShip->shield = Gear::StartingShield();
	playerShip->UpdateStatsFromGear();

	playerShip->SetLevelOfAllWeaponsTo(0);

}

/// Loads target level. The source and separate .txt description have the same name, just different file-endings, e.g. "Level 1.png" and "Level 1.txt"
void PlayingLevel::LoadLevel(String fromSource)
{

	lastSpawnGroup = nullptr;
	levelTime = flyTime = Time(TimeType::MILLISECONDS_NO_CALENDER, 0);

	bool nonStandardLevel = false;
	if (fromSource != "CurrentStageLevel")
	{
		nonStandardLevel = true;
		currentStage->SetInt(999);
		currentLevel->SetInt(0);
	}
	// Reset stats for this specific level.
	LevelKills()->iValue = 0;
	LevelScore()->iValue = 0;
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

	if (fromSource == "CurrentStageLevel")
	{
		fromSource = "Levels/Stage " + currentStage->ToString() + "/Level " + currentStage->ToString() + "-" + currentLevel->ToString();
		if (currentStage->GetInt() == 0)
			fromSource = tutorialLevel;
		bool success = level.Load(fromSource);
		if (!success) {
			LogMain("Unable to load level from source "+fromSource+".", ERROR);
			return;
		}
	}
	this->levelSource = fromSource;

	level.SetupCamera();
	if (!playerShip)
		NewPlayer();

	// Reset player cooldowns if needed.
	if (playerShip)
		playerShip->RandomizeWeaponCooldowns();


	/// Add entity to track for both the camera, blackness and player playing field.
	if (levelEntity == nullptr)
	{
		levelEntity = LevelEntity->Create(playingFieldSize, playingFieldPadding, levelCamera);
		LevelEntity->SetVelocity(level.BaseVelocity());
	}

	/// Add emitter for stars at player start.
	ClearStars();
	NewStars(level.starSpeed.NormalizedCopy(), level.starSpeed.Length(), 0.2f, level.starColor);
	LetStarsTrack(levelEntity, Vector3f(playingFieldSize.x + 10.f, 0, 0));


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

}

void PlayingLevel::SpawnPlayer() {
	level.SpawnPlayer(*this, playerShip, Vector3f(levelEntity->worldPosition.x - playingFieldHalfSize.x, 0, 0));

}


void PlayingLevel::UpdateRenderArrows()
{
	Vector2f minField = levelEntity->worldPosition - playingFieldHalfSize - Vector2f(1,1);
	Vector2f maxField = levelEntity->worldPosition + playingFieldHalfSize + Vector2f(1,1);

	List<Vector3f> newPositions;
	for (int i = 0; i < shipEntities.Size(); ++i)
	{	
		// Grab the position
		EntitySharedPtr e = shipEntities[i];
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
	Vector2f minField = levelEntity->worldPosition - playingFieldHalfSize - Vector2f(1,1);
	Vector2f maxField = levelEntity->worldPosition + playingFieldHalfSize + Vector2f(1,1);

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


void PlayingLevel::JumpToTime(String timeString)
{
	// Jump to target level-time. Adjust position if needed.
	levelTime.ParseFrom(timeString);
	HideLevelMessage();
	level.SetSpawnGroupsFinishedAndDefeated(levelTime);
	level.OnLevelTimeAdjusted(levelTime);
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
AppWindow* spawnWindow = 0;
UserInterface* spawnUI = 0;

void PlayingLevel::OpenSpawnWindow()
{
	if (!spawnWindow)
	{
		spawnWindow = WindowMan.NewWindow("SpawnWindow", "Spawn Window");
		spawnWindow->SetRequestedSize(Vector2i(400, 300));
		spawnWindow->Create();
		UserInterface* ui = spawnUI = spawnWindow->CreateUI();
		ui->Load("gui/SpawnWindow.gui");
	}
	spawnWindow->Show();
	/// Update lists inside.
	List<String> shipTypes;
	for (int i = 0; i < Ship::types.Size(); ++i)
	{
		ShipPtr type = Ship::types[i];
		if (type->allied)
			continue;
		shipTypes.AddItem(type->name);
	}
	QueueGraphics(new GMSetUIContents(spawnUI, "ShipTypeToSpawn", shipTypes));
	List<String> spawnFormations;
	for (int i = 0; i < (int)Formation::FORMATIONS; ++i)
	{
		spawnFormations.AddItem(GetName(Formation(i)));
	}
	QueueGraphics(new GMSetUIContents(spawnUI, "SpawnFormation", spawnFormations));
}

void PlayingLevel::CloseSpawnWindow()
{
	if (spawnWindow)
		spawnWindow->Close();
}


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




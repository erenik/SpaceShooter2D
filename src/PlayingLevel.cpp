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

/// Each other being original position, clamped position, orig, clamp, orig3, clamp3, etc.
List<Vector3f> renderPositions;

EntitySharedPtr LevelEntity() {
	return PlayingLevel::levelEntity;
}
Ship * PlayerShip() {
	return PlayingLevel::playerShip;
}
EntitySharedPtr PlayerShipEntity() {
	return PlayerShip()->entity;
}
PlayingLevel& PlayingLevelRef() {
	return *PlayingLevel::singleton;
}


Ship* PlayingLevel::playerShip = nullptr;
EntitySharedPtr PlayingLevel::levelEntity = nullptr;

GameVariable* SpaceShooter2D::currentLevel = nullptr,
	* SpaceShooter2D::currentStage = nullptr,
	* SpaceShooter2D::score = nullptr,
	* SpaceShooter2D::money = nullptr;



Vector2f PlayingLevel::playingFieldHalfSize = Vector2f(),
	playingFieldSize = Vector2f();

void PlayingLevel::SetPlayingFieldSize(Vector2f newSize)
{
	playingFieldSize = newSize;
	playingFieldHalfSize = newSize * .5f;
}


// Inherited via AppState
void PlayingLevel::OnEnter(AppState* previousState) {

	assert(singleton == nullptr);
	singleton = this;

	UpdateUI();
	String toPush = "gui/HUD.gui";
	PushUI(toPush);

	UpdateHUDGearedWeapons();
	UpdateUIPlayerHP(true);
	UpdateUIPlayerShield(true);

	if (inGameMenuOpened)
		PushUI("gui/InGameMenu.gui");
	else
		PopUI("gui/InGameMenu.gui");

	if (mode == PLAYING_LEVEL)
		OpenSpawnWindow();
	else
		CloseSpawnWindow();


	// Create.. the sparks! o.o
// New global sparks system.
	sparks = new Sparks(true);
	// Register it for rendering.
	Graphics.QueueMessage(new GMRegisterParticleSystem(sparks, true));

	stars = new Stars(true);
	stars->deleteEmittersOnDeletion = true;
	Graphics.QueueMessage(new GMRegisterParticleSystem(stars, true));

	/// Add emitter
	starEmitter = new StarEmitter(Vector3f());
	Graphics.QueueMessage(new GMAttachParticleEmitter(starEmitter, stars));

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
	timeElapsedMs = timeInMs;

	if (paused)
		return;

	Cleanup();

	level.Process(*this, timeInMs);
	UpdateCooldowns();
	UpdateRenderArrows();

}

void PlayingLevel::OnExit(AppState* nextState) {
		levelEntity = NULL;
	SleepThread(50);
	// Register it for rendering.
	Graphics.QueueMessage(new GMUnregisterParticleSystem(sparks, true));
	Graphics.QueueMessage(new GMUnregisterParticleSystem(stars, true));
	MapMan.DeleteAllEntities();
	SleepThread(100);

}


/// Callback function that will be triggered via the MessageManager when messages are processed.
void PlayingLevel::ProcessMessage(Message* message)
{
	String msg = message->msg;
	if (mode == SSGameMode::EDIT_WEAPON_SWITCH_SCRIPTS)
		ProcessMessageWSS(message);
	level.ProcessMessage(message);
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
		{
			showLevelStats = true;
			// Add level score to total upon showing level stats. o.o
			score->iValue += LevelScore()->iValue;
			ShowLevelStats();
		}
		else if (msg.StartsWith("HideLevelStats"))
		{
			showLevelStats = false;
			UpdateUI();
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
			PhysicsMan.QueueMessage(new PMSetEntity(levelEntity, PT_POSITION, Vector3f(level.goalPosition - 5.f, 10, 0)));
			//				GraphicsMan.QueueMessage(new GMSetCamera(levelCamera, CT_POSITION, Vector3f(level.goalPosition - 5.f, 10, 0)));
		}
		else if (msg == "FinishStage")
		{
			/// Add munny o.o
			money->iValue += 1000 + (currentStage->iValue - 1) * 2000;
			ScriptMan.PlayScript("scripts/FinishStage.txt");
		}
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
		else if (msg == "ToggleMenu")
		{
			if (mode != PLAYING_LEVEL)
				return;
			// Pause the game.
			if (!paused)
			{
				Pause();
				// Bring up the in-game menu.
				OpenInGameMenu();
			}
			else
			{
				inGameMenuOpened = false;
				UpdateUI();
				Resume();
			}
		}
		else if (msg == "ToggleBlackness")
		{
			if (blacknessEntities.Size())
			{
				bool visible = blacknessEntities[0]->IsVisible();
				GraphicsMan.QueueMessage(new GMSetEntityb(blacknessEntities, GT_VISIBILITY, !visible));
				Viewport* viewport = MainWindow()->MainViewport();
				viewport->renderGrid = visible;
			}
		}

	}
	}
}


// Bring up the in-game menu.
void PlayingLevel::OpenInGameMenu()
{
	inGameMenuOpened = true;
	UpdateUI();
}


void PlayingLevel::Cleanup()
{
	/// Remove projectiles which have been passed by.
	for (int i = 0; i < projectileEntities.Size(); ++i)
	{
		EntitySharedPtr proj = projectileEntities[i];
		ProjectileProperty* pp = (ProjectileProperty*)proj->GetProperty(ProjectileProperty::ID());
		if (pp->sleeping ||
			(proj->worldPosition[0] < despawnPositionLeft ||
				proj->worldPosition[0] > spawnPositionRight ||
				proj->worldPosition[1] < -1.f ||
				proj->worldPosition[1] > playingFieldSize[1] + 2.f
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
		Ship* ship = level.ships[i];
		if (ship->destroyed || !ship->spawned)
		{
			level.ships.RemoveItem(ship); // Remove it. Remove links to spawn-group too.
			delete ship;
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

/// Loads target level. The source and separate .txt description have the same name, just different file-endings, e.g. "Level 1.png" and "Level 1.txt"
void PlayingLevel::LoadLevel(String fromSource)
{
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

	QueueGraphics(new GMSetUIb("LevelMessage", GMUI::VISIBILITY, false));


	showLevelStats = false;
	inGameMenuOpened = false;
	if (fromSource == "CurrentStageLevel")
	{
		fromSource = "Levels/Stage " + currentStage->ToString() + "/Level " + currentStage->ToString() + "-" + currentLevel->ToString();
	}
	if (currentStage->GetInt() == 0)
		fromSource = "Levels/Tutorial";
	this->levelSource = fromSource;
	// Delete all entities.
	MapMan.DeleteAllEntities();
	shipEntities.Clear();
	projectileEntities.Clear();

	GraphicsMan.PauseRendering();
	SleepThread(50);
	PhysicsMan.Pause();

	level.Load(fromSource);
	level.SetupCamera();
	if (!playerShip)
		NewPlayer();

	// Reset player cooldowns if needed.
	if (playerShip)
		playerShip->RandomizeWeaponCooldowns();


	/// Clear old stars?
	QueueGraphics(new GMClearParticles(stars));

	/// Add emitter for stars at player start.
	float emissionSpeed = level.starSpeed.Length();
	Vector3f starDir = level.starSpeed.NormalizedCopy();
	float starScale = 0.2f;

	ParticleEmitter* startEmitter = new ParticleEmitter();
	startEmitter->newType = true;
	startEmitter->instantaneous = true;
	startEmitter->constantEmission = 1400;
	startEmitter->positionEmitter.type = EmitterType::PLANE_XY;
	startEmitter->positionEmitter.SetScale(100.f);
	startEmitter->velocityEmitter.type = EmitterType::VECTOR;
	startEmitter->velocityEmitter.vec = starDir;
	startEmitter->SetEmissionVelocity(emissionSpeed);
	startEmitter->SetParticleLifeTime(60.f);
	startEmitter->SetScale(starScale);
	startEmitter->SetColor(level.starColor);
	QueueGraphics(new GMAttachParticleEmitter(startEmitter, stars));

	/// Update base emitter emitting all the time.
	starEmitter->newType = true;
	starEmitter->direction = starDir;
	starEmitter->SetEmissionVelocity(emissionSpeed);
	starEmitter->SetParticlesPerSecond(40);
	starEmitter->positionEmitter.type = EmitterType::PLANE_XY;
	starEmitter->positionEmitter.SetScale(30.f);
	starEmitter->velocityEmitter.type = EmitterType::VECTOR;
	starEmitter->velocityEmitter.vec = starDir;
	starEmitter->SetParticleLifeTime(60.f);
	starEmitter->SetColor(level.starColor);
	starEmitter->SetScale(starScale);


	/// Add entity to track for both the camera, blackness and player playing field.
	Vector3f initialPosition = Vector3f(0, 10, 0);
	if (!levelEntity)
	{
		levelEntity = EntityMan.CreateEntity("LevelEntity", NULL, NULL);
		levelEntity->localPosition = initialPosition;
		PhysicsProperty* pp = levelEntity->physics = new PhysicsProperty();
		pp->collisionsEnabled = false;
		pp->type = PhysicsType::KINEMATIC;
		/// Add blackness to track the level entity.
		for (int i = 0; i < 4; ++i)
		{
			EntitySharedPtr blackness = EntityMan.CreateEntity("Blackness" + String(i), ModelMan.GetModel("sprite.obj"), TexMan.GetTexture("0x0A"));
			float scale = 50.f;
			float halfScale = scale * 0.5f;
			blackness->scale = scale * Vector3f(1, 1, 1);
			Vector3f position;
			position[2] = 5.f; // Between game plane and camera
			switch (i)
			{
			case 0: position[0] += playingFieldHalfSize[0] + halfScale + playingFieldPadding; break;
			case 1: position[0] -= playingFieldHalfSize[0] + halfScale + playingFieldPadding; break;
			case 2: position[1] += playingFieldHalfSize[1] + halfScale + playingFieldPadding; break;
			case 3: position[1] -= playingFieldHalfSize[1] + halfScale + playingFieldPadding; break;
			}
			blackness->localPosition = position;
			levelEntity->AddChild(blackness);
			blacknessEntities.Add(blackness);
		}
		// Register blackness entities for rendering.
		GraphicsMan.QueueMessage(new GMRegisterEntities(blacknessEntities));
		PhysicsMan.QueueMessage(new PMRegisterEntities(levelEntity));
		// Set level camera to track the level entity.
		GraphicsMan.QueueMessage(new GMSetCamera(levelCamera, CT_ENTITY_TO_TRACK, levelEntity));
	}
	// Track ... level with effects.
	starEmitter->entityToTrack = levelEntity;
	starEmitter->positionOffset = Vector3f(playingFieldSize.x + 10.f, 0, 0);
	//	GraphicsMan.QueueMessage(new GMSetParticleEmitter(starEmitter, GT_EMITTER_ENTITY_TO_TRACK, playerShip->entity));
	//	GraphicsMan.QueueMessage(new GMSetParticleEmitter(starEmitter, GT_EMITTER_POSITION_OFFSET, Vector3f(70.f, 0, 0)));
		// Reset position of level entity if already created.
	levelEntity->localPosition = initialPosition;
	levelEntity->physics->velocity = level.BaseVelocity();
	//	PhysicsMan.QueueMessage(new PMSetEntity(levelEntity, PT_POSITION, initialPosition));
		// Set velocity of the game.
	//	PhysicsMan.QueueMessage(new PMSetEntity(levelEntity, PT_VELOCITY, level.BaseVelocity()));
		// Reset position of player!
	//	PhysicsMan.QueueMessage(new PMSetEntity(playerShip->entity, PT_POSITION, initialPosition));

	level.AddPlayer(*this, playerShip);
	// Reset player stats.
	playerShip->hp = playerShip->maxHP;
	playerShip->shieldValue = playerShip->maxShieldValue;
	playerShip->entity->localPosition = initialPosition + Vector3f(-50, 0, 0);


	sparks->SetAlphaDecay(DecayType::QUADRATIC);

	GraphicsMan.ResumeRendering();
	PhysicsMan.Resume();
	// Set mode! UI updated from within.
	SetMode(PLAYING_LEVEL);
	level.OnEnter();
	// Run start script.
	ScriptMan.PlayScript("scripts/OnLevelStart.txt");

	// o.o
	this->Resume();
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
	ShadeMan.SetActiveShader(NULL, *graphicsState);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(GraphicsThreadGraphicsState.projectionMatrixD.getPointer());
	glMatrixMode(GL_MODELVIEW);
	Matrix4d modelView = GraphicsThreadGraphicsState.viewMatrixD * GraphicsThreadGraphicsState.modelMatrixD;
	glLoadMatrixd(modelView.getPointer());
	// Enable blending
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	float z = -4;
	glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	GraphicsThreadGraphicsState.currentTexture = NULL;
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
		Ship* type = Ship::types[i];
		if (type->allied)
			continue;
		shipTypes.AddItem(type->name);
	}
	QueueGraphics(new GMSetUIContents(spawnUI, "ShipTypeToSpawn", shipTypes));
	List<String> spawnFormations;
	for (int i = 0; i < Formation::FORMATIONS; ++i)
	{
		spawnFormations.AddItem(Formation::GetName(i));
	}
	QueueGraphics(new GMSetUIContents(spawnUI, "SpawnFormation", spawnFormations));
}

void PlayingLevel::CloseSpawnWindow()
{
	if (spawnWindow)
		spawnWindow->Close();
}

#include "LevelEditor.h"

#include "File/LogFile.h"
#include "Missions.h"
#include "Properties/LevelProperty.h"

#include "Message/MathMessage.h"
#include "PlayingLevel/SpaceStars.h"
#include "StateManager.h"
#include "Graphics/Messages/GMUI.h"
#include "Level/SpawnGroup.h"
#include "Input/Action.h"
#include "Input/Keys.h"
#include "PlayingLevel.h"
#include "Window/AppWindowManager.h"

AppWindow* spawnWindow = 0;
UserInterface* spawnUI = 0;


/** 
	TODO:
	- Visualisera nivåns fiender och obstacles
	- Placera startpunkt, mål, fiender, triggers
	- Trigga meddelande eller något vid viss punkt på skärmen
*/

LevelEditor::LevelEditor() {

}

LevelEditor::~LevelEditor() {

}

// Inherited via AppState
void LevelEditor::OnEnter(AppState* previousState) {
	LogMain("Entering Level editor ", INFO);

	editedMission = MissionsMan.GetMissions()[0];
	LoadMission(editedMission);

	QueueGraphics(GMPushUI::ToWindow("gui/LevelEditor.gui"));

	inputMapping.bindings.Add(new Binding(Action::FromString("CenterCamera"), KEY::HOME));
	inputMapping.bindings.Add(new Binding(Action::CreateStartStopAction("ZoomOut"), KEY::PG_UP));
	inputMapping.bindings.Add(new Binding(Action::CreateStartStopAction("ZoomIn"), KEY::PG_DOWN));
}

int nextUpdateInfoMs = 100;

void LevelEditor::Process(int timeInMs) {
	if (zoomSpeed != 0)
		levelCamera->SetTargetZoom(levelCamera->CurrentZoom() + zoomSpeed);


	nextUpdateInfoMs -= timeInMs;
	if (nextUpdateInfoMs < 0) {
		nextUpdateInfoMs = 100;
		SpawnGroup * closest = nullptr;
		for (int i = 0; i < editedLevel.spawnGroups.Size(); ++i) {
			SpawnGroup* sg = editedLevel.spawnGroups[i];
			if (closest == nullptr)
				closest = sg;
			else if ((sg->spawnedAtPosition - levelCamera->Position()).LengthSquared() < (closest->spawnedAtPosition - levelCamera->position).LengthSquared()) {
				closest = sg;
			}
		}
		// Print details?
		QueueGraphics(new GMSetUIt("centerInfoText", GMUI::TEXT, closest ? closest->name : ""));
		if (closest) {
			QueueGraphics(new GMSetUIt("SpawnGroupName", GMUI::STRING_INPUT_TEXT, closest->name, spawnUI));
			QueueGraphics(new GMSetUIv2i("SpawnGroupPosition", GMUI::VECTOR_INPUT, closest->position, spawnUI));
			QueueGraphics(new GMSetUIv2i("SGSize", GMUI::VECTOR_INPUT, closest->size, spawnUI));
			QueueGraphics(new GMSetUIi("SGAmount", GMUI::INTEGER_INPUT, closest->number, spawnUI));
		}
	}
	

}

void LevelEditor::Render(GraphicsState* graphicsState) {
}

void LevelEditor::OnExit(AppState* nextState) {
	MapMan.DeleteAllEntities();

}

void LevelEditor::ProcessMessage(Message* message) {
	if (message->type == MessageType::FLOAT_MESSAGE) {
		// Do stuff, like moving spawn time.
	}
	else if (message->type == MessageType::INTEGER_MESSAGE) {
		String msg = message->msg;
		if (msg == "SetSGAmount") {
			editedSpawnGroup->number = ((IntegerMessage*)message)->value;
			editedSpawnGroup->Despawn();
		}
	}
	if (message->type == MessageType::STRING) {
		String msg = message->msg;
		if (msg == "OnReloadUI") {
			QueueGraphics(GMPushUI::ToWindow("gui/LevelEditor.gui"));
			QueueGraphics(new GMSetUIt("bottomInfoText", GMUI::TEXT, "Editing "+ editedMission->levelFilePath));
			QueueGraphics(GMPushUI::ToWindow("gui/SpawnWindow.gui", spawnWindow));
		}
		else if (msg == "ReloadLevel") {
			LoadMission(editedMission);
		}
		else if (msg == "CenterCamera") { 
			// Do stuff
			QueueGraphics(new GMSetCamera(levelCamera, CT_POSITION, Vector3f(0, 0, 10)));
		}
		else if (msg == "OpenLevel") {
			auto missions = MissionsMan.GetMissions();
			List<String> levelNames;
			for (int i = 0; i < missions.Size(); ++i) {
				levelNames.Add(missions[i]->levelFilePath);
			}
			QueueGraphics(new GMSetUIContents("OpenLevelDropDown", levelNames));
		}
		else if (msg == "StartZoomIn") {
			zoomSpeed = 2;
		}
		else if (msg == "StopZoomIn" || msg == "StopZoomOut") {
			zoomSpeed = 0;
		}
		else if (msg == "StartZoomOut") {
			zoomSpeed = -2;
		}
	}
	else if (message->type == MessageType::MOUSE_MESSAGE) {
		// do nothing
		MouseMessage * mm = (MouseMessage*)message;
		switch (mm->interaction) {
		case MouseMessage::LDOWN:
			movingCamera = true;
			break;
		case MouseMessage::LUP:
			movingCamera = false;
		case MouseMessage::MOVE:
			// Pan o-o
			if (movingCamera) {
				levelCamera->position -= Vector2f(mm->coords - previousMousePosition) * 0.005f * levelCamera->CurrentZoom();
			}
			break;
		case MouseMessage::SCROLL:
			levelCamera->SetTargetZoom(levelCamera->CurrentZoom() * (1 - mm->scrollDistance * 0.5f) - mm->scrollDistance);
			return;
		}
		previousMousePosition = mm->coords;
	}
	else {
		LogMain("Received unhandled message of type: " + String(message->type), INFO);
	}
	ProcessGeneralMessage(message);
}

void LevelEditor::LoadMission(Mission * mission) {
	editedLevel = Level();
	bool success = editedLevel.Load(editedMission->levelFilePath);

	MapMan.DeleteAllEntities();

	OpenSpawnWindow();

	if (!success) {
		LogMain("Unable to load level from source " + editedMission->levelFilePath + ".", ERROR);
		return;
	}

	QueueGraphics(new GMSetUIt("bottomInfoText", GMUI::TEXT, "Editing " + editedMission->levelFilePath));


	editedLevel.SetupCamera();
	levelCamera->trackingMode = TrackingMode::NONE;
	levelCamera->smoothness = 0.05f;
	QueueGraphics(new GMSetCamera(levelCamera, CT_ZOOM, 33.0f));

	/// Add entity to track for both the camera, blackness and player playing field.
	levelEntity = LevelEntity->Create(editedLevel.playingFieldSize, playingFieldPadding, false);
	LevelEntity->SetVelocity(editedLevel.BaseVelocity());

	/// Add emitter for stars at player start.
	ClearStars();
	NewStars(editedLevel.starSpeed.NormalizedCopy(), editedLevel.starSpeed.Length(), 0.2f, editedLevel.starColor);
	LetStarsTrack(levelEntity, Vector3f(editedLevel.playingFieldSize.x + 10.f, 0, 0));

	GraphicsMan.ResumeRendering();
	PhysicsMan.Pause();
	editedLevel.OnEnter();
	QueueGraphics(new GMRecompileShaders()); // Shouldn't be needed...

	// Pause all logic.
	PlayingLevel::paused = true;

	// Have 100 pixels correspond to 1 second? or based on the speed in-game?
	for (int i = 0; i < editedLevel.spawnGroups.Size(); ++i) {

		AETime levelTime = AETime(TimeType::MILLISECONDS_NO_CALENDER);

		// Jump ahead to spawn group time.
		SpawnGroup * sg = editedLevel.spawnGroups[i];
		// Set global var
		activeLevel = &editedLevel;

		levelTime = sg->SpawnTime();

		// 1 unit = 1 second, approx 100 pixels
		levelEntity->worldPosition.x = levelTime.Milliseconds() / 200;
		editedLevel.UpdateSpawnDespawnLimits(levelEntity);

		// Spawn until it returns false
		while (!sg->Spawn(levelTime, nullptr)) {
			levelTime.AddMs(10); // Add 10 ms per iteration until done.
		};
	}
}

void LevelEditor::OpenSpawnWindow()
{
	if (!spawnWindow)
	{
		spawnWindow = WindowMan.NewWindow("SpawnWindow", "Spawn Window");
		spawnWindow->SetRequestedSize(Vector2i(600, 400));
		spawnWindow->Create();
		UserInterface* ui = spawnUI = spawnWindow->CreateUI();
		QueueGraphics(GMPushUI::ToWindow("gui/SpawnWindow.gui", spawnWindow));
		//ui->Load("gui/SpawnWindow.gui");
		//ui->PushToStack("SpawnWindow");
	}
	spawnWindow->Show();
	// No need to re-render the 3d scene here.
	spawnWindow->renderScene = false;

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

void LevelEditor::CloseSpawnWindow()
{
	if (spawnWindow)
		spawnWindow->Close();
}



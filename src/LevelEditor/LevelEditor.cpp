
#include "LevelEditor.h"

#include "File/LogFile.h"
#include "Missions.h"
#include "Properties/LevelProperty.h"

#include "PlayingLevel/SpaceStars.h"
#include "StateManager.h"
#include "Graphics/Messages/GMUI.h"
#include "Level/SpawnGroup.h"
#include "Input/Action.h"
#include "Input/Keys.h"
#include "PlayingLevel.h"


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
}

void LevelEditor::Process(int timeInMs) {
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
	if (message->type == MessageType::STRING) {
		String msg = message->msg;
		if (msg == "OnReloadUI") {
			QueueGraphics(GMPushUI::ToWindow("gui/LevelEditor.gui"));
			QueueGraphics(new GMSetUIt("bottomInfoText", GMUI::TEXT, "Editing "+ editedMission->levelFilePath));
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
	}
	else if (message->type == MessageType::MOUSE_MESSAGE) {
		// do nothing
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

	if (!success) {
		LogMain("Unable to load level from source " + editedMission->levelFilePath + ".", ERROR);
		return;
	}

	QueueGraphics(new GMSetUIt("bottomInfoText", GMUI::TEXT, "Editing " + editedMission->levelFilePath));


	editedLevel.SetupCamera();
	levelCamera->trackingMode = TrackingMode::NONE;

	/// Add entity to track for both the camera, blackness and player playing field.
	levelEntity = LevelEntity->Create(editedLevel.playingFieldSize, playingFieldPadding, levelCamera);
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

		// 1 pixel = 5 ms, 1 second = 200 pixels
		//PlayingLevelRef().spawnPositionRight = levelTime.Milliseconds() / 5;

		// Spawn until it returns false
		while (!sg->Spawn(levelTime, nullptr)) {
			levelTime.AddMs(10); // Add 10 ms per iteration until done.
		};
	}
}


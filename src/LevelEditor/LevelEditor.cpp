
#include "LevelEditor.h"

// Engine
#include "File/LogFile.h"
#include "Message/MathMessage.h"
#include "StateManager.h"
#include "Graphics/Messages/GMUI.h"
#include "Input/Action.h"
#include "Input/Keys.h"
#include "Window/AppWindowManager.h"

// Game-specific
#include "Missions.h"
#include "Properties/LevelProperty.h"
#include "PlayingLevel/SpaceStars.h"
#include "PlayingLevel.h"
#include "Level/SpawnGroup.h"
#include "Level/LevelMessage.h"

AppWindow* spawnWindow = 0;
UserInterface* spawnUI = 0;


/** 
	TODO:
	- Visualisera nivåns fiender och obstacles
	- Placera startpunkt, mål, fiender, triggers
	- Trigga meddelande eller något vid viss punkt på skärmen
*/

LevelEditor::LevelEditor() {
	id = SSGameMode::LEVEL_EDITOR;
	editedSpawnGroup = nullptr;
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
		nextUpdateInfoMs = 200;
		auto spawnGroups = editedLevel.SpawnGroups();
		SpawnGroup * closestSG = nullptr;
		LevelMessage * closestLM = nullptr;
		float closestDistance = 100000;
		for (int i = 0; i < spawnGroups.Size(); ++i) {
			SpawnGroup* sg = spawnGroups[i];
			float distance = (sg->spawnedAtPosition - levelCamera->Position()).LengthSquared();
			if (closestSG == nullptr || distance < closestDistance) {
				closestSG = sg;
				closestDistance = distance;
			}
		}
		auto levelMessages = editedLevel.Messages();
		for (int i = 0; i < levelMessages.Size(); ++i) {
			LevelMessage * lm = levelMessages[i];
			float distance = (lm->editorPosition - levelCamera->Position()).LengthSquared();
			if (distance < closestDistance)
			{
				closestLM = lm;
				closestDistance = distance;
				closestSG = nullptr;
			}
		}

		// Print details?
		if (closestSG) {
			QueueGraphics(new GMSetUIb("lLevelMessageEditor", GMUI::VISIBILITY, false, spawnUI));
			QueueGraphics(new GMSetUIt("centerInfoText", GMUI::TEXT, closestSG ? closestSG->name : ""));
			if (closestSG) {
				QueueGraphics(new GMSetUIt("SGName", GMUI::STRING_INPUT_TEXT, closestSG->name, spawnUI));
				QueueGraphics(new GMSetUIs("SGShipType", GMUI::DROP_DOWN_INPUT_SELECT, closestSG->shipType, spawnUI));
				QueueGraphics(new GMSetUIs("SpawnFormation", GMUI::DROP_DOWN_INPUT_SELECT, GetName(closestSG->formation), spawnUI));
				QueueGraphics(new GMSetUIv2i("SGPosition", GMUI::VECTOR_INPUT, closestSG->position, spawnUI));
				QueueGraphics(new GMSetUIv2i("SGSize", GMUI::VECTOR_INPUT, closestSG->size, spawnUI));
				QueueGraphics(new GMSetUIi("SGAmount", GMUI::INTEGER_INPUT, closestSG->number, spawnUI));
				QueueGraphics(new GMSetUIs("SGSpawnTime", GMUI::STRING_INPUT_TEXT, closestSG->spawnTimeString, spawnUI));
				

				if (editedSpawnGroup != nullptr)
					QueueGraphics(new GMSetEntityVec4f(editedSpawnGroup->GetEntities(), GT_COLOR, Vector4f(1, 1, 1, 1)));
				editedSpawnGroup = closestSG;
				QueueGraphics(new GMSetEntityVec4f(editedSpawnGroup->GetEntities(), GT_COLOR, Vector4f(3, 3, 3, 1)));
			}

			editedLevelMessage = nullptr;
		}
		else if (closestLM) {
			editedSpawnGroup = nullptr;

			QueueGraphics(new GMSetUIt("centerInfoText", GMUI::TEXT, closestLM->GetEditorText(20)));
			QueueGraphics(new GMSetUIb("lLevelMessageEditor", GMUI::VISIBILITY, true, spawnUI));
			QueueGraphics(new GMSetUIs("LMName", GMUI::STRING_INPUT_TEXT, closestLM->name, spawnUI));
			QueueGraphics(new GMSetUIi("LMStartTime", GMUI::INTEGER_INPUT, closestLM->startTimeOffsetSeconds, spawnUI));
			QueueGraphics(new GMSetUIs("LMTextID", GMUI::STRING_INPUT_TEXT, closestLM->textID, spawnUI));
			QueueGraphics(new GMSetUIs("LMScript", GMUI::STRING_INPUT_TEXT, (closestLM->strings.Size() > 0? closestLM->strings[0] : ""), spawnUI));
			QueueGraphics(new GMSetUIs("LMCondition", GMUI::STRING_INPUT_TEXT, closestLM->condition, spawnUI));
			QueueGraphics(new GMSetUIb("LMGoToRewindPoint", GMUI::TOGGLED, closestLM->goToRewindPoint, spawnUI));

			editedLevelMessage = closestLM;
			
		}
	}
	

}

void LevelEditor::Render(GraphicsState* graphicsState) {
}

void LevelEditor::OnExit(AppState* nextState) {
	MapMan.DeleteAllEntities();

	// Hide UI
	QueueGraphics(new GMPopUI("gui/LevelEditor.gui", true));
	if (spawnWindow)
		spawnWindow->Hide();
}

void LevelEditor::ProcessMessage(Message* message) {
	String msg = message->msg;
	if (message->type == MessageType::FLOAT_MESSAGE) {
		// Do stuff, like moving spawn time.
	}
	else if (message->type == MessageType::SET_STRING) {
		auto strmsg = ((SetStringMessage*)message);
		if (editedSpawnGroup != nullptr) {
			if (msg == "SetSGSpawnTime") {
				editedSpawnGroup->SetSpawnTimeString(strmsg->value, PreviousMessageOrSpawnGroupTime(editedSpawnGroup));
				Respawn(editedSpawnGroup);
			}
			else if (msg.Contains("SGShipType")) {
				editedSpawnGroup->shipType = strmsg->value;
				Respawn(editedSpawnGroup);
			}
			else if (msg.Contains("SpawnFormation")) {
				editedSpawnGroup->formation = GetFormationByName(strmsg->value);
				Respawn(editedSpawnGroup);
			}
		}
	}
	else if (message->type == MessageType::INTEGER_MESSAGE) {
		if (msg == "SetSGAmount") {
			editedSpawnGroup->number = ((IntegerMessage*)message)->value;
			if (editedSpawnGroup->number < 1)
				editedSpawnGroup->number = 1;
			Respawn(editedSpawnGroup);
		}
	}
	else if (message->type == MessageType::VECTOR_MESSAGE) {
		VectorMessage * vm = ((VectorMessage*)message);
		if (msg == "SetSGPosition") {
			editedSpawnGroup->position = vm->GetVector2f();
			Respawn(editedSpawnGroup);
		}
		else if (msg == "SetSGSize") {
			editedSpawnGroup->size = vm->GetVector2f();
			Respawn(editedSpawnGroup);
		}
	}
	if (message->type == MessageType::STRING) {
		if (msg == "OnReloadUI") {
			QueueGraphics(GMPushUI::ToWindow("gui/LevelEditor.gui"));
			QueueGraphics(new GMSetUIt("bottomInfoText", GMUI::TEXT, "Editing "+ editedMission->levelFilePath));
			QueueGraphics(GMPushUI::ToWindow("gui/SpawnWindow.gui", spawnWindow));
			PopulateSpawnWindowLists();
		}
		else if (msg == "PlaytestLevel") {
			// Save level in an .editor file.
			levelToTest = "editor.level";
			editedLevel.Save(levelToTest);
			// Go to PlayingLevel, set up onComplete and onDeath to return to the editor.
			SetMode(SSGameMode::PLAYING_LEVEL);
		}
		else if (msg == "DeleteLM") {
			// Despawn entiti representin it.
			editedLevelMessage->DespawnEditorEntity();
			int index = editedLevel.GetElementIndexOf(editedLevelMessage);
			editedLevel.levelElements.RemoveIndex(index, ListOption::RETAIN_ORDER);
			delete editedLevelMessage;
			// Update the remaining spawngroups after here.
			UpdatePositionsOfSpawnGroupsAfterIndex(index);
			editedLevelMessage = nullptr;
		}
		else if (msg == "CreateNewGroup") {
			if (editedSpawnGroup) {
				SpawnGroup * sg = new SpawnGroup(*editedSpawnGroup); // Copy over relevant data if available.
				int index = editedLevel.GetElementIndexOf(editedSpawnGroup);
				editedLevel.levelElements.Insert(LevelElement(sg), index + 1); // Insert one slot after the current selection.
				sg->SetSpawnTimeString(sg->spawnTimeString, editedSpawnGroup->spawnTime);
				Spawn(sg);
				UpdatePositionsOfSpawnGroupsAfterIndex(index);
			}
			else {
				SpawnGroup * sg = new SpawnGroup();
				sg->number = 3;
				editedLevel.levelElements.Add(LevelElement(sg));
				Spawn(sg);
			}
		}
		else if (msg == "DeleteSG") {
			if (editedSpawnGroup) {
				editedSpawnGroup->Despawn();
				int index = editedLevel.GetElementIndexOf(editedSpawnGroup);
				editedLevel.levelElements.RemoveIndex(index, ListOption::RETAIN_ORDER);
				delete editedSpawnGroup;
				// Update the remaining spawngroups after here.
				UpdatePositionsOfSpawnGroupsAfterIndex(index);
				editedSpawnGroup = nullptr;
			}
		}
		else if (msg == "SaveLevel") {
			editedLevel.Save(editedLevel.source+".overwrite");
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
			if (movingCamera && mm->element == nullptr) {
				levelCamera->position -= Vector2f(mm->coords - previousMousePosition) * 0.005f * levelCamera->CurrentZoom();
			}
			break;
		case MouseMessage::SCROLL:
			if (mm->element == nullptr)
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
	Time levelTime(TimeType::MILLISECONDS_NO_CALENDER);
	for (int i = 0; i < editedLevel.levelElements.Size(); ++i) {

		LevelElement le = editedLevel.levelElements[i];
		if (le.sg) {
			SpawnGroup * sg = le.sg;
			sg->InvalidateSpawnTime();
			sg->SetSpawnTimeString(sg->spawnTimeString, levelTime);
			// Set global var
			activeLevel = &editedLevel;
			Spawn(sg);
			levelTime = sg->spawnTime;
		}
		else if (le.lm) {
			// Create a placeholder entity to represent the message? 
			//QueueGraphics(new GMRenderText)
			LevelMessage * levelMessage = le.lm;

			int minOffset = 1;
			int offset = levelMessage->startTimeOffsetSeconds;
			if (offset < minOffset)
				offset = minOffset;

			levelMessage->startTime = GetSpawnTime(levelTime, offset);
			UpdateWorldEntityForLevelTime(levelMessage->startTime);
			levelMessage->SpawnEditorEntity();
			levelTime = levelMessage->startTime;
		}
	}

	// Recalc positions as the above doesn't work..?
	//UpdatePositionsOfSpawnGroupsAfterIndex(0);
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

	PopulateSpawnWindowLists();
}

void LevelEditor::PopulateSpawnWindowLists() {
	/// Update lists inside.
	List<String> shipTypes;
	for (int i = 0; i < Ship::types.Size(); ++i)
	{
		Ship* type = Ship::types[i];
		if (type->allied)
			continue;
		shipTypes.AddItem(type->name);
	}
	QueueGraphics(new GMSetUIContents(spawnUI, "SGShipType", shipTypes));
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

// Spawns spawn group at appropriate place in the editor for manipulation.
void LevelEditor::Spawn(SpawnGroup * sg) {

	AETime levelTime = sg->spawnTime;

	UpdateWorldEntityForLevelTime(levelTime);

	// Spawn until it returns false
	while (!sg->Spawn(levelTime, nullptr)) {
		levelTime.AddMs(10); // Add 10 ms per iteration until done.
	};

}

void LevelEditor::Respawn(SpawnGroup * sg) {
	sg->Despawn();
	sg->ResetForSpawning();
	Spawn(sg);
}

Time LevelEditor::PreviousMessageOrSpawnGroupTime(void * comparedToSGorLM) {
	for (int i = 0; i < editedLevel.levelElements.Size(); ++i) {
		SpawnGroup * sg = editedLevel.levelElements[i].sg;
		LevelMessage * lm = editedLevel.levelElements[i].lm;

		if (comparedToSGorLM == sg || comparedToSGorLM == lm) {
			// return the previous index, if possible.
			auto previous = editedLevel.levelElements[i - 1];
			if (i >= 1) {
				if (previous.sg)
					return previous.sg->spawnTime;
				else
					return previous.lm->startTime;
			}
		}
	}
	return Time(TimeType::MILLISECONDS_NO_CALENDER);
}

void LevelEditor::UpdatePositionsOfSpawnGroupsAfterIndex(int index) {
	// First reset the spawn/start times to invalid, as they all need re-calculation, otherwise bad values will be reused in LastMessageOrSpawnGroupTime
	auto spawnGroups = editedLevel.SpawnGroups();
	for (int i = 0; i < editedLevel.levelElements.Size(); ++i) {
		LevelElement levelElement = editedLevel.levelElements[i];
		levelElement.InvalidateSpawnTime();
	}

	// Now update positions of all groups and messages
	Time levelTime(TimeType::MILLISECONDS_NO_CALENDER);
	for (int i = 0; i < editedLevel.levelElements.Size(); ++i) {
		LevelElement le = editedLevel.levelElements[i];
		if (le.sg) {
			SpawnGroup * sg = le.sg;
			sg->SetSpawnTimeString(sg->spawnTimeString, levelTime);
			levelTime = sg->spawnTime;

			UpdateWorldEntityForLevelTime(sg->spawnTime);
			Respawn(sg);
		}
		else if (le.lm) {
			LevelMessage* levelMessage = le.lm;
			levelMessage->DespawnEditorEntity();

			levelMessage->startTime = levelTime;
			levelMessage->startTime.AddSeconds(levelMessage->startTimeOffsetSeconds);
			levelTime = levelMessage->startTime;

			UpdateWorldEntityForLevelTime(levelMessage->startTime);
			levelMessage->SpawnEditorEntity();

		}
	}

	for (int i = index; i < spawnGroups.Size(); ++i) {
		// Translate all ships as needed.
		//Vector3f toTranslate = sg->CalcGroupSpawnPosition() - sg->spawnedAtPosition;
		//QueuePhysics(new PMSetEntity(sg->GetEntities(), PT_TRANSLATE, toTranslate));
	}
}

void LevelEditor::UpdateWorldEntityForLevelTime(Time levelTime) {
	// approx 100 pixels
	levelEntity->worldPosition.x = levelTime.Milliseconds() / 200;
	editedLevel.UpdateSpawnDespawnLimits(levelEntity);
}



#include "LevelEditor.h"

// Engine
#include "File/LogFile.h"
#include "Message/FileEvent.h"
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
#include "Level/LevelElement.h"
#include "ShipTypeEditor.h"

AppWindow* spawnWindow = 0;
UserInterface* spawnUI = 0;

ShipTypeEditor shipTypeEditor;

/** 
	TODO:
	- Visualisera nivåns fiender och obstacles
	- Placera startpunkt, mål, fiender, triggers
	- Trigga meddelande eller något vid viss punkt på skärmen
*/

LevelEditor::LevelEditor() {
	id = SSGameMode::LEVEL_EDITOR;
	Initialize();
	keyPressedCallback = true;
}

LevelEditor::~LevelEditor() {

}

void LevelEditor::Initialize() {
	editedSpawnGroup = nullptr;
	editedMission = nullptr;
	editedLevelMessage = nullptr;
	editedLevel.levelElements.ClearAndDelete();
}

// Inherited via AppState
void LevelEditor::OnEnter(AppState* previousState) {
	Initialize();

	QueueGraphics(new GMRecompileShaders()); // Shouldn't be needed...

	automaticallySelectClosestElement = true;

	LogMain("Entering Level editor ", INFO);

	// Did we come back after playtesting? Then load the cached level.
	if (this->levelToTest.Length()) {
		LoadLevel(levelToTest);
		levelToTest = "";
	}
	// Otherwise load the tutorial level?
	else {
		//LoadMission(MissionsMan.GetMissions()[0], false);
	}


	QueueGraphics(GMPushUI::ToWindow("gui/LevelEditor.gui", WindowMan.MainWindow()));

	inputMapping.bindings.Add(new Binding(Action::FromString("CenterCamera"), KEY::HOME));
	inputMapping.bindings.Add(new Binding(Action::CreateStartStopAction("ZoomOut"), KEY::PG_UP));
	inputMapping.bindings.Add(new Binding(Action::CreateStartStopAction("ZoomIn"), KEY::PG_DOWN));
	inputMapping.bindings.Add(new Binding(Action::CreateStartStopAction("PanRight"), KEY::RIGHT));
	inputMapping.bindings.Add(new Binding(Action::CreateStartStopAction("PanLeft"), KEY::LEFT));
	inputMapping.bindings.Add(new Binding(Action::FromString("ClearSelection"), KEY::ESCAPE));

	// Initially disable some buttons.
	OnLevelLoaded();
	

}

int nextUpdateInfoMs = 100;

LevelElement* LevelEditor::GetElementClosestToCamera() const {
	if (levelCamera == nullptr)
		return nullptr;

	float closestDistance = 100000;
	LevelElement* closest = nullptr;
	for (int i = 0; i < editedLevel.levelElements.Size(); ++i) {
		LevelElement* levelElement = editedLevel.levelElements[i];
		if (levelElement->spawnGroup) {
			SpawnGroup* sg = levelElement->spawnGroup;
			float distance = (sg->spawnedAtPosition - levelCamera->Position()).LengthSquared();
			if (closest == nullptr || distance < closestDistance) {
				closestDistance = distance;
				closest = levelElement;
			}
		}
		else if (levelElement->levelMessage) {
			LevelMessage * lm = levelElement->levelMessage;
			float distance = (lm->editorPosition - levelCamera->Position()).LengthSquared();
			if (closest == nullptr || distance < closestDistance)
			{
				closestDistance = distance;
				closest = levelElement;
			}
		}
	}
	return closest;
}

void LevelEditor::Select(LevelElement* levelElement) {
	editedLevelElement = levelElement;
	if (editedLevelElement)
		OpenSpawnWindow();
	OnEditedLevelElementChanged();
	automaticallySelectClosestElement = false;


	if (levelElement) {
		QueueGraphics(new GMSetUIt("bottomInfoText", GMUI::TEXT, "Selected element"));
	}
}

void LevelEditor::OnEditedLevelElementChanged() {
	if (editedLevelElement == nullptr)
		return;

	// Print details?
	String indexString = "Element "+String(editedLevel.levelElements.GetIndexOf(editedLevelElement)) + "/" + String(editedLevel.levelElements.Size());

	if (editedLevelElement->spawnGroup) {
		SpawnGroup* sg = editedLevelElement->spawnGroup;
		QueueGraphics(new GMSetUIb("lLevelMessageEditor", GMUI::VISIBILITY, false, spawnUI));
		QueueGraphics(new GMSetUIt("centerInfoText", GMUI::TEXT, indexString + " : "+ (sg ? sg->name : "")));
		if (sg) {
			QueueGraphics(new GMSetUIt("SGName", GMUI::STRING_INPUT_TEXT, sg->name, spawnUI));
			QueueGraphics(new GMSetUIs("SGShipType", GMUI::DROP_DOWN_INPUT_SELECT, sg->shipType, spawnUI));
			QueueGraphics(new GMSetUIs("SpawnFormation", GMUI::DROP_DOWN_INPUT_SELECT, GetName(sg->formation), spawnUI));
			QueueGraphics(new GMSetUIs("SGMovementPattern", GMUI::DROP_DOWN_INPUT_SELECT, sg->movementPattern.name, spawnUI));
			QueueGraphics(new GMSetUIv2i("SGPosition", GMUI::VECTOR_INPUT, sg->position, spawnUI));
			QueueGraphics(new GMSetUIv2i("SGSize", GMUI::VECTOR_INPUT, sg->size, spawnUI));
			QueueGraphics(new GMSetUIi("SGAmount", GMUI::INTEGER_INPUT, sg->number, spawnUI));
			QueueGraphics(new GMSetUIs("SGSpawnTime", GMUI::STRING_INPUT_TEXT, sg->spawnTimeString, spawnUI));
			QueueGraphics(new GMSetUIi("SGSpeed", GMUI::INTEGER_INPUT, sg->relativeSpeed, spawnUI));


			if (editedSpawnGroup != nullptr)
				QueueGraphics(new GMSetEntityVec4f(editedSpawnGroup->GetEntities(), GT_COLOR, Vector4f(1, 1, 1, 1)));
			editedSpawnGroup = sg;
			QueueGraphics(new GMSetEntityVec4f(editedSpawnGroup->GetEntities(), GT_COLOR, Vector4f(3, 3, 3, 1)));
		}

		if (editedLevelMessage != nullptr)
			editedLevelMessage->ResetEditorEntityColor();
		editedLevelMessage = nullptr;
	}
	else if (editedLevelElement->levelMessage) {
		auto lm = editedLevelElement->levelMessage;
		if (editedSpawnGroup != nullptr)
			QueueGraphics(new GMSetEntityVec4f(editedSpawnGroup->GetEntities(), GT_COLOR, Vector4f(1, 1, 1, 1)));
		editedSpawnGroup = nullptr;

		QueueGraphics(new GMSetUIt("centerInfoText", GMUI::TEXT, indexString + " : " + lm->GetEditorText(20)));
		QueueGraphics(new GMSetUIb("lLevelMessageEditor", GMUI::VISIBILITY, true, spawnUI));
		QueueGraphics(new GMSetUIs("LMName", GMUI::STRING_INPUT_TEXT, lm->name, spawnUI));
		QueueGraphics(new GMSetUIi("LMStartTime", GMUI::INTEGER_INPUT, lm->startTimeOffsetSeconds, spawnUI));
		QueueGraphics(new GMSetUIs("LMTextID", GMUI::STRING_INPUT_TEXT, lm->textID, spawnUI));
		QueueGraphics(new GMSetUIs("LMScript", GMUI::STRING_INPUT_TEXT, (lm->string), spawnUI));
		QueueGraphics(new GMSetUIs("LMCondition", GMUI::STRING_INPUT_TEXT, lm->condition, spawnUI));
		QueueGraphics(new GMSetUIb("LMGoToRewindPoint", GMUI::TOGGLED, lm->goToRewindPoint, spawnUI));

		if (editedLevelMessage != nullptr)
			editedLevelMessage->ResetEditorEntityColor();

		editedLevelMessage = lm;
		QueueGraphics(new GMSetEntityVec4f(editedLevelMessage->editorEntity, GT_COLOR, Vector4f(3, 3, 3, 1)));
	}
}

void LevelEditor::Process(int timeInMs) {
	if (levelCamera) {
		if (zoomSpeed != 0)
			levelCamera->SetTargetZoom(levelCamera->CurrentZoom() + zoomSpeed);

		levelCamera->velocity = cameraPan;
		levelCamera->scaleSpeedWithZoom = true;
	}

	if (shipTypeEditor.Visible()) {
		shipTypeEditor.Process(timeInMs);
		return;
	}

	nextUpdateInfoMs -= timeInMs;
	if (nextUpdateInfoMs < 0) {
		nextUpdateInfoMs = 200;
		
		QueueGraphics(new GMSetUIb("DeleteElement", GMUI::ENABLED, editedLevelElement != nullptr));

		if (automaticallySelectClosestElement && !shipTypeEditor.Visible()) {
			editedLevelElement = nullptr;
			editedLevelElement = GetElementClosestToCamera();
			OnEditedLevelElementChanged();
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

/// Callback from the Input-manager, query it for additional information as needed.
void LevelEditor::KeyPressed(int keyCode, bool downBefore) {
	//if (keyCode == KEY::ESCAPE) {
	//	if (automaticallySelectClosestElement == false)
	//		automaticallySelectClosestElement = true;
	//}
}

void LevelEditor::ProcessMessage(Message* message) {
	String msg = message->msg;
	shipTypeEditor.ProcessMessage(message);
	if (message->type == MessageType::ON_UI_PUSHED) {
		OnUIPushed* uiPushed = (OnUIPushed*)message;
		// Set default path to ./Level/
		if (uiPushed->msg == "LoadLevelFromDialog")
			QueueGraphics(new GMSetUIs("LoadLevelFromDialog", GMUI::FILE_BROWSER_PATH, "./Levels"));
		else if (msg == "SaveLevelAsDialog")
			QueueGraphics(new GMSetUIs("SaveLevelAsDialog", GMUI::FILE_BROWSER_PATH, "./Levels"));
	}
	else if (message->type == MessageType::BOOL_MESSAGE) {
		BoolMessage * bm = (BoolMessage*)message;
		if (msg.Contains("LMGoToRewindPoint")) {
			editedLevelMessage->goToRewindPoint = bm->value;
		}
	}
	else if (message->type == MessageType::FILE_EVENT) {
		FileEvent * fileEvent = (FileEvent*)message;
		if (msg == "SaveLevelAsDialog") {
			if (fileEvent->files.Size()) {
				levelSavePath = fileEvent->files[0];
				bool ok = editedLevel.Save(fileEvent->files[0]);
				QueueGraphics(new GMSetUIt("bottomInfoText", GMUI::TEXT, "Saved level: " + fileEvent->files[0]));
			}
		}
		else if (msg == "LoadLevelFromDialog") {
			editedMission = nullptr; // No longer editing any mission, only free levels.
			if (fileEvent->files.Size()) {
				levelSavePath = fileEvent->files[0];
				bool ok = LoadLevel(fileEvent->files[0]);
				QueueGraphics(new GMSetUIt("bottomInfoText", GMUI::TEXT, "Loaded level: " + fileEvent->files[0]));
			}
		}
	}
	else if (message->type == MessageType::FLOAT_MESSAGE) {
		// Do stuff, like moving spawn time.
	}
	else if (message->type == MessageType::SET_STRING) {
		auto strmsg = ((SetStringMessage*)message);
		if (msg == "SetStaticBackground") {
			editedLevel.backgroundSource = strmsg->value;
			LevelEntity->SetBackgroundSource(strmsg->value);
		}
		if (editedSpawnGroup != nullptr) {
			if (msg == "SetSGSpawnTime") {
				editedSpawnGroup->SetSpawnTimeString(strmsg->value, PreviousMessageOrSpawnGroupTime(editedSpawnGroup));
				Respawn(editedLevelElement);
				UpdatePositionsLevelElementsAfter(editedLevelElement);
			}
			else if (msg.Contains("SGShipType")) {
				editedSpawnGroup->shipType = strmsg->value;
				Respawn(editedLevelElement);
			}
			else if (msg.Contains("SpawnFormation")) {
				editedSpawnGroup->formation = GetFormationByName(strmsg->value);
				Respawn(editedLevelElement);
			}
			else if (msg == "SetSGName") {
				editedSpawnGroup->name = strmsg->value;
			}
			else if (msg.Contains("SGMovementPattern"))
				editedSpawnGroup->movementPattern = MovementPattern::ByName(strmsg->value);
		}
		else if (editedLevelMessage) {
			if (msg == "SetLMName")
				editedLevelMessage->name = strmsg->value;
			else if (msg == "SetLMTextID")
				editedLevelMessage->textID = strmsg->value;
			else if (msg == "SetLMScript")
				editedLevelMessage->string = strmsg->value;
			else if (msg == "SetLMCondition")
				editedLevelMessage->condition = strmsg->value;
			else if (msg == "")
				;
			else if (msg == "")
				;
			else if (msg == "")
				;
		}
	}
	else if (message->type == MessageType::INTEGER_MESSAGE) {
		auto intMsg = ((IntegerMessage*)message);
		if (editedSpawnGroup) {
			if (msg == "SetSGAmount") {
				editedSpawnGroup->number = ((IntegerMessage*)message)->value;
				if (editedSpawnGroup->number < 1)
					editedSpawnGroup->number = 1;
				Respawn(editedLevelElement);
			}
			if (msg == "SetSGSpeed") {
				editedSpawnGroup->relativeSpeed = ((IntegerMessage*)message)->value;
			}
		}
		if (editedLevelMessage) {
			if (msg == "SetLMStartTime") {
				editedLevelMessage->startTimeOffsetSeconds = intMsg->value;
				Respawn(editedLevelElement);
			}

		}
	}
	else if (message->type == MessageType::VECTOR_MESSAGE) {
		VectorMessage * vm = ((VectorMessage*)message);
		if (msg == "SetSGPosition") {
			editedSpawnGroup->position = vm->GetVector2f();
			Respawn(editedLevelElement);
		}
		else if (msg == "SetSGSize") {
			editedSpawnGroup->size = vm->GetVector2f();
			Respawn(editedLevelElement);
		}
		else if (msg == "SetStarColor") {
			editedLevel.starColor = vm->GetVector4f();
			// Start animating stars..? or skip it?
		}
		else if (msg == "SetStarSpeed") {
			editedLevel.starSpeed = vm->GetVector3f();
		}
	}
	if (message->type == MessageType::STRING) {
		if (msg == "OnReloadUI") {
			QueueGraphics(GMPushUI::ToWindow("gui/LevelEditor.gui", WindowMan.MainWindow()));
			QueueGraphics(new GMSetUIt("bottomInfoText", GMUI::TEXT, "Reloaded UI"));
			if (spawnWindow == nullptr)
				OpenSpawnWindow();
			else {
				spawnWindow->Show();
				QueueGraphics(GMPushUI::ToWindow("gui/SpawnWindow.gui", spawnWindow));
				PopulateSpawnWindowLists();
			}
		}
		else if (msg == "NewSG") // New spawn group.
			CreateNewSpawnGroup();
		else if (msg == "NewLM")
			CreateNewLevelMessage();
		else if (msg == "DeleteElement") {
			DeleteEditedElement();
		}
		else if (msg == "NewLevel") {
			editedLevel.source = "newlevel.srl"; // Clear it
			editedLevel.levelElements.ClearAndDelete();
			editedLevel.Save("newlevel.srl"); // Overwrite if we had some old shit.
			editedMission = nullptr;
			LoadLevel(editedLevel.source); // Load it for interactin'
		}
		else if (msg == "SaveLevelAs") {
			
		}
		else if (msg == "PlaytestLevel") {
			// Save level in an .editor file.
			levelToTest = "editor.level";
			editedLevel.Save(levelToTest);
			// Go to PlayingLevel, set up onComplete and onDeath to return to the editor.
			SetMode(SSGameMode::PLAYING_LEVEL);
		}
		else if (msg == "DeleteLM") {
			DeleteEditedElement();
		}
		else if (msg == "CreateNewGroup") {
			CreateNewSpawnGroup();
		}
		else if (msg == "DeleteSG") {
			DeleteEditedElement();
		}
		else if (msg == "SaveLevel") {
			editedLevel.Save(levelSavePath);
			QueueGraphics(new GMSetUIt("bottomInfoText", GMUI::TEXT, "Saved level: "+ levelSavePath));
		}
		else if (msg == "ReloadLevel") {
			if (editedMission)
				LoadMission(editedMission, true);
			else
				LoadLevel(editedLevel.source);
		}
		else if (msg == "CenterCamera") { 
			if (levelCamera)
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
		else if (msg == "StartPanRight")
			cameraPan.x = 1;
		else if (msg == "StopPanRight" || msg == "StopPanLeft")
			cameraPan.x = 0;
		else if (msg == "StartPanLeft")
			cameraPan.x = -1;
		else if (msg == "ClearSelection") {
			Select(nullptr);
			automaticallySelectClosestElement = true;
			QueueGraphics(new GMSetUIt("bottomInfoText", GMUI::TEXT, "Cleared selection."));
		}
	}
	else if (message->type == MessageType::RAYCAST) {
		Raycast * raycast = (Raycast*)message;
		if (raycast->isecs.Size()) {
			auto intersection = raycast->isecs[0];
			for (int i = 0; i < raycast->isecs.Size(); ++i) {
				auto entity = raycast->isecs[i].entity;
				for (int p = 0; p < entity->properties.Size(); ++p) {
					auto entityProperty = entity->properties[p];
					auto lmProperty = (LevelMessageProperty*)entity->GetProperty(LevelMessageProperty::ID);
					auto sgProperty = (SpawnGroupProperty*)entity->GetProperty(SpawnGroupProperty::ID);
					if (lmProperty)
						Select(lmProperty->levelElement);
					else if (sgProperty)
						Select(sgProperty->levelElement);
				}
			}
		}
	}
	else if (message->type == MessageType::MOUSE_MESSAGE) {
		// do nothing
		MouseMessage * mm = (MouseMessage*)message;
		switch (mm->interaction) {
		case MouseMessage::LDOWN:
			movingCamera = true; 
			screenDistancePanned = 0;
			break;
		case MouseMessage::LUP: {
			movingCamera = false;
			if (mm->element == nullptr && screenDistancePanned < 5) {
				// Do some raytracing or just check if an enity is near here?
				AppWindow * activeWindow = ActiveWindow();
				if (activeWindow != MainWindow())
					return;
				// Try to get ray.
				Ray ray;
				if (!activeWindow->GetRayFromScreenCoordinates(mm->coords, ray))
					return;
				QueuePhysics(new PMRaycast(ray));
			}
			break;
		}
		case MouseMessage::MOVE:
			// Pan o-o
			if (levelCamera && movingCamera && mm->element == nullptr) {
				screenDistancePanned += (mm->coords - previousMousePosition).Length();
				levelCamera->position -= Vector2f(mm->coords - previousMousePosition) * 0.005f * levelCamera->CurrentZoom();
			}
			break;
		case MouseMessage::SCROLL:
			if (levelCamera && mm->element == nullptr)
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

void LevelEditor::DeleteEditedElement() {
	int index = editedLevel.levelElements.GetIndexOf(editedLevelElement);
	editedLevelElement->Despawn();
	editedLevel.levelElements.RemoveIndex(index, ListOption::RETAIN_ORDER);

	if (editedLevelElement) {
		SAFE_DELETE(editedLevelElement->spawnGroup);
		SAFE_DELETE(editedLevelElement->levelMessage); 
		editedLevelMessage = nullptr;
		editedSpawnGroup = nullptr;
	}
	SAFE_DELETE(editedLevelElement);

	// Update the remaining spawngroups after here.
	UpdatePositionsLevelElementsAfter(index - 1);

	automaticallySelectClosestElement = true; // Auto-target new elements if deleting currently selected one(s)
}

void LevelEditor::CreateNewSpawnGroup() {
	SpawnGroup * sg;
	if (editedLevelElement) {
		if (editedLevelElement->spawnGroup)
			sg = new SpawnGroup(*editedSpawnGroup); // Copy over relevant data if available.
		else
			sg = new SpawnGroup();
		int index = editedLevel.levelElements.GetIndexOf(editedLevelElement);
		editedLevel.levelElements.Insert(new LevelElement(sg), index + 1); // Insert one slot after the current selection.
		editedLevelElement = editedLevel.levelElements[index + 1];
		UpdatePositionsLevelElementsAfter(index - 1);
	}
	else {
		sg = new SpawnGroup();
		editedLevel.levelElements.Add(new LevelElement(sg));
		editedLevelElement = editedLevel.levelElements[0];
	}
	if (sg)
		QueueGraphics(new GMSetUIt("bottomInfoText", GMUI::TEXT, "New Spawn group created: " + sg->name));
}

void LevelEditor::CreateNewLevelMessage() {
	LevelMessage * lm;
	if (editedLevelElement) {
		if (editedLevelElement->levelMessage)
			lm = new LevelMessage(*editedLevelMessage); // Copy over relevant data if available.
		else
			lm = new LevelMessage();
		int index = editedLevel.levelElements.GetIndexOf(editedLevelElement);
		editedLevel.levelElements.Insert(new LevelElement(lm), index + 1); // Insert one slot after the current selection.
		editedLevelElement = editedLevel.levelElements[index + 1];
	}
	else {
		lm = new LevelMessage();
		editedLevel.levelElements.Add(new LevelElement(lm));
		editedLevelElement = editedLevel.levelElements[0];
	}
	Spawn(editedLevelElement, CalculateEditorSpawnTimeFor(editedLevelElement));
	UpdatePositionsLevelElementsAfter(editedLevelElement);
	if (lm)
		QueueGraphics(new GMSetUIt("bottomInfoText", GMUI::TEXT, "New LevelMessage created: " + lm->name));
}

void LevelEditor::LoadMission(Mission * mission, bool force) {

	if (mission == nullptr)
		return;

	if (editedMission == mission && !force) {
		LogMain("Already got mission loaded.", INFO);
		return;
	}
	editedMission = mission;
	levelSavePath = editedMission->levelFilePath;
	LoadLevel(editedMission->levelFilePath);
}

bool LevelEditor::LoadLevel(String fromPath) {
	editedLevel = Level();
	bool success = editedLevel.Load(fromPath);
	if (!success) {
		LogMain("Unable to load level from path " + fromPath + ".", ERROR);
		return false;
	}

	MapMan.DeleteAllEntities();

	//OpenSpawnWindow();


	QueueGraphics(new GMSetUIt("bottomInfoText", GMUI::TEXT, "Editing " + fromPath));


	editedLevel.SetupCamera();
	levelCamera->trackingMode = TrackingMode::NONE;
	levelCamera->smoothness = 0.05f;
	QueueGraphics(new GMSetCamera(levelCamera, CT_ZOOM, 33.0f));

	/// Add entity to track for both the camera, blackness and player playing field.
	levelEntity = LevelEntity->Create(editedLevel.playingFieldSize, playingFieldPadding, false, &editedLevel);
	LevelEntity->SetVelocity(editedLevel.BaseVelocity());

	/// Add emitter for stars at player start.
	ClearStars();
	NewStars(editedLevel.starSpeed.NormalizedCopy(), editedLevel.starSpeed.Length(), 0.2f, editedLevel.starColor);
	LetStarsTrack(levelEntity, Vector3f(editedLevel.playingFieldSize.x + 10.f, 0, 0));

	GraphicsMan.ResumeRendering();
	PhysicsMan.Pause();
	editedLevel.OnEnter();
	

	// Pause all logic.
	PlayingLevel::paused = true;

	// Set global var
	activeLevel = &editedLevel;

	// Spawn entities
	for (int i = 0; i < editedLevel.levelElements.Size(); ++i) {
		LevelElement* le = editedLevel.levelElements[i];
		Spawn(le, CalculateEditorSpawnTimeFor(le));
	}


	// Enable buttons for creating new elements.

	OnLevelLoaded();
	return true;
}

// Update some UI after reading from files.
void LevelEditor::OnLevelLoaded() {

	bool levelLoaded = editedLevel.levelElements.Size() > 0;

	LogMain("OnLevelLoaded: " + String(levelLoaded) + ", elements: " + String(editedLevel.levelElements.Size()), INFO);

	auto mainUI = MainUI();
	QueueGraphics(new GMSetUIb("NewSG", GMUI::ENABLED, levelLoaded, mainUI));
	QueueGraphics(new GMSetUIb("NewLM", GMUI::ENABLED, levelLoaded, mainUI));
	QueueGraphics(new GMSetUIb("DeleteElement", GMUI::ENABLED, levelLoaded, mainUI));
	QueueGraphics(new GMSetUIb("lLevelGeneralData", GMUI::ENABLED, levelLoaded, UIFilter::IncludeChildren, mainUI));
	QueueGraphics(new GMSetUIb("PlayTestLevel", GMUI::ENABLED, levelLoaded, mainUI));
	QueueGraphics(new GMSetUIb("ReloadLevelButton", GMUI::ENABLED, levelLoaded, mainUI));
	QueueGraphics(new GMSetUIb("SaveLevelButton", GMUI::ENABLED, levelLoaded, mainUI));
	QueueGraphics(new GMSetUIb("SaveAsButton", GMUI::ENABLED, levelLoaded, mainUI));

	QueueGraphics(new GMSetUIv4f("StarColor", GMUI::VECTOR_INPUT, editedLevel.starColor, mainUI));
	QueueGraphics(new GMSetUIv3f("StarSpeed", GMUI::VECTOR_INPUT, editedLevel.starSpeed, mainUI));
}


void LevelEditor::OpenSpawnWindow()
{
	if (!spawnWindow)
	{
		spawnWindow = WindowMan.NewWindow("SpawnWindow", "Spawn Window");
		spawnWindow->SetRequestedSize(Vector2i(800, 600));
		spawnWindow->Create();
		UserInterface* ui = spawnUI = spawnWindow->CreateUI();
		QueueGraphics(GMPushUI::ToWindow("gui/SpawnWindow.gui", spawnWindow));
		//ui->Load("gui/SpawnWindow.gui");
		//ui->PushToStack("SpawnWindow");
	}
	spawnWindow->Show();
	spawnWindow->SetAlwaysOnTop();
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

	List<String> spawnGroupMovementPatterns;
	for (int i = 0; i < MovementPattern::movementPatterns.Size(); ++i)
		spawnGroupMovementPatterns.Add(MovementPattern::movementPatterns[i].name);
	QueueGraphics(new GMSetUIContents(spawnUI, "SGMovementPattern", spawnGroupMovementPatterns));
}


void LevelEditor::CloseSpawnWindow()
{
	if (spawnWindow)
		spawnWindow->Close();
}

// Spawns spawn group at appropriate place in the editor for manipulation.
void LevelEditor::Spawn(LevelElement* levelElement, const Time atTime) {
	UpdateWorldEntityForLevelTime(atTime);
	if (levelElement->spawnGroup) {
		// Spawn until it returns false
		while (!levelElement->spawnGroup->Spawn(atTime, nullptr, levelElement));
	}
	else if (levelElement->levelMessage) {
		levelElement->levelMessage->SpawnEditorEntity(levelElement);
	}

}

void LevelEditor::Respawn(LevelElement* levelElement) {
	if (levelElement->spawnGroup) {
		auto sg = levelElement->spawnGroup;
		sg->Despawn();
		sg->ResetForSpawning();
		Spawn(levelElement, CalculateEditorSpawnTimeFor(levelElement));
	}
	else if (levelElement->levelMessage) {
		auto lm = levelElement->levelMessage;
		lm->DespawnEditorEntity();
		Spawn(levelElement, CalculateEditorSpawnTimeFor(levelElement));
	}
}

Time LevelEditor::CalculateEditorSpawnTimeFor(LevelElement* levelElementToCalc) {
	// Positions of levelTime + offsets for each item which had +0 in levelTime adjustment, just to make the easily interactable in the editor.
	Time editorLevelTime(TimeType::MILLISECONDS_NO_CALENDER);
	int minimumMillisecondsBetweenEachGroup = 500;
	float timeMultiplierForEditorTime = 2;
	for (int i = 0; i < editedLevel.levelElements.Size(); ++i) {
		LevelElement& le = *editedLevel.levelElements[i];
		if (le.spawnGroup) {
			SpawnGroup * spawnGroup = le.spawnGroup;
			Time toAdd = SpawnGroup::SpawnTimeFromString(spawnGroup->spawnTimeString, Time(TimeType::MILLISECONDS_NO_CALENDER));
			toAdd.intervals *= timeMultiplierForEditorTime;
			if (toAdd.intervals == 0)
				toAdd.AddMs(minimumMillisecondsBetweenEachGroup);
			editorLevelTime += toAdd;

			if (spawnGroup == levelElementToCalc->spawnGroup)
				return editorLevelTime;
		}
		else if (le.levelMessage) {
			LevelMessage* levelMessage = le.levelMessage;
			if (levelMessage->startTimeOffsetSeconds == 0)
				editorLevelTime.AddMs(minimumMillisecondsBetweenEachGroup);
			else 
				editorLevelTime.AddSeconds(levelMessage->startTimeOffsetSeconds * timeMultiplierForEditorTime);

			if (levelMessage == levelElementToCalc->levelMessage)
				return editorLevelTime;
		}
	}
	return editorLevelTime;
}

Time LevelEditor::PreviousMessageOrSpawnGroupTime(void * comparedToSGorLM) {
	auto levelElements = editedLevel.levelElements;
	for (int i = 0; i < levelElements.Size(); ++i) {
		SpawnGroup * sg = levelElements[i]->spawnGroup;
		LevelMessage * lm = levelElements[i]->levelMessage;

		if (comparedToSGorLM == sg || comparedToSGorLM == lm) {
			// return the previous index, if possible.
			auto previous = levelElements[i - 1];
			if (i >= 1) {
				return previous->SpawnOrStartTime();

			}
		}
	}
	return Time(TimeType::MILLISECONDS_NO_CALENDER);
}


void LevelEditor::UpdatePositionsLevelElementsAfter(int index) {
	if (index < 0)
		index = 0;
	if (index > editedLevel.levelElements.Size())
		return;
	if (editedLevel.levelElements.Size() == 0)
		return;
	index = index % editedLevel.levelElements.Size();
	UpdatePositionsLevelElementsAfter(editedLevel.levelElements[index]);
}

void LevelEditor::UpdatePositionsLevelElementsAfter(LevelElement* levelElement) {
	auto spawnGroups = editedLevel.SpawnGroups();
	for (int i = 0; i < editedLevel.levelElements.Size(); ++i) {
		LevelElement* levelElement = editedLevel.levelElements[i];
		levelElement->InvalidateSpawnTime();
	}

	// Positions of levelTime + offsets for each item which had +0 in levelTime adjustment, just to make the easily interactable in the editor.
	Time editorLevelTime(TimeType::MILLISECONDS_NO_CALENDER);
	for (int i = 0; i < editedLevel.levelElements.Size(); ++i) {
		LevelElement* le = editedLevel.levelElements[i];
		Respawn(le);
	}
}

void LevelEditor::UpdateWorldEntityForLevelTime(Time levelTime) {
	// approx 100 pixels
	levelEntity->worldPosition.x = levelTime.Milliseconds() / 200;
	editedLevel.UpdateSpawnDespawnLimits(levelEntity);
}


/// Emil Hedemalm
/// 2015-06-28
/// Level.

#include "SpawnGroup.h"
#include "Text/TextManager.h"
#include "../SpaceShooter2D.h"
#include "LevelMessage.h"
#include "File/LogFile.h"
#include "PlayingLevel.h"
#include "Test/TutorialTests.h"

Time GetSpawnTime(Time lastMessageOrSpawnGroupTime, int secondsToAdd) {
	Time newTime = lastMessageOrSpawnGroupTime;
	newTime.AddSeconds(secondsToAdd);
	return newTime;
}


LevelMessage::LevelMessage()
	: dontSkip(false)
	, name("")
	, startTimeOffsetSeconds(0)
	, editorEntity(nullptr)
{
	displayed = hidden = false;
	goToTime = startTime = stopTime = Time(TimeType::MILLISECONDS_NO_CALENDER, 0);
	type = LMType::TEXT_MESSAGE;
	eventType = STRING_EVENT;
	textID = "";
	goToRewindPoint = false;
}

void LevelMessage::PrintAll()
{
	std::cout<<"\nType: "<<(type == LMType::TEXT_MESSAGE? "Text message" : "Event");
	std::cout<<"\nTime: "<<startTime.Milliseconds();
	if (type == LMType::TEXT_MESSAGE)
	{
		std::cout<<"\nTextID: "<<textID;
	}
}


int activeLevelDisplayMessages = 0;

// UI
bool LevelMessage::Trigger(PlayingLevel& playingLevel, Level * level)
{
	bool trigger = true;
	if (condition.Length())
	{
		if (condition.StartsWith("TriggeredEvent")) {
			String eventName = condition.Tokenize(":")[1];
			trigger = playingLevel.eventsTriggered.Exists(eventName);
		}
		if (condition == "FailedToDefeatAllEnemies")
		{
			trigger = playingLevel.DefeatedAllEnemiesInTheLastSpawnGroup() == false;
			LogMain("FailedToDefeatAllEnemies: "+ String(trigger), INFO);
		}
		else if (condition == "FailedToSurvive")
		{
			trigger = playingLevel.failedToSurvive;
		}
		else if (condition.Contains("SpaceDebrisNotCollected(")) {
			int target = condition.Tokenize("()")[1].ParseInt();
			trigger = playingLevel.spaceDebrisCollected != target;
			playingLevel.spaceDebrisCollected = 0;
		}
		else {
			LogMain("Bad condition in LevelMessage", WARNING);
		}
		std::cout<<" trigger: "<<trigger;
	}

	// Add it to log of triggered events
	if (trigger) {
		if (name.Length() > 0)
			playingLevel.eventsTriggered.Add(name);
	}

	if (!trigger)
	{
		// Mark it as if it has been displayed and triggered already?
		displayed = hidden = true;
		return false;
	}

	TutorialTests::OnLevelMessageTriggered(this);

	if (type == LMType::TEXT_MESSAGE)
	{
		// o.o uiiii
		if (textID == "-1")
		{
			PrintAll();
		}
		Text text = TextMan.GetText(textID); 		
		if (text.Contains("$")) {
			text.Replace("$Name", playingLevel.PlayerName());
			const InputMapping& inputMapping = playingLevel.InputMapping();
			String movementKeys, proceedKeys, shootKeys, selectWeaponKeys, toggleWeaponScriptKeys;
			for (int i = 0; i < inputMapping.bindings.Size(); ++i) {
				auto binding = inputMapping.bindings[i];
				if (binding->name == "MoveShipUp")
					movementKeys.Add(binding->KeysToTriggerItToString());
				if (binding->name == "MoveShipDown")
					movementKeys.Add(binding->KeysToTriggerItToString());
				if (binding->name == "MoveShipLeft")
					movementKeys.Add(binding->KeysToTriggerItToString());
				if (binding->name == "MoveShipRight")
					movementKeys.Add(binding->KeysToTriggerItToString());
				if (binding->name == "ProceedMessage")
					proceedKeys = binding->KeysToTriggerItToString();
				if (binding->name.Contains("Shooting"))
					shootKeys = binding->KeysToTriggerItToString();
				if (binding->name.Contains("Weapon:"))
					selectWeaponKeys += binding->KeysToTriggerItToString();
				if (binding->name.Contains("ToggleWeaponScript"))
					toggleWeaponScriptKeys = binding->KeysToTriggerItToString();
			}

			String inputColorStart = "$color(0.4)";
			String colorReset = "$color(reset)";
			text.Replace("$MovementKeys", inputColorStart + movementKeys + colorReset);
			text.Replace("$ProceedKey", inputColorStart + proceedKeys + colorReset);
			text.Replace("$ShootKey", inputColorStart + shootKeys + colorReset);
			text.Replace("$SelectWeaponKeys", inputColorStart + selectWeaponKeys + colorReset);
			text.Replace("$ToggleWeaponScriptKeys", inputColorStart + toggleWeaponScriptKeys + colorReset);

			// Convert the text to actual color markers before submitting anything..!
			text.ConvertColorMarkers();
		}

		QueueGraphics(new GMSetUIt("LevelMessage", GMUI::TEXT, text));
		QueueGraphics(new GMSetUIb("LevelMessage", GMUI::VISIBILITY, true));
		displayed = true;
		++activeLevelDisplayMessages;
	}
	else if (type == LMType::EVENT)
	{
		displayed = true;
		if (string.Length())
			MesMan.QueueMessages(string);
		if (goToTime.intervals != 0)
		{
			level->SetTime(goToTime);
			LogMain("Jumping to time: "+String(goToTime.Seconds()), INFO);
			return false; // Return as if it failed, so the event is not saved as currently active one. (instantaneous)
		}
		if (goToRewindPoint) {
			level->SetTime(playingLevel.rewindPoint);
			LogMain("Rewinding to time: " + String(playingLevel.rewindPoint.Seconds()), INFO);
			return false; // Return as if it failed, so the event is not saved as a currently active one. (instantaneous).
		}
	}
	return true;
}

void LevelMessage::Hide(PlayingLevel& playingLevel)
{
	if (type == LMType::TEXT_MESSAGE)
	{
		--activeLevelDisplayMessages;
		if (activeLevelDisplayMessages <= 0)
		{
			QueueGraphics(new GMSetUIb("LevelMessage", GMUI::VISIBILITY, false));
		}
		displayed = false;
	}
	hidden = true;
}

String LevelMessage::GetEditorText(int maxChars) {
	String toSet = textID;
	if (toSet.Length() == 0 && string.Length()) {
		toSet = string;
	}
	else if (goToRewindPoint) {
		toSet = "Go to rewind point";
	}

	// Shorten as needed.
	if (toSet.Length() > maxChars)
		toSet = toSet.Part(0, maxChars - 3) + "...";

	if (condition.Length() > 0)
		toSet = "(IF ...) ";

	toSet.ToUpperCase();
	return toSet;
}

LevelMessageProperty::LevelMessageProperty(Entity* owner, LevelMessage* levelMessage, LevelElement* levelElement)
	: EntityProperty("LevelMessageProp", ID, owner)
	, levelMessage(levelMessage)
	, levelElement(levelElement)
{
	// Set up entity scale, text to be rendered, etc. based on the LevelMessage.
	QueuePhysics(new PMSetEntity(owner, PT_SET_SCALE, Vector3f(2.0f, 20.f, 2.f)));
	QueuePhysics(new PMSetEntity(owner, PT_PHYSICS_SHAPE, ShapeType::AABB));

	// Events = Yellow
	// Text = Gray
	ResetColor();

	String toSet = levelMessage->GetEditorText(12);

	QueueGraphics(new GMSetEntitys(owner, GT_TEXT, toSet));
	QueueGraphics(new GMSetEntityf(owner, GT_TEXT_SIZE_RATIO, 0.3f));
	QueueGraphics(new GMSetEntityVec4f(owner, GT_TEXT_POSITION, Vector3f(0, int(owner->localPosition.x) % 20 - 10, 0)));
}

void LevelMessageProperty::ProcessMessage(Message * message) {
	if (message->msg == "ResetColor") {
		ResetColor();
	}
}
void LevelMessageProperty::ResetColor() {
	if (levelMessage->type == LMType::EVENT)
		QueueGraphics(new GMSetEntityVec4f(owner, GT_COLOR, Vector4f(1, 1, 0, 1)));
	else
		QueueGraphics(new GMSetEntityVec4f(owner, GT_COLOR, Vector4f(0.5f, 0.5f, 0.5f, 1)));
}

void LevelMessage::SpawnEditorEntity(LevelElement* levelElement) {
	Vector3f position = Vector3f(PlayingLevelRef().spawnPositionRight, 0, 0);
	this->editorPosition = position;
	editorEntity = MapMan.CreateEntity("LevelMessageEntity", ModelMan.GetModel("cube"), TexMan.GetTexture("0xFFFF"), position);
	editorEntity->localPosition = position; // Set position so it can be used to calc text rendering in next step.
	editorEntity->properties.Add(new LevelMessageProperty(editorEntity, this, levelElement));
}
void LevelMessage::DespawnEditorEntity() {
	if (editorEntity)
		MapMan.DeleteEntity(editorEntity);
	editorEntity = nullptr;
}

void LevelMessage::ResetEditorEntityColor() {
	Message message("ResetColor");
	editorEntity->ProcessMessage(&message);
}


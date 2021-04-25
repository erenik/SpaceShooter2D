/// Emil Hedemalm
/// 2015-06-28
/// Level.

#ifndef LEVEL_MSG_H
#define LEVEL_MSG_H

#include "Base/Ship.h"
#include "Color.h"

class Level;

// LevelMessage-type
enum class LMType {
	TEXT_MESSAGE,
	EVENT,
};

struct LevelElement;

Time GetSpawnTime(Time lastMessageOrSpawnGroupTime, int secondsToAdd);

class LevelMessage 
{
public:
	LevelMessage();
	void PrintAll(); // debug
	// UI Returns true if it was displayed, false if skipped (condition false).
	bool Trigger(PlayingLevel& playingLevel, Level * level);
	void Hide(PlayingLevel& playingLevel);
	
	String name;
	
	/// Set and used only in the editor.
	Vector3f editorPosition;
	// Depends on type, and stuff.
	String GetEditorText(int maxChars);

	void SpawnEditorEntity(LevelElement* levelElement);
	void DespawnEditorEntity();
	void ResetEditorEntityColor();

	String condition;
	String string;
	LMType type;
	int eventType;
	enum {
		STRING_EVENT,
		GO_TO_TIME_EVENT,
		GO_TO_REWIND_POINT,
	};
	bool displayed, hidden;
	
	// # of seconds after previous spawn group or message that this message should be presented. 
	int startTimeOffsetSeconds;

	// In LevelTime
	Time startTime;
	Time stopTime;
	String textID;
	Time goToTime;
	bool goToRewindPoint;
	// If true, will trigger even when jumping forward in time while testing.
	bool dontSkip;

	Entity* editorEntity;

private:
};

#include "Entity/EntityProperty.h"

struct LevelElement;

class LevelMessageProperty : public EntityProperty {
public:
	static const int ID = EntityPropertyID::CUSTOM_GAME_1;
	LevelMessageProperty(Entity* owner, LevelMessage * levelMessage, LevelElement* levelElement);
	void ProcessMessage(Message * message) override;
	void ResetColor();
	LevelMessage * levelMessage;
	LevelElement* levelElement;
};

#endif // LEVEL_MSG_H
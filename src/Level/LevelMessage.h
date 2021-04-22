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

	void SpawnEditorEntity();
	void DespawnEditorEntity();

	String condition;
	List<String> strings;
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

private:
	Entity* editorEntity;
};

#include "Entity/EntityProperty.h"

class LevelMessageProperty : public EntityProperty {
public:
	LevelMessageProperty(Entity* owner, LevelMessage * levelMessage);
	LevelMessage * levelMessage;
};

#endif // LEVEL_MSG_H
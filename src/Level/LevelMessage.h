/// Emil Hedemalm
/// 2015-06-28
/// Level.

#ifndef LEVEL_MSG_H
#define LEVEL_MSG_H

#include "Base/Ship.h"
#include "Color.h"

class Level;

class LevelMessage 
{
public:
	LevelMessage();
	void PrintAll(); // debug
	// UI Returns true if it was displayed, false if skipped (condition false).
	bool Trigger(PlayingLevel& playingLevel, Level * level);
	void Hide(PlayingLevel& playingLevel);
	
	String name;
	enum {
		TEXT_MESSAGE,
		EVENT,
	};
	String condition;
	List<String> strings;
	int type; 
	int eventType;
	enum {
		STRING_EVENT,
		GO_TO_TIME_EVENT,
		GO_TO_REWIND_POINT,
	};
	bool displayed, hidden;
	Time startTime;
	Time stopTime;
	String textID;
	Time goToTime;
	bool goToRewindPoint;
	// If true, will trigger even when jumping forward in time while testing.
	bool dontSkip;
};

#endif // LEVEL_MSG_H
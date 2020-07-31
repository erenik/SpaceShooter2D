/// Emil Hedemalm
/// 2015-06-28
/// Level.

#include "SpawnGroup.h"
#include "Text/TextManager.h"
#include "../SpaceShooter2D.h"
#include "LevelMessage.h"
#include "File/LogFile.h"
#include "PlayingLevel.h"

LevelMessage::LevelMessage()
{
	displayed = hidden = false;
	goToTime = startTime = stopTime = Time(TimeType::MILLISECONDS_NO_CALENDER, 0);
	type = TEXT_MESSAGE;
	eventType = STRING_EVENT;
	textID = "";
	goToRewindPoint = false;
}

void LevelMessage::PrintAll()
{
	std::cout<<"\nType: "<<(type == TEXT_MESSAGE? "Text message" : "Event");
	std::cout<<"\nTime: "<<startTime.Milliseconds();
	if (type == TEXT_MESSAGE)
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
	if (!trigger)
	{
		// Mark it as if it has been displayed and triggered already?
		displayed = hidden = true;
		return false;
	}


	if (type == LevelMessage::TEXT_MESSAGE)
	{
		// o.o uiiii
		if (textID == "-1")
		{
			PrintAll();
		}
		Text text = TextMan.GetText(textID); 
		text.Replace("$Name", playingLevel.playerName);
		QueueGraphics(new GMSetUIs("LevelMessage", GMUI::TEXT, text));
		QueueGraphics(new GMSetUIb("LevelMessage", GMUI::VISIBILITY, true));
		displayed = true;
		++activeLevelDisplayMessages;
	}
	else if (type == LevelMessage::EVENT)
	{
		displayed = true;
		if (strings.Size())
			MesMan.QueueMessages(strings);
		if (goToTime.intervals != 0)
		{
			level->SetTime(goToTime);
			LogMain("Jumping to time: "+String(goToTime.Seconds()), INFO);
			return false; // Return as if it failed, so the event is not saved as currently active one. (instantaneous)
		}
		if (goToRewindPoint) {
			level->SetTime(playingLevel.rewindPoint);
			LogMain("Rewinding to time: " + String(playingLevel.rewindPoint.Seconds()), INFO);
		}
	}
	return true;
}

void LevelMessage::Hide(PlayingLevel& playingLevel)
{
	if (type == LevelMessage::TEXT_MESSAGE)
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

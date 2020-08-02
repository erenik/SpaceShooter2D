/// Emil Hedemalm
/// 2015-01-21
/// Level.

#include "../SpaceShooter2D.h"
#include "SpawnGroup.h"
#include "Text/TextManager.h"
#include "LevelMessage.h"
#include "File/LogFile.h"
#include "PlayingLevel.h"

#define SET_GROUP_DEFAULTS { group->pausesGameTime = loadLevel->spawnGroupsPauseGameTime; }

namespace LevelLoader 
{
	LevelMessage * lastMessage = nullptr;
	LevelMessage * message = NULL;
	SpawnGroup * group = (SpawnGroup*)0;
	Level * loadLevel = (Level*)0;
	SpawnGroup * lastGroup = (SpawnGroup*)0;

	void Init() {
		lastMessage = nullptr;
		message = nullptr;
		group = nullptr;
		lastGroup = nullptr;
		loadLevel = nullptr;
	}

	/// Additional spawn times for duplicates of the same group (times specified at start)
	List<Time> spawnTimes;

	Vector3i goalColor;

	List<ShipColorCoding> colorCodings;
	List<String> lines;

	enum {
		PARSE_MODE_INIT,
		PARSE_MODE_FORMATIONS,
		PARSE_MODE_MESSAGES,
	};
	int parseMode = 0;

	Time LastSpawnGroupOrMessageTime();

	// AddMessageIfNeeded(this)
	void AddMessageIfNeeded(Level * level) {
		if (message) { 
			String prefix = "Message";
			if (message->type == LevelMessage::EVENT)
				prefix = "Event";
			LogMain(prefix+" "+ message->textID+" added at time: "+ message->startTime.ToString("m:S"), INFO);
			level->messages.Add(message); 
			lastMessage = message;
			message = nullptr;
		} 
	}


	void ParseMessageStartTime(String arg) {
		message->startTime = LastSpawnGroupOrMessageTime();
		if (arg.StartsWith("+")) {
			message->startTime.AddSeconds(arg.Tokenize(":")[1].ParseInt());
		}
		else if (arg.IsNumber())
			message->startTime.ParseFrom(arg);
		LogMain("Message start time set to: " + message->startTime.ToString("m:S"), INFO);
		message->stopTime = message->startTime + Time(TimeType::MILLISECONDS_NO_CALENDER, 5000); // Default 5 seconds?
	}

	// Returns the most recent time parsed so far, regardless if Spawngroup or message/event.
	Time LastSpawnGroupOrMessageTime() {
		Time lastSpawnGroupTime = lastGroup ? lastGroup->SpawnTime() : Time(TimeType::MILLISECONDS_NO_CALENDER);
		Time lastMessageTime = lastMessage ? lastMessage->startTime : Time(TimeType::MILLISECONDS_NO_CALENDER);
		Time messageTime = message ? message->startTime : Time(TimeType::MILLISECONDS_NO_CALENDER);
		Time spawnGroupTime = group ? group->SpawnTime() : Time(TimeType::MILLISECONDS_NO_CALENDER);

		Time bestSoFar = lastSpawnGroupTime;
		if (lastMessageTime > bestSoFar)
			bestSoFar = lastMessageTime;
		if (messageTime > bestSoFar)
			bestSoFar = messageTime;
		if (spawnGroupTime > bestSoFar)
			bestSoFar = spawnGroupTime;
		return bestSoFar;
	}

	void AddGroupsIfNeeded() 
	{ 
		if (group) {
			lastGroup = group; 
			LogMain("Setting spawn time to group from "+group->SpawnTime().ToString("m:S.n")+" to "+spawnTimes[0].ToString("m:S.n"), DEBUG);
			group->SetSpawnTime(spawnTimes[0]); // Set spawn time if not already done so.
			if (group->name.Length() == 0)
				group->name = group->SpawnTime().ToString("m:S.n");
			loadLevel->spawnGroups.AddItem(group);
			LogMain("SpawnGroup "+String(loadLevel->spawnGroups.Size()+1)+" added "+group->name+"\t"+group->SpawnTime().ToString("m:S.n"), DEBUG);
		} 
		for (int p = 1; p < spawnTimes.Size(); ++p)
		{
			SpawnGroup * newGroup = new SpawnGroup(*lastGroup);
			SET_GROUP_DEFAULTS;
			newGroup->SetSpawnTime(spawnTimes[p]);
			newGroup->name = lastGroup->name + "_"+String(p+1);
			loadLevel->spawnGroups.AddItem(newGroup);
			LogMain("SpawnGroup "+String(loadLevel->spawnGroups.Size()+1)+" added "+newGroup->name+"\t"+newGroup->SpawnTime().ToString("m:S.n"), DEBUG);
		}
		spawnTimes.Clear();
		group = NULL;
	}
	void ParseTimeStringsFromLine(String line)
	{
		spawnTimes.Clear();
		List<String> timeStrs = line.Tokenize(" \t,");
		timeStrs.RemoveIndex(0, ListOption::RETAIN_ORDER);
		for (int i = 0; i < timeStrs.Size(); ++i)
		{
			String timeStr = timeStrs[i];
			if (timeStr.StartsWith("x"))
			{
				int duplicates = timeStr.Tokenize("x")[0].ParseInt();
				// Since previous one is already there, add it 11 more times.
				String timeStrPrev = timeStrs[i-1];
				for (int j = 1; j < duplicates; ++j)
					timeStrs.AddItem(timeStrPrev);
				/// Parse them as usual next iterations.
				continue;
			}
			Time t(TimeType::MILLISECONDS_NO_CALENDER);
			if (timeStr.StartsWith("+"))
			{
				Time lastTime = LastSpawnGroupOrMessageTime();
				t = lastTime; 
				t.AddSeconds(timeStr.Tokenize(":")[1].ParseInt());
			}
			else {
				bool ok = t.ParseFrom(timeStr);
				if (!ok)
					continue;
			}
			LogMain("Adding spawn group at time " + t.ToString("m:S"), INFO);
			/*			if (t.Milliseconds() == 0) /// Spawning at 0:0.0 should be possible - used in generator.
				continue;*/
			spawnTimes.AddItem(t);
		}
		assert(spawnTimes.Size());
	}
};

using namespace LevelLoader;

/*
#define	ADD_GROUPS_IF_NEEDED { if (group) {\ 
			lastGroup = group; \
			spawnGroups.Add(group);\
		} \
		for (int p = 1; p < spawnTimes.Size(); ++p){\

		}\
		group = NULL;\
	}
*/

int Level::SpawnGroupsActive() {
	int numActive = 0;
	for (int i = 0; i < spawnGroups.Size(); ++i) {
		SpawnGroup * spawnGroup = spawnGroups[i];
		if (spawnGroup->shipsSpawned == 0) // Ignore those not yet spawned.
			continue;
		if (spawnGroup->ShipsActive() > 0) 
			++numActive;
	}
	return numActive;
}

/// Deletes all ships, spawngroups, resets variables to defaults.
void Level::Clear(PlayingLevel& playingLevel)
{
	this->endCriteria = Level::NEVER;
	this->RemoveRemainingSpawnGroups();
	this->RemoveExistingEnemies(playingLevel);
}

bool Level::FinishedSpawning()
{
	for (int i = 0; i < spawnGroups.Size(); ++i)
	{
		SpawnGroup * sg = spawnGroups[i];
		if (!sg->FinishedSpawning())
			return false;
	}
	return true;
}

bool Level::Load(String fromSource)
{
	LevelLoader::Init();

	// Clear old stuff
	Clear(PlayingLevelRef());
	LevelLoader::loadLevel = this;
	
	source = fromSource;

	// Reset level stats.
	spaceShooter->ResetLevelStats();


	/// Clear old stuff.
	ships.Clear();
	spawnGroups.ClearAndDelete();
	messages.ClearAndDelete();

	/// Set end criteria..
	endCriteria = Level::NO_MORE_ENEMIES;

	String sourceTxt = source;
	source+".ogg";
	// music = 
	lines = File::GetLines(sourceTxt);

	if (lines.Size() == 0)
	{
		LogMain("Unable to read any lines of text from level source: "+sourceTxt, ERROR);
	}
	bool inComments = false;

	for (int i = 0; i < lines.Size(); ++i)
	{
		String line = lines[i];
		line.SetComparisonMode(String::NOT_CASE_SENSITIVE);
		// Formation specific parsing.
		List<String> tokens = line.Tokenize(" ()\t");
		if (tokens.Size() == 0)
			continue;
		String var, arg, arg2, parenthesisContents;
		if (tokens.Size() > 0)
			 var = tokens[0];
		if (tokens.Size() > 1)
			arg = tokens[1];
		if (tokens.Size() > 2)
			arg2 = tokens[2];
		// o.o
		if (line.StartsWith("StopLoading"))
			break;
		if (line.StartsWith("//"))
			continue;
		if (line.Contains("/*"))
			inComments = true;
		else if (line.Contains("*/"))
			inComments = false;
		if (inComments)
			continue;
		if (line.StartsWith("SpawnGroupsPauseGameTime"))
			spawnGroupsPauseGameTime = arg.ParseBool();
		else if (line.StartsWith("SpawnGroup"))
		{
			AddMessageIfNeeded(loadLevel);
			AddGroupsIfNeeded();
			group = new SpawnGroup();
			SET_GROUP_DEFAULTS;
			// Parse time.
			ParseTimeStringsFromLine(line);
			group->SetSpawnTime(spawnTimes[0]);
	//		group->spawnTime.PrintData();
//			String timeStr = line.Tokenize(" \t")[1];
//			group->spawnTime.ParseFrom(timeStr);
			parseMode = PARSE_MODE_FORMATIONS;
		}
		else if (line.StartsWith("MessagesPauseGameTime"))
		{
			messagesPauseGameTime = arg.ParseBool();
		}
		else if (line.StartsWith("PlayBGM:"))
		{
			music = line;
			music.Remove("PlayBGM:");
			music.RemoveSurroundingWhitespaces();
		}
		else if (line.StartsWith("Message"))
		{
			AddMessageIfNeeded(this);
			message = new LevelMessage();
			ParseMessageStartTime(arg);
			parseMode = PARSE_MODE_MESSAGES;
		}
		else if (line.StartsWith("Event"))
		{
			AddMessageIfNeeded(this);
			message = new LevelMessage();
			message->type = LevelMessage::EVENT;
			message->strings.Add(line - "Event "); // By default, add argument to be invoked on passing it.
			message->textID = arg; // By default, for logging, add arg as textId.
			ParseMessageStartTime(arg);
			parseMode = PARSE_MODE_MESSAGES;
		}
		if (parseMode == PARSE_MODE_MESSAGES)
		{
			if (var == "Condition") {
				message->condition = line - "Condition";
				message->condition.RemoveSurroundingWhitespaces();
			}
			if (var == "TextID")
				message->textID = arg;
			if (var == "String")
			{
				String strArg = line - "String";
				strArg.RemoveSurroundingWhitespaces();
				message->strings.AddItem(strArg);
			}
			if (var == "DontSkip")
			{
				message->dontSkip = true;
			}
			if (var == "GoToTime")
			{
				message->eventType = LevelMessage::GO_TO_TIME_EVENT;
				String strArg = line - "GoToTime";
				strArg.RemoveSurroundingWhitespaces();
				message->goToTime.ParseFrom(strArg);
			}
			if (var == "GoToRewindPoint") {
				message->eventType = LevelMessage::GO_TO_TIME_EVENT;
				message->goToRewindPoint = true;
			}
		}
		if (parseMode == PARSE_MODE_FORMATIONS)
		{
			// Grab parenthesis
			tokens = line.Tokenize("()");
			if (tokens.Size() > 1)
				parenthesisContents = tokens[1];
			if (var == "CopyGroup")
			{
				AddGroupsIfNeeded();
				// Copy last one.
				group = new SpawnGroup(*lastGroup);
				SET_GROUP_DEFAULTS;
				// Parse time.
				ParseTimeStringsFromLine(line);
				group->SetSpawnTime(spawnTimes[0]);
				parseMode = PARSE_MODE_FORMATIONS;
			}
			if (var == "CopyNamedGroup")
			{
				AddGroupsIfNeeded();
				// Copy last one.
				SpawnGroup * named = 0;
				List<String> tokens = line.Tokenize(" \t");
				assert(tokens.Size() >= 3);
				if (tokens.Size() < 3)
				{
					LogMain("Lacking arguments in CopyNamedGroup at line "+String(i)+" in file "+fromSource, ERROR);
					continue;
				}
				String name = tokens[1];
				for (int j = 0; j < spawnGroups.Size(); ++j)
				{
					SpawnGroup * sg = spawnGroups[j];
					if (sg->name == name)
					{
						named = sg;
						break;
					}
				}
				assert(named);
				if (!named)
				{
					LogMain("Unable to find group by name "+name+" for CopyNamedGroup at line "+String(i)+" in file "+fromSource, ERROR);
					continue;
				}
				group = new SpawnGroup(*named);
				SET_GROUP_DEFAULTS;
				// Parse time.
				ParseTimeStringsFromLine(line);
				group->SetSpawnTime(spawnTimes[0]);
				parseMode = PARSE_MODE_FORMATIONS;
			}
			if (var == "RelativeSpeed")
				group->relativeSpeed = arg.ParseFloat();
			if (var == "Shoot")
				group->shoot = arg.ParseBool();
			if (var == "Name")
				group->name = arg;
			if (var == "SpawnTime")
				group->SetSpawnTime(ParseTimeFrom(arg));
			if (var == "Position")
				group->position.ParseFrom(line - "Position");
			if (var == "ShipType")
				group->shipType = arg;
			if (var == "TimeBetweenShipSpawnsMs")
				group->spawnIntervalMsBetweenEachShipInFormation = arg.ParseInt();
			if (var == "Formation")
			{
//				arg.PrintData();
				group->ParseFormation(arg);
			}
			if (var == "Number" || var == "Amount")
				group->number = arg.ParseInt();
			if (var == "Size")
				group->size.ParseFrom((line - "Size"));
			continue;
		}
		if (line.StartsWith("ShipType"))
		{
			List<String> tokens = line.Tokenize(" ");
			ShipColorCoding newCode;
			if (tokens.Size() < 2)
				continue;
			newCode.ship = tokens[1];
			if (tokens.Size() < 3)
				continue;
			assert(tokens[2] == "RGB");
			if (tokens.Size() < 6)
			{
				std::cout<<"ERrror";
				continue;
			}
			newCode.color[0] = tokens[3].ParseInt();
			newCode.color[1] = tokens[4].ParseInt();
			newCode.color[2] = tokens[5].ParseInt();
			colorCodings.Add(newCode);
		}
		else if (line.StartsWith("Goal"))
		{
			List<String> tokens = line.Tokenize(" ");
			if (tokens.Size() < 5){std::cout<<"\nError"; continue;}
			goalColor[0] = tokens[2].ParseInt();
			goalColor[1] = tokens[3].ParseInt();
			goalColor[2] = tokens[4].ParseInt();
		}
		else if (line.StartsWith("StarSpeed"))
		{
			String vector = line - "StarSpeed";
			starSpeed.ParseFrom(vector);
		}
		else if (line.StartsWith("StarColor"))
		{
			String vector = line - "StarColor";
			starColor.ParseFrom(vector);
		}
	}
	// Add last group, if needed.
	AddGroupsIfNeeded();
	AddMessageIfNeeded(this);

	/// Sort groups based on spawn-time?

	// Sort messages based on time?

	// No gravity
	PhysicsMan.QueueMessage(new PMSet(PT_GRAVITY, Vector3f(0,0,0)));

	
	for (int i = 0; i < messages.Size(); ++i)
	{
	//	messages[i]->PrintAll();
	}

	// Add player? - Done later via script
	return true;
}


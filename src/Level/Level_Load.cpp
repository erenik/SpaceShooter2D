/// Emil Hedemalm
/// 2015-01-21
/// Level.

#include "Text/TextManager.h"
#include "LevelMessage.h"
//#include "File/LogFile.h"

// Gamespecific
#include "../SpaceShooter2D.h"
#include "SpawnGroup.h"
#include "PlayingLevel.h"
#include "LevelElement.h"

#include "File/LogFile.h"

namespace LevelLoader 
{
	LevelMessage * lastMessage = nullptr;
	LevelMessage * message = NULL;
	SpawnGroup * group = (SpawnGroup*)0;
	Level * loadLevel = (Level*)0;
	SpawnGroup * lastGroup = (SpawnGroup*)0;
	int lineNumber = 0;

	void Init() {
		lastMessage = nullptr;
		message = nullptr;
		group = nullptr;
		lastGroup = nullptr;
		loadLevel = nullptr;
	}

	/// Additional spawn times for duplicates of the same group (times specified at start)
	List<String> spawnTimeStrings;

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
			if (message->type == LMType::EVENT)
				prefix = "Event";
			LogMain(prefix + " " + message->textID + " added at time: " + message->startTime.ToString("m:S"), INFO);
			level->levelElements.Add(new LevelElement(message));
			lastMessage = message;
			message = nullptr;
		}
	}

	void AddGroupsIfNeeded()
	{
		if (group) {
			lastGroup = group;
			LogMain("Setting spawn time to group from " + group->spawnTimeString + " to " + spawnTimeStrings[0], DEBUG);
			group->SetSpawnTimeString(spawnTimeStrings[0], LastSpawnGroupOrMessageTime()); // Set spawn time if not already done so.
			if (group->name.Length() == 0)
				group->name = group->SpawnTime().ToString("m:S.n");
			loadLevel->AddSpawnGroup(group);
			LogMain("SpawnGroup " + String(loadLevel->SpawnGroups().Size() + 1) + " added " + group->name + "\t" + group->SpawnTime().ToString("m:S.n"), DEBUG);
		}
		for (int p = 1; p < spawnTimeStrings.Size(); ++p)
		{
			SpawnGroup * newGroup = new SpawnGroup(*lastGroup);
			SetGroupDefaults(newGroup);
			newGroup->SetSpawnTimeString(spawnTimeStrings[p], LastSpawnGroupOrMessageTime());
			newGroup->name = lastGroup->name + "_" + String(p + 1);
			loadLevel->AddSpawnGroup(newGroup);
			LogMain("SpawnGroup " + String(loadLevel->SpawnGroups().Size() + 1) + " added " + newGroup->name + "\t" + newGroup->SpawnTime().ToString("m:S.n"), DEBUG);
		}
		spawnTimeStrings.Clear();
		group = NULL;
	}

	void AddMessageOrSpawnGroupIfNeeded(Level* level) {
		AddMessageIfNeeded(level);
		AddGroupsIfNeeded();
	}

	void SetGroupDefaults(SpawnGroup * sg) {
		sg->lineNumber = lineNumber; // What line it corresponds to in the file :)
		sg->pausesGameTime = loadLevel->spawnGroupsPauseGameTime;
	}


	void ParseMessageStartTime(String arg) {
		message->startTime = LastSpawnGroupOrMessageTime();
		if (arg.StartsWith("+")) {
			message->startTimeOffsetSeconds = arg.Tokenize(":")[1].ParseInt();
			message->startTime.AddSeconds(message->startTimeOffsetSeconds);
		}
		else if (arg.IsNumber()) {
			message->startTimeOffsetSeconds = arg.ParseInt();
			message->startTime.AddSeconds(message->startTimeOffsetSeconds);
		}
		//else if (arg.IsNumber())
		//			message->startTime.ParseFrom(arg);
		else {
			// Auto-increment at least 100 ms
			message->startTime.AddMs(100);
		}
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
		if (spawnGroupTime.Type() != TimeType::UNDEFINED) {
			if (spawnGroupTime > bestSoFar)
				bestSoFar = spawnGroupTime;
		}
		return bestSoFar;
	}

	void ParseTimeStringsFromLine(String line)
	{
		spawnTimeStrings.Clear();
		List<String> tokens = line.Tokenize(" \t,");
		String firstToken = tokens[0];
		// Remove the first word
		tokens.RemoveIndex(0, ListOption::RETAIN_ORDER);
		// If CopyNamedGroup, remove the 2nd one as well.
		if (firstToken == "CopyNamedGroup")
			tokens.RemoveIndex(0, ListOption::RETAIN_ORDER);
		for (int i = 0; i < tokens.Size(); ++i)
		{
			String token = tokens[i];
			if (token.StartsWith("x"))
			{
				int duplicates = token.Tokenize("x")[0].ParseInt();
				// Since previous one is already there, add it 11 more times.
				String timeStrPrev = tokens[i-1];
				for (int j = 1; j < duplicates; ++j)
					tokens.AddItem(timeStrPrev);
				/// Parse them as usual next iterations.
				continue;
			}
			if (token == "")
			LogMain("Adding spawn group at time " + token, INFO);
			/*			if (t.Milliseconds() == 0) /// Spawning at 0:0.0 should be possible - used in generator.
				continue;*/
			spawnTimeStrings.AddItem(token);
		}
		assert(spawnTimeStrings.Size());
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
	for (int i = 0; i < SpawnGroups().Size(); ++i) {
		SpawnGroup * spawnGroup = SpawnGroups()[i];
		if (spawnGroup->shipsSpawned == 0) // Ignore those not yet spawned.
			continue;
		if (spawnGroup->ShipsActive() > 0) 
			++numActive;
	}
	return numActive;
}

void Level::AddSpawnGroup(SpawnGroup* sg) {
	levelElements.Add(new LevelElement(sg));
}
void Level::AddMessage(LevelMessage* lm) {
	levelElements.Add(new LevelElement(lm));
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
	for (int i = 0; i < SpawnGroups().Size(); ++i)
	{
		SpawnGroup * sg = SpawnGroups()[i];
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
	for (int i = 0; i < levelElements.Size(); ++i) {
		SAFE_DELETE(levelElements[i]->spawnGroup);
		SAFE_DELETE(levelElements[i]->levelMessage);
	}
	levelElements.ClearAndDelete();

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
		LevelLoader::lineNumber = i + 1;
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
			AddMessageOrSpawnGroupIfNeeded(loadLevel);
			group = new SpawnGroup();
			group->lineNumber = i + 1; // What line it corresponds to in the file :)
			SetGroupDefaults(group);
			// Parse time.
			ParseTimeStringsFromLine(line);
			group->SetSpawnTimeString(spawnTimeStrings[0], LastSpawnGroupOrMessageTime());
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
			AddMessageOrSpawnGroupIfNeeded(loadLevel);
			message = new LevelMessage();
			ParseMessageStartTime(arg);
			parseMode = PARSE_MODE_MESSAGES;
		}
		else if (line.StartsWith("Event"))
		{
			AddMessageOrSpawnGroupIfNeeded(loadLevel);
			message = new LevelMessage();
			message->type = LMType::EVENT;
			
			// If the line contains a time addition, then don't add the rest as a string, it will be added later.
			String lineWithoutEvent = (line - "Event");
			lineWithoutEvent.RemoveSurroundingWhitespaces();
			if (lineWithoutEvent.Length() > 0) {
				if (!line.Contains("+")) {
					message->string = lineWithoutEvent; // By default, add argument to be invoked on passing it.
				}
			}			

			//message->textID = arg; // By default, for logging, add arg as textId. what?
			ParseMessageStartTime(arg);
			parseMode = PARSE_MODE_MESSAGES;
		}
		if (parseMode == PARSE_MODE_MESSAGES)
		{
			if (var == "Name") {
				message->name = line - "Name";
				message->name.RemoveSurroundingWhitespaces();
			}
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
				message->string = strArg;
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
				AddMessageOrSpawnGroupIfNeeded(loadLevel);
				// Copy last one.
				group = new SpawnGroup(*lastGroup);
				SetGroupDefaults(group);
				// Parse time.
				ParseTimeStringsFromLine(line);
				group->SetSpawnTimeString(spawnTimeStrings[0], LastSpawnGroupOrMessageTime());
				parseMode = PARSE_MODE_FORMATIONS;
			}
			if (var == "CopyNamedGroup")
			{
				AddMessageOrSpawnGroupIfNeeded(loadLevel);
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
				for (int j = 0; j < SpawnGroups().Size(); ++j)
				{
					SpawnGroup * sg = SpawnGroups()[j];
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
				SetGroupDefaults(group);
				// Parse time.
				ParseTimeStringsFromLine(line);
				group->SetSpawnTimeString(spawnTimeStrings[0], LastSpawnGroupOrMessageTime());
				parseMode = PARSE_MODE_FORMATIONS;
			}
			if (var == "RelativeSpeed")
				group->relativeSpeed = arg.ParseFloat();
			if (var == "Shoot")
				group->shoot = arg.ParseBool();
			if (var == "Name")
				group->name = arg;
			if (var == "SpawnTime") {
				Time spawnTime;
				spawnTime.ParseFrom(arg);
				group->SetSpawnTime(spawnTime);
			}
			if (var == "Movement")
				group->movementPattern = MovementPattern::ByName(arg);
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


	/*
	for (int i = 0; i < messages.Size(); ++i)
	{
		messages[i]->PrintAll();
	}
	*/

	// Add player? - Done later via script
	return true;
}

bool Level::Save(String toFile) {
	// Write initial stuff like StarColor, level size

	// Start writing Events, Messages and SpawnGroups in the order they came, stepping 10ms per loop.
	Time levelTime = Time(TimeType::MILLISECONDS_NO_CALENDER);

	List<LevelElement*> levelElementsToWrite = this->levelElements;

	String toWrite;

	File outFile(toFile);
	bool ok = outFile.OpenForWritingText();
	if (!ok) {
		LogMain("Failed to save level", ERROR);
		return false;
	}
	outFile.WriteLine("// Auto-generated file from saving level in the editor.");
	for (int i = 0; i < levelElementsToWrite.Size(); ++i) {
		LevelElement& le = *levelElementsToWrite[i];

		SpawnGroup * sg = le.spawnGroup;
		if (sg) {
			// Write it.
			outFile.WriteLine("SpawnGroup "+sg->spawnTimeString);
			outFile.WriteLine("Name " + sg->name);
			outFile.WriteLine("ShipType " + sg->shipType);
			outFile.WriteLine("Number " + String(sg->number));
			outFile.WriteLine("Formation " + GetName(sg->formation));
			outFile.WriteLine("Movement " + sg->movementPattern.name);
			outFile.WriteLine("Size xy " + VectorString(sg->size));
			outFile.WriteLine("Position xy " + VectorString(sg->position));
			outFile.WriteLine("RelativeSpeed " + String(sg->relativeSpeed));
		}

		if (le.levelMessage) {
			LevelMessage * levelMessage = le.levelMessage;
			outFile.WriteLine((levelMessage->type == LMType::EVENT? "Event " : "Message ") + String(levelMessage->startTimeOffsetSeconds));
			
			if (levelMessage->name.Length() > 0)
				outFile.WriteLine("Name " + levelMessage->name);
			if (levelMessage->string.Length() > 0)
				outFile.WriteLine("String " + levelMessage->string);
			if (levelMessage->textID.Length() > 0)
				outFile.WriteLine("TextID " + levelMessage->textID);
			if (levelMessage->dontSkip)
				outFile.WriteLine("DontSkip");
			if (levelMessage->condition.Length() > 0)
				outFile.WriteLine("Condition " + levelMessage->condition);
			if (levelMessage->goToRewindPoint)
				outFile.WriteLine("GoToRewindPoint");

			// e.g: Condition TriggeredEvent : FailedInitialTutorialEvent
;		}

		outFile.WriteLine(""); // empty line for readability between elements
	}

	outFile.Close();
}


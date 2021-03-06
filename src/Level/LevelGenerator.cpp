//Skapad 22-02-2016 kl 10:33 p� morgonen. Det var en m�ndag.

#include "Level.h"
#include "../SpaceShooter2D.h"
#include "SpawnGroup.h"
#include "../MovementPattern.h"
#include "PlayingLevel.h"
#include "LevelElement.h"

int difficulty = 1;
int defaultDelay = 0;
bool isSling = false;

AETime levelDuration(TimeType::MILLISECONDS_NO_CALENDER);


void GenerateLevel (PlayingLevel & playingLevel, String arguments)
{
	Level & level = playingLevel.GetLevel();
	Vector2f playingFieldSize = level.playingFieldSize;
	level.Clear(playingLevel);
	
	level.endCriteria = Level::NO_MORE_ENEMIES;

	List<String> args = arguments.Tokenize(" ,");
	
	if(args.Size() > 1)
	{
		String timeString = args[1];
		String diffString = args[2];
		levelDuration.ParseFrom(timeString);
		difficulty = diffString.ParseInt();
	}
	if (levelDuration.Seconds() == 0)
		levelDuration.AddMs(1000 * 180);
	
	std::cout<<"\nYo what up in da hooooood. Level generator on da waaaaay!!";
	std::cout<<"\nYou have given "<<levelDuration.Seconds()<<" as time total, and "<<difficulty<<" as difficulty! Woopdeedoo!";
	
	
	//Filter wanted ships, difficulty and alien type mainlyyyyyy
	List<String> relevantShips;
	for(int q = 0; q<Ship::types.Size(); q++)
	{
		Ship* shipType = Ship::types[q];
		
		//Catch unwanted bosses and skip them
		if(shipType->boss) 
			continue;
		//Criteria, difficulty lator!
		if(shipType->difficulty > difficulty)
			continue;
		relevantShips.AddItem(shipType -> name);
	}
	
	// Randomizer
	Random selector;
	
	CreateFolder("GeneratedLevels");

	File::ClearFile("Generatedlevel.srl");
	// Create spawns in order
	AETime spawnTime(TimeType::MILLISECONDS_NO_CALENDER);
	
	//Slight delay to make loading more fair
	spawnTime.AddMs(3000);
	while(spawnTime.Seconds() <= levelDuration.Seconds())
	{
		SpawnGroup * sg = new SpawnGroup();
		Movement movement;

		//Formation identity
		isSling = selector.Randi(100)>33;
		
		/// Time between individual spawns
		sg->spawnIntervalMsBetweenEachShipInFormation = defaultDelay;
		if(isSling==true)
		{
			sg->spawnIntervalMsBetweenEachShipInFormation = (selector.Randi(16) + 4)*50;
		}

		//Select movement pattern
		sg->movementPattern = MovementPattern::movementPatterns[selector.Randi(MovementPattern::movementPatterns.Size())];
		std::cout<<"\n"<<sg->movementPattern.name;
		
		//Select Rotation pattern
		// When it spawns
		sg->SetSpawnTime(spawnTime);
		// Pick a ship
		sg->shipType = relevantShips[selector.Randi(relevantShips.Size())];
		level.levelElements.Add(new LevelElement(sg));
		// Pick a formation
		sg->formation = Formation(selector.Randi((int)Formation::FORMATIONS - 2) + 1);
		
		
		// Pick a number of ships
		sg->number = selector.Randi(5)+1;
		// Pick a formation size
		sg->size = Vector2f((float) sg->number, (float) sg->number);
		//make sure SQUARE formation doesn't create unallowed ships
		
		while(sg->number < 5 && sg->formation == Formation::SQUARE)
		{
			sg->formation = Formation(selector.Randi((int)Formation::FORMATIONS - 2) + 1);
		}
		
		/// Default spawn on right side of field, and spawn location.
		// Randomize Y
		float randomAmountY = playingFieldSize.y - sg->size.y;
		Vector2f playingFieldHalfSize = playingFieldSize / 2;
		sg->position = Vector2f(playingFieldHalfSize.x+5.f, selector.Randf(randomAmountY) - randomAmountY * 0.5f);
		/// Add to position offsets if requested by the movement pattern
		sg->position += sg->movementPattern.spawnOffset;


		// Cooldown between formation spawns
		
		spawnTime.AddMs((selector.Randi(5)+4)*500);

		String str = sg->GetLevelCreationString(sg->SpawnTime());
		File::AppendToFile("Generatedlevel.srl", str);
		
	}
	AETime time = AETime::Now(); 
	String timestr = time.ToString("Y-M-D-H-m");
	String contents = File::GetContents("Generatedlevel.srl");
	//contents.PrintData();
	String outputFile = "./GeneratedLevels/Generatedlevel"+timestr+".srl";
	File::AppendToFile(outputFile, contents);

	/// Set current level's source to be the generated file's, so that it is reloaded upon death automatically.
	level.source = outputFile;

};

/// Emil Hedemalm & Karl Wängberg
/// 2016-02-25
/// Movement pattern class batching up movement, rotation, etc. for easing level generation.

#include "MovementPattern.h"
#include "File/File.h"
#include "String/StringUtil.h"
#include "File/LogFile.h"

List<MovementPattern> MovementPattern::movementPatterns;

/* static */ MovementPattern MovementPattern::ByName(String name)
{
	for (int i = 0; i < movementPatterns.Size(); ++i) {
		MovementPattern movementPattern = movementPatterns[i];
		if (movementPattern.name == name)
			return movementPattern;
	}
	assert(false && "Failed to find movement pattern by name");
	return MovementPattern();
}

/* static */ void MovementPattern::LoadPatterns(String fromPath)
{
	List<String> lines = File::GetLines(fromPath);
	if (lines.Size() == 0) {
		LogMain("Unable to load movement patterns from file: " + fromPath, ERROR);
		return;
	}
	char delimiter = FindCSVDelimiter(lines[0]);
	List<String> columns = TokenizeCSV(lines[0], delimiter);	
	for(int i = 1; i < lines.Size(); ++i)
	{

		List<String> tokens = TokenizeCSV(lines[i], delimiter);
		MovementPattern mp;
		for(int o = 0; o < tokens.Size(); ++o)
		{
			String column = columns[o];
			if(column == "Name")
			{
				mp.name = tokens[o];
			}
			if(column == "ID")
			{
				mp.ID = tokens[o].ParseInt();
			}
			if(column == "Movement")
			{
				mp.movements = Movement::ParseFrom(tokens[o]);
			}
			if(column == "Rotation")
			{
				mp.rotations = Rotation::ParseFrom(tokens[o]);
			}
			if (column == "Spawning Offset")
				mp.spawnOffset.ParseFrom(tokens[o]);
		} 
		MovementPattern::movementPatterns.AddItem(mp);
	}
}
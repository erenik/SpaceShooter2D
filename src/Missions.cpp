/// Emil Hedemalm
/// 2020-09-09
/// Missions which can be undertaken in the campaign.

#pragma once

#include "Missions.h"
#include "File/File.h"
#include "Game/GameVariableManager.h"

Mission::Mission() : 
	replay(false)
{
};

bool Mission::RequirementsFulfilled() {
	for (int i = 0; i < requires.Size(); ++i) {
		String require = requires[i];
		GameVar * var = GameVars.GetInt(require);
		if (!var)
			return false;
		if (var->iValue == 0)
			return false;
	}
	return true;
}


MissionsManager* MissionsManager::instance = nullptr;

MissionsManager::~MissionsManager() {
	missions.ClearAndDelete();
}

MissionsManager& MissionsManager::Instance() {
	if (!instance)
		instance = new MissionsManager();
	return *instance;
}

void MissionsManager::Deallocate() {
	delete instance;
	instance = nullptr;
}

List<Mission*> MissionsManager::GetMissions() {
	if (missions.Size())
		return missions;

	// Enter all missions into the list? Check pre-requisites?
	LoadMissions();
	return missions;
};

class TextParser {
public:
	String ParseKeyValue(String line, String key) {
		return (line - key).WithSurroundingWhitespacesRemoved();
	}
};


void MissionsManager::LoadMissions() {
	List<String> lines = File::GetLines("data/Missions.txt");
	TextParser tp;
	Mission * mission = new Mission();
	for (int i = 0; i < lines.Size(); ++i) {
		String line = lines[i];
		if (line.StartsWith("//"))
			continue;
		if (line.StartsWith("Name")) {
			if (mission->name.Length() > 0) {
				missions.Add(mission);
				mission = new Mission();
			}
			mission->name = tp.ParseKeyValue(line, "Name");
		}
		if (line.StartsWith("Requires")) {
			mission->requires.Add(tp.ParseKeyValue(line, "Requires"));
		}
		if (line.StartsWith("Replay")) {
			mission->replay = tp.ParseKeyValue(line, "Replay");
		}
		if (line.StartsWith("Level")) {
			mission->levelFilePath = tp.ParseKeyValue(line, "Level");
		}

	}
	if (mission->name.Length() > 0)
		missions.Add(mission);
}

List<Mission*> MissionsManager::GetAvailableMissions() {
	GetMissions();
	Missions availableMissions;
	for (int i = 0; i < missions.Size(); ++i){
		Mission * mission = missions[i];
		
		// Check reply - if already played it
		if (!mission->replay){
			int played = GameVars.GetIntValue(mission->name);
			if (played)
				continue;
		}
		if (!mission->RequirementsFulfilled())
			continue;

		availableMissions.Add(mission);
	}
	return availableMissions;
}

Mission * MissionsManager::GetMissionByName(String name) {
	Missions missions = GetMissions();
	name.SetComparisonMode(String::NOT_CASE_SENSITIVE);
	for (int i = 0; i < missions.Size(); ++i) {
		if (missions[i]->name == name)
			return missions[i];
	}
	return nullptr;
}

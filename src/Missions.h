/// Emil Hedemalm
/// 2020-09-09
/// Missions which can be undertaken in the campaign.

#pragma once

#include "String/AEString.h"

#define Missions List<Mission*>

struct Mission {
	Mission();
	bool RequirementsFulfilled();

	String name;
	String levelFilePath;
	bool replay;
	// Requires True for variables of given name - implying those Missions have been completed previously.
	List<String> requires;
};

#define MissionsMan MissionsManager::Instance()

class MissionsManager {
	static MissionsManager * instance;
	~MissionsManager();
public:
	static MissionsManager& Instance();
	static void Deallocate();

	List<Mission*> GetMissions();
	List<Mission*> GetAvailableMissions();
	Mission * GetMissionByName(String name);
private:
	void LoadMissions();

	List<Mission*> missions;
};

// Emil Hedemalm
// 2020-08-05
// In the hanger, interacting with NPCs, buying gear, observing the ship in 3D view...? :D 

#pragma once

#include "SpaceShooter2D.h"

class PlayerShip;

class HangarMessage {
public:
	HangarMessage() {}
	HangarMessage(Text text, int gainMoney) : text(text), gainMoney(gainMoney){
	};

	Text text = "";
	int gainMoney = 0;
};

class InHangar : public SpaceShooter2D {
public:
	virtual ~InHangar();
	/// Function when entering this state, providing a pointer to the previous StateMan.
	void OnEnter(AppState * previousState) override;
	/// Main processing function, using provided time since last frame.
	void Process(int timeInMs) override;
	/// Function when leaving this state, providing a pointer to the next StateMan.
	void OnExit(AppState * nextState) override;

	/// Callback function that will be triggered via the MessageManager when messages are processed.
	virtual void ProcessMessage(Message* message) override;
	

private:

	void PushUI();
	void PopulateMissionsList();

	void SetUpScene();

	List<Gear> GetRelevantGear();
	List<Gear> relevantGear;

	void DisplayMessage(HangarMessage msg);

	void UpdateUpgradesLists(); // Shop
	void UpdateEditShipScreen(); // Edit ship
	void FillEditScreenEntries(String listName, Gear::Type type);
	void FillSelectGearList(String list, Gear::Type type, Gear currentlyEquippedGearInSlot);
	void OnGearChanged(); // Update edit screen entries and save game.


	void UpdateUpgradeStatesInList(); // Updates colors n stuff based on level
	void UpdateHoverUpgrade(String upgrade, bool force = false);
	void UpdateActiveUpgrade(String upgrade);
	void UpdateUpgradesMoney();
	/// Shop handling...
	void BuySellToUpgrade(String upgrade);
	//void UpdateGearList();

	void SetGearCategory(Gear::Type category);

	void UpdateGearDetails(String gearName);
	void MoreStats(String upgrade, String inElement);


	List<HangarMessage> messageQueue;
	bool hangarMessageDisplayed;
	HangarMessage currentMessage;

	int displayedMoney;

	PlayerShip * playerShip;

	String previousActiveUpgrade;
	String previousHoveredUpgrade;

	// Depends where you click.
	bool replaceGear;
	Gear::Type replaceGearType;
	int replaceGearIndex;
};

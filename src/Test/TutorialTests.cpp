/// Emil Hedemalm
/// 2020-12-08
/// Tests to verify that the initial tutorial works as intended.

#include "Globals.h"
#include "String/AEString.h"

#include "Level/LevelMessage.h"
#include "Message/Message.h"
#include "Physics/Messages/PhysicsMessage.h"
#include "Message/MessageManager.h"
#include "StateManager.h"
#include "Graphics/Messages/UI/GMProceedUI.h"
#include "File/LogFile.h"
#include "PlayingLevel.h"
#include "Player/Player.h"
#include "Base/PlayerShip.h"

namespace TutorialTests {
	struct Test {
	public:
		String name;
		List<String> actions;
		int secondsWaited = 0;
	};

	List<Test> tests;
	List<Test> testsQueue;

	Test activeTest;
	int testActionIndex = -1;
	bool messageReceived = false;
	String listeningForMessageWithTextID;

	bool eventReceived = false;
	String waitForEvent;

	int millisecondsWaited = 0;
	int secondsWaited = 0;
	String testLog = "";

	void RunAllTests() {

		secondsWaited = 0;
		testLog = "/////////////////////// TESTS ////////////////////////////";

		testsQueue = tests;
		activeTest = testsQueue[0];
		activeTest.actions = List<String>("GameSpeed:5") + activeTest.actions;
		testsQueue.RemoveIndex(0, ListOption::RETAIN_ORDER);
		testActionIndex = 0;
	}

	void FailTutorialTests();

	void StartTest(String byName) {
	
	}

	void RegisterTests() {
		if (tests.Size() > 0)
			return;
		FailTutorialTests();
	}

	void FirstTutorialTests() {
		Test test;
		test.name = "FailInitialTutorial";
		test.actions.Add("NewGame", "ProceedUntilMessage:MoveWithWASD");
		test.actions.Add("ExpectEvent:FailedInitialTutorialEvent", "ExpectMessage:TryAgainToFire");
		tests.Add(test);

		test.name = "SucceedInitialTutorial";
		test.actions.Add("NewGame",
			"ProceedUntilMessage:MoveWithWASD",
			"AutoAim",
			"ExpectMessage:WellDone");
		tests.Add(test);
	}

	void SecondTutorialTests() {
		Test test;

		test.actions.Clear();
		test.name = "FailSecondTutorial";
		test.actions.Add("NewGame", "SetLevelTimeToMessage:FirstImmobileTargetsCleared", "ProceedUntilMessage:SecondTask");
		test.actions.Add("ExpectEvent:FailedWeaponSwitchTutorialEvent", "ExpectMessage:TryAgainWeaponSwitch");
		tests.Add(test);

		test.actions.Clear();
		test.name = "SucceedSecondTutorial";
		test.actions.Add("NewGame",
			"SetLevelTimeToMessage:FirstImmobileTargetsCleared",
			"ProceedUntilMessage:SecondTask",
			"AutoAim",
			"ExpectMessage:WellDone");
		tests.Add(test);

	}

	void ThirdTutorialTests() {
		Test test;

		test.actions.Clear();
		test.name = "FailThirdTutorialTest";
		test.actions.Add("NewGame",
			"DisableAutoAim",
			"SetLevelTimeToMessage:SecondTutorialCompleteMessage");
		test.actions.Add("ProceedUntilMessage:EngagingWeaponScript",
			"DisableWeaponScripts",
			"ExpectEvent:FailedToKillWeaponScriptTargets",
			"ExpectMessage:TryAgainWeaponScript");
		tests.Add(test);

		test.actions.Clear();
		test.name = "SucceedThirdTutorialTest";
		test.actions.Add("NewGame",
			"AutoAim",
			"SetLevelTimeToMessage:SecondTutorialCompleteMessage");
		test.actions.Add("ProceedUntilMessage:EngagingWeaponScript",
			"ExpectMessage:DeactivatingWeaponScript");
		tests.Add(test);
	}

	void FailTutorialTests(){
		FirstTutorialTests();
		SecondTutorialTests();
		ThirdTutorialTests();
		Test test;

		//test.actions.Clear();
		//test.name = "FailThirdTutorialTest";
		//test.actions.Add("NewGame",
		//tests.Add(test);



	}

	void AddMilliseconds(int milliseconds) {
		millisecondsWaited += milliseconds;
		if (millisecondsWaited > 1000) {
			secondsWaited += 1;
			millisecondsWaited = 0;
		}
	}

	void Update(int milliseconds) {
		if (testActionIndex == -1)
			return;

		if (secondsWaited > 60) {
			testLog += "\nTEST: Test timed out: " + activeTest.name + ", on action: " + activeTest.actions[testActionIndex];
			
			LogMain(testLog, INFO);
			testActionIndex = -1;
			MesMan.QueueMessages("ExitToMainMenu");
			return;
		}

		Test& currentTest = activeTest;
		if (testActionIndex >= currentTest.actions.Size())
		{
			activeTest.secondsWaited = secondsWaited;
			String msg = "\nTEST: Finished test " + currentTest.name+ " in "+String(activeTest.secondsWaited);
			testLog += msg;
			LogMain(msg, INFO);
			if (testsQueue.Size() == 0) {
				// All tests done
				LogMain(testLog + "\nTEST: All tests done!", INFO);
				testActionIndex = -1;
				MesMan.QueueMessages("ExitToMainMenu");
				return;
			}
			testActionIndex = 0;
			secondsWaited = 0;
			currentTest = testsQueue[0];
			testsQueue.RemoveIndex(0, ListOption::RETAIN_ORDER);
			return;
		}

		String action = currentTest.actions[testActionIndex];
		if (action == "NewGame") {
			MesMan.QueueMessages("NewGame");
			++testActionIndex;
		}
		if (action == "AutoAim") {
			PlayingLevelRef().playerShip->shoot = true;
			PlayingLevelRef().playerShip->SetAutoAim(true);
			++testActionIndex;
		}
		if (action == "DisableAutoAim") {
			PlayingLevelRef().playerShip->shoot = false;
			PlayingLevelRef().playerShip->SetAutoAim(false);
			++testActionIndex;
		}
		if (action == "DisableWeaponScripts") {
			PlayingLevelRef().playerShip->weaponScriptActive = false;
			++testActionIndex;
		}
		if (action.StartsWith("SetLevelTimeToMessage")) {
			MesMan.QueueMessages(action);
			++testActionIndex;
		}
		if (action.StartsWith("GameSpeed")) {
			++testActionIndex;
			float speed = action.Tokenize(":")[1].ParseFloat();
			QueuePhysics(new PMSet(PT_SIMULATION_SPEED, speed));
			PlayingLevelRef().gameSpeed = speed;
		}
		else if (action.StartsWith("ProceedUntilMessage")) {
			// Received
			if (messageReceived) {
				messageReceived = false;
				++testActionIndex;
			}
			// Set up waiting for message.
			else {
				messageReceived = false;
				listeningForMessageWithTextID = action.Tokenize(":")[1];
				millisecondsWaited += milliseconds;
				if (millisecondsWaited > 1000) {
					MesMan.QueueMessages("ProceedMessage");
					secondsWaited += 1;
					millisecondsWaited = 0;
				}
;			}
		}
		else if (action.StartsWith("ExpectEvent")) {
			if (eventReceived) {
				++testActionIndex;
				eventReceived = false;
			}
			else {
				AddMilliseconds(milliseconds);
				waitForEvent = action.Tokenize(":")[1];
			}
		}
		else if (action.StartsWith("ExpectMessage")) {
			if (messageReceived) {
				++testActionIndex;
				messageReceived = false;
			}
			else {
				messageReceived = false;
				listeningForMessageWithTextID = action.Tokenize(":")[1];
				AddMilliseconds(milliseconds);
			}
		}

	}

	void ProcessMessage(Message * message) {
		switch (message->type) {
		case MessageType::STRING:
			if (message->msg == listeningForMessageWithTextID)
				messageReceived = true;
			break;
		}
	}

	void OnLevelMessageTriggered(LevelMessage * levelMessage) {
		if (waitForEvent.Length() > 0 && levelMessage->name == waitForEvent) {
			eventReceived = true;
		}
		if (listeningForMessageWithTextID.Length() > 0 && levelMessage->textID == listeningForMessageWithTextID) {
			messageReceived = true;
			MesMan.QueueMessages("ProceedMessage");
		}
	}

};


/// Emil Hedemalm
/// 2020-12-08
/// Tests to verify that the initial tutorial works as intended.

#include "Globals.h"
#include "String/AEString.h"

namespace TutorialTests {

	void RunAllTests();

	void StartTest(String byName);

	void RegisterTests();

	void Update(int milliseconds);

	void ProcessMessage(Message * message);

	void OnLevelMessageTriggered(LevelMessage * levelMessage);
};


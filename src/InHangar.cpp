// Emil Hedemalm
// 2020-08-05
// In the hanger, interacting with NPCs, buying gear, observing the ship in 3D view...? :D 

#include "InHangar.h"

#include "Graphics/Messages/GMUI.h"
#include "Input/InputManager.h"

String hangarGui = "gui/Hangar.gui";

InHangar::~InHangar() {
}

/// Function when entering this state, providing a pointer to the previous StateMan.
void InHangar::OnEnter(AppState * previousState) {
	// Push Hangar UI 
	QueueGraphics(GMPushUI::ToWindow(hangarGui));
		
	/// Enable Input-UI navigation via arrow-keys and Enter/Esc.
	InputMan.SetForceNavigateUI(true);

	// Set up some 3D scene with the ship?
	SetUpScene();

	// Play different bgm or stop it.
	QueueAudio(new AMStopBGM());
}

/// Main processing function, using provided time since last frame.
void InHangar::Process(int timeInMs) {

}

/// Function when leaving this state, providing a pointer to the next StateMan.
void InHangar::OnExit(AppState * nextState) {
	QueueGraphics(new GMPopUI(hangarGui, true));
}


/// Callback function that will be triggered via the MessageManager when messages are processed.
void InHangar::ProcessMessage(Message* message)
{
	String msg = message->msg;
	switch (message->type)
	{
	case MessageType::STRING:
		if (msg == "OnReloadUI") {
			QueueGraphics(GMPushUI::ToWindow("gui/Hangar.gui"));
		}
	}
	// Process general messages.
	SpaceShooter2D::ProcessMessage(message);
}



void InHangar::SetUpScene(){
	MapMan.DeleteAllEntities();
}


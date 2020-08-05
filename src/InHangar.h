// Emil Hedemalm
// 2020-08-05
// In the hanger, interacting with NPCs, buying gear, observing the ship in 3D view...? :D 

#pragma once

#include "SpaceShooter2D.h"

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
	void SetUpScene();

};

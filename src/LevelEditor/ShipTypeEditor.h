// Emil Hedemalm
// 2021-04-29
// UI code for managing ship types and editing them within the level editor. See ShipTypeEditor.gui for design.

#pragma once

class Ship;
class Message;

class ShipTypeEditor{
public:
	// Updating the ship based on inputs.
	void ProcessMessage(Message* message);

	void Process(int timeInMs);

	// Update UI.
	void OnEditedShipUpdated();

	void Push();
	void Hide();
	bool Visible() {
		return visible;
	};

	void UpdateAvailableShipTypes();

	Ship * editedShip = nullptr;

private:
	bool visible = false;
};

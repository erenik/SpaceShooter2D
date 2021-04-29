// Emil Hedemalm
// 2021-04-29
// UI code for managing ship types and editing them within the level editor. See ShipTypeEditor.gui for design.

#include "ShipTypeEditor.h"

#include "Message/Message.h"
#include "Base/Ship.h"
#include "StateManager.h"
#include "Graphics/Messages/GMUI.h"

Ship * editedShip;

// Updating the ship based on inputs.
void ShipTypeEditor::ProcessMessage(Message* message) {

	String msg = message->msg;

	if (message->type == MessageType::ON_UI_PUSHED) {
		OnUIPushed* uiPushed = (OnUIPushed*)message;
		// Set default path to ./Level/
		if (uiPushed->msg == "ShipTypeEditor") {
			// Populate list of ship types.
			auto shipTypes = Ship::types;
			List<String> shipTypeNames;
			for (int i = 0; i < shipTypes.Size(); ++i) {
				shipTypeNames.Add(shipTypes[i]->name);
			}
			QueueGraphics(new GMSetUIContents("OpenShipType", shipTypeNames));
			QueueGraphics(new GMSetUIContents("ShipType", List<String>("Pirate", "Asteroid")));
		}
	}

	if (message->type == MessageType::SET_STRING) {
		SetStringMessage * ssm = (SetStringMessage*)message;
		if (msg == "SetShipType") {
			editedShip = Ship::GetByType(ssm->value);
			OnEditedShipUpdated();
		}
		else if (msg.Contains("OpenShipType")) {
			editedShip = Ship::GetByType(ssm->value);
			OnEditedShipUpdated();
		}
	}
}

// Update UI.
void ShipTypeEditor::OnEditedShipUpdated() {
	if (editedShip) {
		QueueGraphics(new GMSetUIs("ShipType", GMUI::DROP_DOWN_INPUT_SELECT, editedShip->type));
		QueueGraphics(new GMSetUIs("ShipName", GMUI::STRING_INPUT, editedShip->name));
		if (editedShip->weaponSet.Size())
			QueueGraphics(new GMSetUIs("Weapons", GMUI::DROP_DOWN_INPUT_SELECT, editedShip->weaponSet[0]->name));
		else 
			QueueGraphics(new GMSetUIs("Weapons", GMUI::DROP_DOWN_INPUT_SELECT, ""));
		//QueueGraphics(new GMSetUIs("MovementPattern", GMUI::DROP_DOWN_INPUT_SELECT, editedShip->mo));
		QueueGraphics(new GMSetUIf("Speed", GMUI::FLOAT_INPUT, editedShip->speed));
		QueueGraphics(new GMSetUIi("HitPoints", GMUI::INTEGER_INPUT, editedShip->maxHP));
		QueueGraphics(new GMSetUIs("GraphicsModel", GMUI::FILE_INPUT, editedShip->visuals.graphicModel));
		QueueGraphics(new GMSetUIs("Texture", GMUI::FILE_INPUT, editedShip->visuals.textureSource));
		QueueGraphics(new GMSetUIi("CollisionDamage", GMUI::INTEGER_INPUT, editedShip->collideDamage));
		QueueGraphics(new GMSetUIi("Score", GMUI::INTEGER_INPUT, editedShip->score));

		QueueGraphics(new GMSetUIs("TexturePreview", GMUI::TEXTURE_SOURCE, editedShip->visuals.textureSource));
	}
}



// Emil Hedemalm
// 2021-04-29
// UI code for managing ship types and editing them within the level editor. See ShipTypeEditor.gui for design.

#include "ShipTypeEditor.h"

#include "Message/Message.h"
#include "Message/MathMessage.h"
#include "Base/Ship.h"
#include "StateManager.h"
#include "Graphics/Messages/GMUI.h"

Ship * editedShip;

Time deleteClickTime;

void ShipTypeEditor::OnEnter() {
	editedShip = nullptr;
	OnEditedShipUpdated();
}

// Updating the ship based on inputs.
void ShipTypeEditor::ProcessMessage(Message* message) {

	String msg = message->msg;

	if (message->type == MessageType::STRING) {
		if (msg == "SaveShipTypes") {
			Ship::SaveTypes("data/AllShips.csv");
		}
		if (msg == "NewShipType") {
			Ship * newType = Ship::NewShipType("New type");
			editedShip = newType;
			OnEditedShipUpdated();
			// Inherit defaults from currently selected one?.. or not.
			UpdateAvailableShipTypes();
		}
		if (msg == "DeleteShipType") {
			if (editedShip == nullptr)
				return;
			Time now = Time::Now();
			int msDiff = 100000;
			if (deleteClickTime.Type() == now.Type())
				msDiff = (now - deleteClickTime).Milliseconds();
			if (msDiff < 5000) {
				Ship::types.Remove(editedShip);
				delete editedShip;
				editedShip = nullptr;
				QueueGraphics(new GMSetUIt("DeleteShipType", GMUI::TEXT, "Delete ship type"));
				OnEditedShipUpdated();
				UpdateAvailableShipTypes();
			}
			else {
				QueueGraphics(new GMSetUIt("DeleteShipType", GMUI::TEXT, "Delete ship type - Press again to confirm"));
			}
			deleteClickTime = now;
		}
	}

	if (message->type == MessageType::ON_UI_PUSHED) {
		OnUIPushed* uiPushed = (OnUIPushed*)message;
		// Set default path to ./Level/
		if (uiPushed->msg == "ShipTypeEditor") {
			UpdateAvailableShipTypes();

			List<String> shipTypeTypes;
			shipTypeTypes.Add("Pirate", "Asteroid", "PracticeTarget", "Debris");
			QueueGraphics(new GMSetUIContents("ShipType", shipTypeTypes));

			List<String> weaponTypes;
			for (int i = 0; i < Weapon::types.Size(); ++i) 
				weaponTypes.Add(Weapon::types[i].name);
			QueueGraphics(new GMSetUIContents("Weapons", weaponTypes));


			OnEnter();
			QueueGraphics(new GMSetUIb("DeleteShipType", GMUI::ENABLED, false));

			visible = true;
		}
	}
	else if (message->type == MessageType::ON_UI_POPPED) {
		OnUIPopped* uiPopped = (OnUIPopped*) message;
		if (uiPopped->msg == "ShipTypeEditor") {
			visible = false;
		}
	}

	if (message->type == MessageType::SET_STRING) {
		SetStringMessage * ssm = (SetStringMessage*)message;
		if (msg == "SetShipType") {
			editedShip = Ship::GetByType(ssm->value);
			OnEditedShipUpdated();
			QueueGraphics(new GMSetUIb("DeleteShipType", GMUI::ENABLED, editedShip != nullptr));
		}
		else if (msg.Contains("ShipName")) {
			editedShip->name = ssm->value;
		}
		else if (msg.Contains("OpenShipType")) {
			editedShip = Ship::GetByType(ssm->value);
			OnEditedShipUpdated();
			QueueGraphics(new GMSetUIb("DeleteShipType", GMUI::ENABLED, editedShip != nullptr));
		}
		else if (msg.Contains("GraphicsModel")) {
			editedShip->visuals.graphicModel = ssm->value;
		}
		else if (msg.Contains("Texture")) {
			editedShip->visuals.textureSource = ssm->value;
			QueueGraphics(new GMSetUIs("TexturePreview", GMUI::TEXTURE_SOURCE, editedShip->visuals.textureSource));
		}
	}
	else if (message->type == MessageType::FLOAT_MESSAGE) {
		FloatMessage* fm = (FloatMessage*)message;
		if (msg.Contains("Speed")) {
			editedShip->speed = fm->value;
		}
	}
	else if (message->type == MessageType::INTEGER_MESSAGE) {
		IntegerMessage* im = (IntegerMessage*)message;
		if (msg.Contains("HitPoints")) {
			editedShip->maxHP = im->value;
		}
		else if (msg.Contains("CollisionDamage")) {
			editedShip->collideDamage = im->value;
		}
		else if (msg.Contains("Score")) {
			editedShip->score = im->value;
		}
	}
}

int lastUpdateMs = 200;
void ShipTypeEditor::Process(int timeInMs) {
	lastUpdateMs -= timeInMs;
	if (lastUpdateMs < 0) {
		lastUpdateMs = 200;
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

	QueueGraphics(new GMSetUIb("DeleteShipType", GMUI::ENABLED, editedShip != nullptr));
	QueueGraphics(new GMSetUIb("lAttributes", GMUI::ENABLED, editedShip != nullptr, UIFilter::IncludeChildren));

}

void Push() {

}
void Hide() {
}

void ShipTypeEditor::UpdateAvailableShipTypes() {
	// Populate list of ship types.
	auto shipTypes = Ship::types;
	List<String> shipTypeNames;
	for (int i = 0; i < shipTypes.Size(); ++i) {
		shipTypeNames.Add(shipTypes[i]->name);
	}
	QueueGraphics(new GMSetUIContents("OpenShipType", shipTypeNames));

}



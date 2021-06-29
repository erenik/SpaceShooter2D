/// Emil Hedemalm
/// 2020-12-12
/// UI style defaults for the game.

#include "SpaceShooter2D.h"


#include "UI/Buttons/UIToggleButton.h"
#include "UI/Lists/UIScrollBarHandle.h"
#include "UI/Lists/UIScrollBar.h"
#include "UI/UIInputs.h"
#include "Graphics/Fonts/TextFont.h"

void SpaceShooter2D::SetupUIStyling() {
	TextFont::defaultFontSource = "img/fonts/Font_test_BW.png";
	UIText::defaultForceUpperCase = true;

	//UIElement::defaultTextColor = Color::ColorByHexName("#8f1767ff");
	UIVisuals::defaultTextureSource = "0x22AA";

	UIToggleButton::defaultOnToggledTexture = "";
	UIToggleButton::defaultOnNotToggledTexture = "";

	UIInput::defaultInputTextColor = Color::ColorByHexName("#f9c22bff");

	UIScrollBar::defaultTextureSource = "#323353ff";

	UIScrollBarHandle::defaultTextureSource = "ui/Orange2";

	UIScrollBarHandle::defaultTopBorder = "ui/ScrollBarHandle_UpperBorder";
	UIScrollBarHandle::defaultRightBorder = "";
	UIScrollBarHandle::defaultBottomBorder = "ui/ScrollBarHandle_BottomBorder";

	int borderScale = 3;
	UIScrollBarHandle::defaultLockWidth = 12 * borderScale;
	UIScrollBarHandle::defaultLockHeight = 0;
	UIScrollBarHandle::defaultBorderOffset = 2;

	TextFont::colors = TextColors(
		Vector3f(0.05f,0,0), 
		Vector3f(0.15f,0,0), 
		Vector3f(0.25f, 0,0));
	TextFont::colors.notToggledIdle = TextFont::colors.notToggledHover = Vector3f(0.35,0,0);
	TextFont::colors.toggledIdle = TextFont::colors.toggledHover = Vector3f(0.45,0,0);
}

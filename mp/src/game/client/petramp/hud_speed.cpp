#include "cbase.h"
#include "hudelement.h"
#include "hud_numericdisplay.h"
#include "iclientmode.h"

#include "vgui_controls/Panel.h"
#include "vgui/ISurface.h"
#include "vgui/ILocalize.h"

#include "vphysics_interface.h"
#include "c_prop_vehicle.h"

using namespace vgui;

// shoutouts waezone, czf

static ConVar pmp_speedometer("gh_speedometer",
	"1",
	FCVAR_CLIENTDLL | FCVAR_ARCHIVE,
	"speedometer. 0 - disabled, 1 - enabled");


class CHudSpeedometer : public CHudElement, public CHudNumericDisplay {
	DECLARE_CLASS_SIMPLE(CHudSpeedometer, CHudNumericDisplay);

public:
	CHudSpeedometer(const char* elementName);

	virtual void Init();
	virtual void VidInit();
	virtual void Reset();
	virtual bool ShouldDraw();
	virtual void OnThink();
};

DECLARE_HUDELEMENT(CHudSpeedometer);

CHudSpeedometer::CHudSpeedometer(const char* elementName) :
	CHudElement(elementName), CHudNumericDisplay(NULL, "HudSpeedometer")
{
	SetParent(g_pClientMode->GetViewport());
	SetHiddenBits(HIDEHUD_PLAYERDEAD);
}

void CHudSpeedometer::Init() {
	Reset();
}

void CHudSpeedometer::VidInit() {
	Reset();
}

bool CHudSpeedometer::ShouldDraw() {
	return pmp_speedometer.GetBool() && CHudElement::ShouldDraw();
}

void CHudSpeedometer::Reset() {
	SetLabelText(L"UPS");
	SetDisplayValue(0);
}

void CHudSpeedometer::OnThink() {
	Vector velocity(0., 0., 0.);
	CBasePlayer* player = C_BasePlayer::GetLocalPlayer();

	if (player) {
		velocity = player->GetLocalVelocity();

		// do funny projection, minmax tracking, etc here

		SetDisplayValue((int) velocity.Length());
	}
}

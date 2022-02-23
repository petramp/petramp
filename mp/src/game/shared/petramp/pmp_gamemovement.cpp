#include "cbase.h"

#include "pmp_gamemovement.h"

#include "tier0/memdbgon.h"

CPMPGameMovement::CPMPGameMovement() {}

// expose PetraMP gamemovement to SDK base
static CPMPGameMovement g_GameMovement;
IGameMovement* g_pGameMovement = (IGameMovement*)&g_GameMovement;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(
	CGameMovement,
	IGameMovement,
	INTERFACENAME_GAMEMOVEMENT,
	g_GameMovement
);
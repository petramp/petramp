#pragma once

#include "gamemovement.h"

#if defined( CLIENT_DLL )
#include "c_basehlplayer.h"
#define CHL2_Player C_BaseHLPlayer
#else
#include "hl2_player.h"
#endif

class CPMPGameMovement : public CGameMovement {
	typedef CGameMovement BaseClass;

public:
	CPMPGameMovement();

//	int TryPlayerMove(Vector* pFirstDest = nullptr, trace_t* pFirstTrace = nullptr) override;
	void CategorizePosition() override;
};
#include "cbase.h"

#include "pmp_gamemovement.h"
#include "movevars_shared.h"

#include "tier0/memdbgon.h"

CPMPGameMovement::CPMPGameMovement() {}

#define NON_JUMP_VELOCITY 140.f
// steepness limit (z norm minimum to be considered grounded)
#define ZNORM_GROUNDED_LIMIT 0.7f

void CPMPGameMovement::CategorizePosition()
{
	Vector point;
	trace_t pm;

	// [valve] reset this each time we-recategorize,
	// otherwise we have bogus friction when we jump into water
	// and plunge downward really quickly
	player->m_surfaceFriction = 1.0f;

	// [valve] doing this before we move may introduce a potential latency in water detection, but
    // doing it after can get us stuck on the bottom in water if the amount we move up
    // is less than the 1 pixel 'threshold' we're about to snap to.    Also, we'll call
    // this several times per frame, so we really need to avoid sticking to the bottom of
    // water on each call, and the converse case will correct itself if called twice.
	CheckWater();

	if (player->IsObserver()) {
		// don't have a ground entity if just observing
		return;
	}

	float offset = sv_ground_band.GetFloat();

	const Vector bumpOrigin = mv->GetAbsOrigin();
	point[0] = bumpOrigin[0];
	point[1] = bumpOrigin[1];
	point[2] = bumpOrigin[2] - offset;

	float zvel = mv->m_vecVelocity[2];
	bool movingUp = zvel > 0.f;
	bool movingUpRapidly = zvel > NON_JUMP_VELOCITY;
	
	// fix cases in which the ground is moving up too
	// [valve] notes some tickets in which this happened w/ saveloads
	float groundEntityZvel = 0.f;
	if (movingUpRapidly) {
		CBaseEntity* ground = player->GetGroundEntity();
		if (ground) {
			groundEntityZvel = ground->GetAbsVelocity().z;
			movingUpRapidly = (zvel - groundEntityZvel) > NON_JUMP_VELOCITY;
		}
	}

	// TODO(petra) hack to keep code based on mom wallrun; needs updates for our dives.
	bool moveDown = true;

	// suddenly not on ground!
	if (movingUpRapidly || (movingUp && player->GetMoveType() == MOVETYPE_LADDER)) {
		SetGroundEntity(NULL);
	}
	else if (moveDown)
	{
		// try and move down
		TryTouchGround(bumpOrigin, point, GetPlayerMins(), GetPlayerMaxs(), MASK_PLAYERSOLID, COLLISION_GROUP_PLAYER_MOVEMENT, pm);

		// no ground isn't ground. also, steep planes aren't ground
		if (!pm.m_pEnt || pm.plane.normal[2] < ZNORM_GROUNDED_LIMIT) {
			// try subboxes and see if we can find shallower, standable slope
			TryTouchGroundInQuadrants(bumpOrigin, point, MASK_PLAYERSOLID, COLLISION_GROUP_PLAYER_MOVEMENT, pm);
			
			// if we still can't find any standable ground, unground
			if (!pm.m_pEnt || pm.plane.normal[2] < ZNORM_GROUNDED_LIMIT) {
				SetGroundEntity(nullptr);

				// [valve] "probably want to add a check for a +z velocity too!"
				if ((mv->m_vecVelocity.z > 0.f) && (player->GetMoveType() != MOVETYPE_NOCLIP)) {
					player->m_surfaceFriction = 0.25f;
				}
			}

			// MOM DIVERGENCE FROM VALVE:
			// here, [valve] does else SetGroundEntity(&pm); no branch like this in [mom].
		}
		else
		{
			if (player->GetGroundEntity() == nullptr) {
				Vector nextVelocity = mv->m_vecVelocity;
				
				// [mom] apply half-gravity; this matches what would be done in next tick pre-movement-code.
				nextVelocity.z -= player->GetGravity() * GetCurrentGravity() * .5f * gpGlobals->frametime;

				// [mom] attempt to predict what would happen on next tick, assuming no grounding
				bool speculateGrounded = true;
				// TODO(petra): FIX FOR AUTOBHOP HERE
				// TODO(petra): replace `true` with a `sv_edge_fix` cvar
				if (true) {
					trace_t tr_fall;

					// [mom] don't sim fall if we're already on ground
					if (pm.fraction != 0.0f) {
						Vector endFall;
						// endFall = mv->GetAbsOrigin() + frametime*nextVelocity
						VectorMA(mv->GetAbsOrigin(), gpGlobals->frametime, nextVelocity, endFall);

						TracePlayerBBox(mv->GetAbsOrigin(), endFall, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, tr_fall);
					}
					// TODO(petra): track interactions following [mom].
					else
					{
						// fallback
						tr_fall = pm;
					}

					if (!tr_fall.DidHit()) {
						// miss ground next tick
						speculateGrounded = false;
					}
					else {
						ClipVelocity(nextVelocity, tr_fall.plane.normal, nextVelocity, 1.0f);

						// [mom] simulate slide on ground
						Vector endSlide;
						// endSlide = endpos + frametime*nextVelocity
						VectorMA(tr_fall.endpos, gpGlobals->frametime, nextVelocity, endSlide);

						trace_t tr_slide;
						TracePlayerBBox(mv->GetAbsOrigin(), endSlide, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, tr_slide);

						// [mom] is there ground under the player? if not, it's an edgebug
						trace_t tr_ground;
						TryTouchGround(tr_slide.endpos, tr_slide.endpos - Vector(0.f, 0.f, offset), GetPlayerMins(), GetPlayerMaxs(), MASK_PLAYERSOLID, COLLISION_GROUP_PLAYER_MOVEMENT, tr_ground);

						if (!tr_ground.DidHit()) {
							speculateGrounded = false; // [mom] edgebug next tick
						}
					}
				}

				// [mom] switches on sv_rngfix_enable; we just do the default 0 branch
				{
					ClipVelocity(nextVelocity, pm.plane.normal, nextVelocity, 1.0f);

					// [mom] if player won't slide on a ramp next tick, and they'll be grounded,
					// set gnd ent (if they don't want to bhop)
					// TODO(petra): FIX FOR AUTOBHOP HERE
					if (nextVelocity.z <= NON_JUMP_VELOCITY && speculateGrounded) {						
						// [mom] "make sure we check clip velocity on slopes/surfs before setting the ground entity and nulling out velocity.z"
						if (sv_slope_fix.GetBool() && nextVelocity.Length2DSqr() > mv->m_vecVelocity.Length2DSqr()) {
							VectorCopy(nextVelocity, mv->m_vecVelocity);
						}

						SetGroundEntity(&pm);
					}
				}
			}
			else {
				// [mom] notes that this isn't necessary for non-tf2 gamemodes. it's not clear if this applies to us; TODO(petra): find out
			}
		}
	}
}

// expose PetraMP gamemovement to SDK base
static CPMPGameMovement g_GameMovement;
IGameMovement* g_pGameMovement = (IGameMovement*)&g_GameMovement;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(
	CGameMovement,
	IGameMovement,
	INTERFACENAME_GAMEMOVEMENT,
	g_GameMovement
);
////////////////////////////////////////////////////////////////////////////
//	Module 		: ai_stalker_events.cpp
//	Created 	: 26.02.2003
//  Modified 	: 26.02.2003
//	Author		: Dmitriy Iassenev
//	Description : Events handling for monster "Stalker"
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ai_stalker.h"
#include "../../pda.h"
#include "../../inventory.h"
#include "../../../xrServerEntities/xrmessages.h"
#include "../../shootingobject.h"
#include "../../level.h"
#include "../../ai_monster_space.h"
#include "../../characterphysicssupport.h"
#include "CustomZone.h"
#include "script_game_object.h"

using namespace StalkerSpace;
using namespace MonsterSpace;

#define SILENCE

void CAI_Stalker::OnEvent(NET_Packet& P, u16 type)
{
	inherited::OnEvent(P, type);
	CInventoryOwner::OnEvent(P, type);

	switch (type)
	{
	case GE_TRADE_BUY:
	case GE_OWNERSHIP_TAKE:
		{
			u16 id;
			P.r_u16(id);
			CObject* O = Level().Objects.net_Find(id);

			R_ASSERT(O);

#ifndef SILENCE
			Msg("Trying to take - %s (%d)", *O->cName(),O->ID());
#endif
			CGameObject* _O = smart_cast<CGameObject*>(O);
			if (inventory().CanTakeItem(smart_cast<CInventoryItem*>(_O)))
			{
				O->H_SetParent(this);
				inventory().Take(_O, true, false);
				if (!inventory().ActiveItem() && GetScriptControl() && smart_cast<CShootingObject*>(O))
					CObjectHandler::set_goal(eObjectActionIdle, _O);

				on_after_take(_O);
#ifndef SILENCE
				Msg("TAKE - %s (%d)", *O->cName(),O->ID());
#endif
			}
			else
			{
				//				DropItemSendMessage(O);
				NET_Packet P;
				u_EventGen(P, GE_OWNERSHIP_REJECT, ID());
				P.w_u16(u16(O->ID()));
				u_EventSend(P);

#ifndef SILENCE
				Msg("TAKE - can't take! - Dropping for valid server information %s (%d)", *O->cName(),O->ID());
#endif
			}
			break;
		}
	case GE_TRADE_SELL:
	case GE_OWNERSHIP_REJECT:
		{
			u16 id;
			P.r_u16(id);

			CObject* O = Level().Objects.net_Find(id);

#pragma todo("Dima to Oles : how can this happen?")
			if (!O)
				break;

			bool just_before_destroy = !P.r_eof() && P.r_u8();
			bool dont_create_shell = (type == GE_TRADE_SELL) || just_before_destroy;


			O->SetTmpPreDestroy(just_before_destroy);
			on_ownership_reject(O, dont_create_shell);


			break;
		}
	}
}

void CAI_Stalker::on_ownership_reject(CObject* O, bool just_before_destroy)
{
	m_pPhysics_support->in_UpdateCL();
	IKinematics* const kinematics = smart_cast<IKinematics*>(Visual());
	kinematics->CalculateBones_Invalidate();
	kinematics->CalculateBones(true);

	CGameObject* const game_object = smart_cast<CGameObject*>(O);
	VERIFY(game_object);

	if (!inventory().DropItem(game_object, just_before_destroy, just_before_destroy))
		return;

	if (O->getDestroy())
		return;

	feel_touch_deny(O, 2000);
}

void CAI_Stalker::generate_take_event(CObject const* const object) const
{
	NET_Packet packet;
	u_EventGen(packet, GE_OWNERSHIP_TAKE, ID());
	packet.w_u16(object->ID());
	u_EventSend(packet);
}

void CAI_Stalker::DropItemSendMessage(CObject* O)
{
	if (!O || !O->H_Parent() || (this != O->H_Parent()))
		return;

#ifndef SILENCE
	Msg("Dropping item!");
#endif
	// We doesn't have similar weapon - pick up it
	NET_Packet P;
	u_EventGen(P, GE_OWNERSHIP_REJECT, ID());
	P.w_u16(u16(O->ID()));
	u_EventSend(P);
}

void CAI_Stalker::UpdateAvailableDialogs(CPhraseDialogManager* partner)
{
	CAI_PhraseDialogManager::UpdateAvailableDialogs(partner);
}

extern BOOL g_ai_die_in_anomaly;
void CAI_Stalker::feel_touch_new(CObject* O)
{
	//	Msg					("FEEL_TOUCH::NEW : %s",*O->cName());
	if (!g_Alive()) return;
	if (Remote()) return;
	if ((O->spatial.type | STYPE_VISIBLEFORAI) != O->spatial.type) return;

	// demonized: add g_ai_die_in_anomaly == 0 check
	if (!(g_ai_die_in_anomaly || m_enable_anomalies_pathfinding)) {
		CCustomZone* sr = smart_cast<CCustomZone*>(O);
		if (sr) {
			return;
		}
	}

	// Now, test for game specific logical objects to minimize traffic
	CInventoryItem* I = smart_cast<CInventoryItem*>(O);

	if (!wounded() && !critically_wounded() && I && I->useful_for_NPC() && can_take(I))
	{
#ifndef SILENCE
		Msg("Taking item %s (%d)!",I->object().cName().c_str(),I->object().ID());
#endif
		// Tosox
		// Callback for when NPC tries to pickup an item
		::luabind::functor<bool> func;
		if (ai().script_engine().functor("_G.CAI_Stalker__OnBeforeOwnershipTake", func)) {
			if (!func(lua_game_object(), I->cast_game_object()->lua_game_object())) {
				return;
			}
		}

		generate_take_event(O);
		return;
	}

	VERIFY2(
		std::find(m_ignored_touched_objects.begin(), m_ignored_touched_objects.end(), O) == m_ignored_touched_objects.
		end(),
		make_string("object %s is already in ignroed touched objects list", O->cName().c_str())
	);
	m_ignored_touched_objects.push_back(O);
}

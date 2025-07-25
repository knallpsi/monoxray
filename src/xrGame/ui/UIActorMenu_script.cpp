////////////////////////////////////////////////////////////////////////////
//	Module 		: UIActorMenu_script.cpp
//	Created 	: 18.04.2008
//	Author		: Evgeniy Sokolov
//	Description : UI ActorMenu script implementation
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "UIActorMenu.h"
#include "../UIGameCustom.h"

#include "UIWindow.h"
#include "UICellItemFactory.h"
#include "UIDragDropListEx.h"
#include "UIDragDropReferenceList.h"
#include "UICellCustomItems.h"

#include "../actor.h"
#include "../inventory_item.h"
#include "UICellItem.h"
#include "../ai_space.h"
#include "../../xrServerEntities/script_engine.h"
#include "eatable_item.h"

#include "UIPdaWnd.h"
#include "UITabControl.h"

#include "UIMainIngameWnd.h"
#include "UIZoneMap.h"
#include "UIMotionIcon.h"
#include "UIHudStatesWnd.h"
#include "UIMessagesWindow.h"

using namespace luabind;

CUIActorMenu* GetActorMenu()
{
	return &CurrentGameUI()->GetActorMenu();
}

CUIPdaWnd* GetPDAMenu()
{
	return &CurrentGameUI()->GetPdaMenu();
}

CUIMainIngameWnd* GetMainGameMenu()
{
	return CurrentGameUI()->UIMainIngameWnd;
}

CUIMessagesWindow* GetMessagesMenu()
{
	return CurrentGameUI()->m_pMessagesWnd;
}

u8 GrabMenuMode()
{
	return (u8)(CurrentGameUI()->GetActorMenu().GetMenuMode());
}

CScriptGameObject* CUIActorMenu::GetCurrentItemAsGameObject()
{
	CGameObject* GO = smart_cast<CGameObject*>(CurrentIItem());
	if (GO)
		return GO->lua_game_object();

	return (0);
}

bool CUIActorMenu::CanRepairItem(PIItem item)
{
	if (item->GetCondition() > 0.99f)
	{
		return false;
	}
	LPCSTR item_name = item->m_section_id.c_str();

	CEatableItem* EItm = smart_cast<CEatableItem*>(item);
	if (EItm)
	{
		bool allow_repair = !!READ_IF_EXISTS(pSettings, r_bool, item_name, "allow_repair", false);
		if (!allow_repair)
			return false;
	}

	LPCSTR partner = m_pPartnerInvOwner->CharacterInfo().Profile().c_str();

	::luabind::functor<bool> funct;
	R_ASSERT2(
		ai().script_engine().functor("inventory_upgrades.can_repair_item", funct),
		make_string("Failed to get functor <inventory_upgrades.can_repair_item>, item = %s", item_name)
	);
	bool can_repair = funct(item_name, item->GetCondition(), partner);

	return can_repair;
}

LPCSTR CUIActorMenu::RepairQuestion(PIItem item, bool can_repair)
{
	LPCSTR partner = m_pPartnerInvOwner->CharacterInfo().Profile().c_str();
	LPCSTR item_name = item->m_section_id.c_str();
	::luabind::functor<LPCSTR> funct2;
	R_ASSERT2(
		ai().script_engine().functor("inventory_upgrades.question_repair_item", funct2),
		make_string("Failed to get functor <inventory_upgrades.question_repair_item>, item = %s", item_name)
	);
	LPCSTR question = funct2(item->m_section_id.c_str(), item->GetCondition(), can_repair, partner);

	return question;
}

void CUIActorMenu::TryRepairItem(CUIWindow* w, void* d)
{
	PIItem item = get_upgrade_item();
	if (!item)
	{
		return;
	}

	LPCSTR item_name = item->m_section_id.c_str();

	bool can_repair = CanRepairItem(item);

	::luabind::functor<bool> funct;
	R_ASSERT2(
		ai().script_engine().functor("inventory_upgrades.can_afford_repair_item", funct),
		make_string("Failed to get functor <inventory_upgrades.can_afford_repair_item>, item = %s", item_name)
	);
	bool enough_money = funct(item_name, item->GetCondition());

	if (can_repair)
	{
		if (enough_money)
		{
			m_repair_mode = true;
			CallMessageBoxYesNo(RepairQuestion(item, true));
		}
		else
			CallMessageBoxOK(RepairQuestion(item, true));
	}
}

void CUIActorMenu::RepairEffect_CurItem()
{
	PIItem item = CurrentIItem();
	if (!item)
	{
		return;
	}
	LPCSTR item_name = item->m_section_id.c_str();

	::luabind::functor<void> funct;
	R_ASSERT(ai().script_engine().functor( "inventory_upgrades.effect_repair_item", funct ));
	funct(item_name, item->GetCondition());

	item->SetCondition(1.0f);
	UpdateConditionProgressBars();
	SeparateUpgradeItem();
	CUICellItem* itm = CurrentItem();
	if (itm)
		itm->UpdateConditionProgressBar();
}

bool CUIActorMenu::CanUpgradeItem(PIItem item)
{
	VERIFY(item && m_pPartnerInvOwner);
	LPCSTR item_name = item->m_section_id.c_str();
	LPCSTR partner = m_pPartnerInvOwner->CharacterInfo().Profile().c_str();

	::luabind::functor<bool> funct;
	R_ASSERT2(
		ai().script_engine().functor( "inventory_upgrades.can_upgrade_item", funct ),
		make_string( "Failed to get functor <inventory_upgrades.can_upgrade_item>, item = %s, mechanic = %s", item_name,
			partner )
	);

	return funct(item_name, partner);
}

void CUIActorMenu::CurModeToScript()
{
	int mode = (int)m_currMenuMode;
	::luabind::functor<void> funct;
	R_ASSERT(ai().script_engine().functor( "actor_menu.actor_menu_mode", funct ));
	funct(mode);
}

void CUIActorMenu::HighlightSectionInSlot(LPCSTR section, u8 type, u16 slot_id)
{
	CUIDragDropListEx* slot_list = m_pInventoryBagList;
	switch (type)
	{
	case EDDListType::iActorBag:
		slot_list = m_pInventoryBagList;
		break;
	case EDDListType::iActorBelt:
		slot_list = m_pInventoryBeltList;
		break;
	case EDDListType::iActorSlot:
		slot_list = GetSlotList(slot_id);
		break;
	case EDDListType::iActorTrade:
		slot_list = m_pTradeActorBagList;
		break;
	case EDDListType::iDeadBodyBag:
		slot_list = m_pDeadBodyBagList;
		break;
	case EDDListType::iPartnerTrade:
		slot_list = m_pTradePartnerList;
		break;
	case EDDListType::iPartnerTradeBag:
		slot_list = m_pTradePartnerBagList;
		break;
	case EDDListType::iQuickSlot:
		slot_list = m_pQuickSlot;
		break;
	case EDDListType::iTrashSlot:
		slot_list = m_pTrashList;
		break;
	}

	if (!slot_list)
		return;

	u32 const cnt = slot_list->ItemsCount();
	for (u32 i = 0; i < cnt; ++i)
	{
		CUICellItem* ci = slot_list->GetItemIdx(i);
		PIItem item = (PIItem)ci->m_pData;
		if (!item)
			continue;

		if (!strcmp(section, item->m_section_id.c_str()) == 0)
			continue;

		ci->m_select_armament = true;
	}

	m_highlight_clear = false;
}


void CUIActorMenu::HighlightForEachInSlot(const ::luabind::functor<bool>& functor, u8 type, u16 slot_id)
{
	if (!functor)
		return;

	CUIDragDropListEx* slot_list = m_pInventoryBagList;
	switch (type)
	{
	case EDDListType::iActorBag:
		slot_list = m_pInventoryBagList;
		break;
	case EDDListType::iActorBelt:
		slot_list = m_pInventoryBeltList;
		break;
	case EDDListType::iActorSlot:
		slot_list = GetSlotList(slot_id);
		break;
	case EDDListType::iActorTrade:
		slot_list = m_pTradeActorBagList;
		break;
	case EDDListType::iDeadBodyBag:
		slot_list = m_pDeadBodyBagList;
		break;
	case EDDListType::iPartnerTrade:
		slot_list = m_pTradePartnerList;
		break;
	case EDDListType::iPartnerTradeBag:
		slot_list = m_pTradePartnerBagList;
		break;
	case EDDListType::iQuickSlot:
		slot_list = m_pQuickSlot;
		break;
	case EDDListType::iTrashSlot:
		slot_list = m_pTrashList;
		break;
	}

	if (!slot_list)
		return;

	u32 const cnt = slot_list->ItemsCount();
	for (u32 i = 0; i < cnt; ++i)
	{
		CUICellItem* ci = slot_list->GetItemIdx(i);
		PIItem item = (PIItem)ci->m_pData;
		if (!item)
			continue;

		if (functor(item->object().cast_game_object()->lua_game_object()) == false)
			continue;

		ci->m_select_armament = true;
	}

	m_highlight_clear = false;
}

#pragma optimize("s",on)
void CUIActorMenu::script_register(lua_State* L)
{
	module(L)
	[
		class_<enum_exporter<EDDListType>>("EDDListType")
		.enum_("EDDListType")
		[
			value("iActorBag", int(EDDListType::iActorBag)),
			value("iActorBelt", int(EDDListType::iActorBelt)),
			value("iActorSlot", int(EDDListType::iActorSlot)),
			value("iActorTrade", int(EDDListType::iActorTrade)),
			value("iDeadBodyBag", int(EDDListType::iDeadBodyBag)),
			value("iInvalid", int(EDDListType::iInvalid)),
			value("iPartnerTrade", int(EDDListType::iPartnerTrade)),
			value("iPartnerTradeBag", int(EDDListType::iPartnerTradeBag)),
			value("iQuickSlot", int(EDDListType::iQuickSlot)),
			value("iTrashSlot", int(EDDListType::iTrashSlot))
		],

		class_<CUIActorMenu, CUIDialogWnd, CUIWndCallback>("CUIActorMenu")
		.def(constructor<>())
		.def("get_drag_item", &CUIActorMenu::GetCurrentItemAsGameObject)
		.def("highlight_section_in_slot", &CUIActorMenu::HighlightSectionInSlot)
		.def("highlight_for_each_in_slot", &CUIActorMenu::HighlightForEachInSlot)
		.def("refresh_current_cell_item", &CUIActorMenu::RefreshCurrentItemCell)
		.def("IsShown", &CUIActorMenu::IsShown)
		.def("ShowDialog", &CUIActorMenu::ShowDialog)
		.def("HideDialog", &CUIActorMenu::HideDialog)
		.def("ToSlot", &CUIActorMenu::ToSlotScript)
		.def("ToBelt", &CUIActorMenu::ToBeltScript),

		class_<CUIPdaWnd, CUIDialogWnd>("CUIPdaWnd")
		.def(constructor<>())
		.def("IsShown", &CUIPdaWnd::IsShown)
		.def("ShowDialog", &CUIPdaWnd::ShowDialog)
		.def("HideDialog", &CUIPdaWnd::HideDialog)
		.def("SetActiveSubdialog", &CUIPdaWnd::SetActiveSubdialog_script)
		.def("SetActiveDialog", &CUIPdaWnd::SetActiveDialog)
		.def("GetActiveDialog", &CUIPdaWnd::GetActiveDialog)
		.def("GetActiveSection", &CUIPdaWnd::GetActiveSection)
		.def("GetTabControl", &CUIPdaWnd::GetTabControl),

		class_<CUIMainIngameWnd, CUIWindow>("CUIMainIngameWnd")
		.def(constructor<>())
		.def_readonly("UIStaticDiskIO", &CUIMainIngameWnd::UIStaticDiskIO)
		.def_readonly("UIStaticQuickHelp", &CUIMainIngameWnd::UIStaticQuickHelp)
		.def_readonly("UIMotionIcon", &CUIMainIngameWnd::UIMotionIcon)
		.def_readonly("UIZoneMap", &CUIMainIngameWnd::UIZoneMap)
		.def_readonly("m_ui_hud_states", &CUIMainIngameWnd::m_ui_hud_states)
		.def_readonly("m_ind_bleeding", &CUIMainIngameWnd::m_ind_bleeding)
		.def_readonly("m_ind_radiation", &CUIMainIngameWnd::m_ind_radiation)
		.def_readonly("m_ind_starvation", &CUIMainIngameWnd::m_ind_starvation)
		.def_readonly("m_ind_weapon_broken", &CUIMainIngameWnd::m_ind_weapon_broken)
		.def_readonly("m_ind_helmet_broken", &CUIMainIngameWnd::m_ind_helmet_broken)
		.def_readonly("m_ind_outfit_broken", &CUIMainIngameWnd::m_ind_outfit_broken)
		.def_readonly("m_ind_overweight", &CUIMainIngameWnd::m_ind_overweight)
		.def_readonly("m_ind_boost_psy", &CUIMainIngameWnd::m_ind_boost_psy)
		.def_readonly("m_ind_boost_radia", &CUIMainIngameWnd::m_ind_boost_radia)
		.def_readonly("m_ind_boost_chem", &CUIMainIngameWnd::m_ind_boost_chem)
		.def_readonly("m_ind_boost_wound", &CUIMainIngameWnd::m_ind_boost_wound)
		.def_readonly("m_ind_boost_weight", &CUIMainIngameWnd::m_ind_boost_weight)
		.def_readonly("m_ind_boost_health", &CUIMainIngameWnd::m_ind_boost_health)
		.def_readonly("m_ind_boost_power", &CUIMainIngameWnd::m_ind_boost_power)
		.def_readonly("m_ind_boost_rad", &CUIMainIngameWnd::m_ind_boost_rad),

		class_<CUIZoneMap>("CUIZoneMap")
		.def(constructor<>())
		.def_readwrite("disabled", &CUIZoneMap::disabled)
		.def_readonly("visible", &CUIZoneMap::visible)
		.def("MapFrame", &CUIZoneMap::MapFrame)
		.def("Background", &CUIZoneMap::Background),

		class_<CUIMotionIcon, CUIWindow>("CUIMotionIcon")
		.def(constructor<>()),

		class_<CUIMessagesWindow, CUIWindow>("CUIMessagesWindow")
		.def(constructor<>()),

		class_<CUIHudStatesWnd, CUIWindow>("CUIHudStatesWnd")
		.def(constructor<>())
		.def_readonly("m_back", &CUIHudStatesWnd::m_back)
		.def_readonly("m_ui_weapon_ammo_color_active", &CUIHudStatesWnd::m_ui_weapon_ammo_color_active)
		.def_readonly("m_ui_weapon_ammo_color_inactive", &CUIHudStatesWnd::m_ui_weapon_ammo_color_inactive)
		.def_readonly("m_ui_weapon_cur_ammo", &CUIHudStatesWnd::m_ui_weapon_cur_ammo)
		.def_readonly("m_ui_weapon_fmj_ammo", &CUIHudStatesWnd::m_ui_weapon_fmj_ammo)
		.def_readonly("m_ui_weapon_ap_ammo", &CUIHudStatesWnd::m_ui_weapon_ap_ammo)
		.def_readonly("m_ui_weapon_third_ammo", &CUIHudStatesWnd::m_ui_weapon_third_ammo)
		.def_readonly("m_fire_mode", &CUIHudStatesWnd::m_fire_mode)
		.def_readonly("m_ui_grenade", &CUIHudStatesWnd::m_ui_grenade)
		.def_readonly("m_ui_weapon_icon", &CUIHudStatesWnd::m_ui_weapon_icon)
		.def_readonly("m_ui_health_bar", &CUIHudStatesWnd::m_ui_health_bar)
		.def_readonly("m_ui_stamina_bar", &CUIHudStatesWnd::m_ui_stamina_bar)
		.def_readonly("m_ui_psy_bar", &CUIHudStatesWnd::m_ui_psy_bar)
		.def_readonly("m_radia_damage", &CUIHudStatesWnd::m_radia_damage)

		// Tronex
		.def_readwrite("m_ui_health_bar_show", &CUIHudStatesWnd::m_ui_health_bar_show)
		.def_readwrite("m_ui_stamina_bar_show", &CUIHudStatesWnd::m_ui_stamina_bar_show)
		.def_readwrite("m_ui_psy_bar_show", &CUIHudStatesWnd::m_ui_psy_bar_show)
	];

	module(L, "ActorMenu")
	[
		def("get_pda_menu", &GetPDAMenu),
		def("get_actor_menu", &GetActorMenu),
		def("get_menu_mode", &GrabMenuMode),
		def("get_maingame", &GetMainGameMenu),

		// NLTP_ASHES
		def("get_messages_menu", &GetMessagesMenu)
	];
}

#include "pch_script.h"
#include "UIListBox.h"
#include "UIListBoxItem.h"
#include "UIListBoxItemMsgChain.h"
#include "ServerList.h"
#include "UIMapList.h"
#include "UISpinText.h"
#include "UIMapInfo.h"
#include "UIComboBox.h"

using namespace luabind;


struct CUIListBoxItemWrapper : public CUIListBoxItem, public ::luabind::wrap_base
{
	CUIListBoxItemWrapper(float h): CUIListBoxItem(h)
	{
	}
};

struct CUIListBoxItemMsgChainWrapper : public CUIListBoxItemMsgChain, public ::luabind::wrap_base
{
	CUIListBoxItemMsgChainWrapper(float h) : CUIListBoxItemMsgChain(h)
	{
	}
};


#pragma optimize("s",on)
void CUIListBox::script_register(lua_State* L)
{
	module(L)
	[

		class_<CUIListBox, CUIScrollView>("CUIListBox")
		.def(constructor<>())
		.def("ShowSelectedItem", &CUIListBox::Show)
		.def("RemoveAll", &CUIListBox::Clear)
		.def("GetSize", &CUIListBox::GetSize)
		.def("GetSelectedItem", &CUIListBox::GetSelectedItem)
		.def("GetSelectedIndex", &CUIListBox::GetSelectedIDX)
		.def("SetSelectedIndex", &CUIListBox::SetSelectedIDX)
		.def("SetItemHeight", &CUIListBox::SetItemHeight)
		.def("GetItemHeight", &CUIListBox::GetItemHeight)
		.def("GetItemByIndex", &CUIListBox::GetItemByIDX)
		.def("GetItem", &CUIListBox::GetItem)
		.def("RemoveItem", &CUIListBox::RemoveWindow)
		.def("AddTextItem", &CUIListBox::AddTextItem)
		.def("AddExistingItem", &CUIListBox::AddExistingItem, adopt(_2)),

		class_<CUIListBoxItem, CUIFrameLineWnd, CUIListBoxItemWrapper>("CUIListBoxItem")
		.def(constructor<float>())
		.def("GetTextItem", &CUIListBoxItem::GetTextItem)
		.def("AddTextField", &CUIListBoxItem::AddTextField)
		.def("AddIconField", &CUIListBoxItem::AddIconField)
		.def("SetTextColor", &CUIListBoxItem::SetTextColor),

		class_<CUIListBoxItemMsgChain, CUIListBoxItem, CUIListBoxItemMsgChainWrapper>("CUIListBoxItemMsgChain")
		.def(constructor<float>()),

		class_<SServerFilters>("SServerFilters")
		.def(constructor<>())
		.def_readwrite("empty", &SServerFilters::empty)
		.def_readwrite("full", &SServerFilters::full)
		.def_readwrite("with_pass", &SServerFilters::with_pass)
		.def_readwrite("without_pass", &SServerFilters::without_pass)
		.def_readwrite("without_ff", &SServerFilters::without_ff)
		.def_readwrite("listen_servers", &SServerFilters::listen_servers),

		class_<connect_error_cb>("connect_error_cb")
		.def(constructor<>())
		.def(constructor<connect_error_cb::lua_object_type, connect_error_cb::lua_function_type>())
		.def("bind", &connect_error_cb::bind)
		.def("clear", &connect_error_cb::clear),

		class_<CServerList, CUIWindow>("CServerList")
		.def(constructor<>())
		.enum_("enum_connect_errcode")
		[
			value("ece_unique_nick_not_registred", int(ece_unique_nick_not_registred)),
			value("ece_unique_nick_expired", int(ece_unique_nick_expired))
		]
		.def("SetConnectionErrCb", &CServerList::SetConnectionErrCb)
		.def("ConnectToSelected", &CServerList::ConnectToSelected)
		.def("SetFilters", &CServerList::SetFilters)
		.def("SetPlayerName", &CServerList::SetPlayerName)
		.def("RefreshList", &CServerList::RefreshGameSpyList)
		.def("RefreshQuick", &CServerList::RefreshQuick)
		.def("ShowServerInfo", &CServerList::ShowServerInfo)
		.def("NetRadioChanged", &CServerList::NetRadioChanged)
		.def("SetSortFunc", &CServerList::SetSortFunc),


		class_<CUIMapList, CUIWindow>("CUIMapList")
		.def(constructor<>())
		.def("SetWeatherSelector", &CUIMapList::SetWeatherSelector)
		.def("SetModeSelector", &CUIMapList::SetModeSelector)
		.def("OnModeChange", &CUIMapList::OnModeChange)
		.def("LoadMapList", &CUIMapList::LoadMapList)
		.def("SaveMapList", &CUIMapList::SaveMapList)
		.def("GetCommandLine", &CUIMapList::GetCommandLine)
		.def("SetServerParams", &CUIMapList::SetServerParams)
		.def("GetCurGameType", &CUIMapList::GetCurGameType)
		.def("StartDedicatedServer", &CUIMapList::StartDedicatedServer)
		.def("SetMapPic", &CUIMapList::SetMapPic)
		.def("SetMapInfo", &CUIMapList::SetMapInfo)
		.def("ClearList", &CUIMapList::ClearList)
		.def("IsEmpty", &CUIMapList::IsEmpty),

		class_<enum_exporter<EGameIDs>>("GAME_TYPE")
		.enum_("gametype")
		[
			value("GAME_UNKNOWN", int(-1)),
			value("eGameIDDeathmatch", int(eGameIDDeathmatch)),
			value("eGameIDTeamDeathmatch", int(eGameIDTeamDeathmatch)),
			value("eGameIDArtefactHunt", int(eGameIDArtefactHunt)),
			value("eGameIDCaptureTheArtefact", int(eGameIDCaptureTheArtefact))
		]

	];
}

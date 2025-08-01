#include "pch_script.h"
#include "PhraseScript.h"
#include "script_engine.h"
#include "ai_space.h"
#include "gameobject.h"
#include "script_game_object.h"
#include "infoportion.h"
#include "inventoryowner.h"
#include "ai_debug.h"
#include "ui/xrUIXmlParser.h"
#include "actor.h"


//загрузка из XML файла
void CDialogScriptHelper::Load(CUIXml* uiXml, XML_NODE* phrase_node)
{
	LoadSequence(uiXml, phrase_node, "precondition", m_Preconditions);
	LoadSequence(uiXml, phrase_node, "action", m_ScriptActions);

	LoadSequence(uiXml, phrase_node, "has_info", m_HasInfo);
	LoadSequence(uiXml, phrase_node, "dont_has_info", m_DontHasInfo);

	LoadSequence(uiXml, phrase_node, "give_info", m_GiveInfo);
	LoadSequence(uiXml, phrase_node, "disable_info", m_DisableInfo);
}

template <class T>
void CDialogScriptHelper::LoadSequence(CUIXml* uiXml, XML_NODE* phrase_node,
                                       LPCSTR tag, T& str_vector)
{
	int tag_num = uiXml->GetNodesNum(phrase_node, tag);
	str_vector.clear();
	for (int i = 0; i < tag_num; ++i)
	{
		LPCSTR tag_text = uiXml->Read(phrase_node, tag, i, NULL);
		str_vector.push_back(tag_text);
	}
}

bool CDialogScriptHelper::CheckInfo(const CInventoryOwner* pOwner) const
{
	THROW(pOwner);

	for (u32 i = 0; i < m_HasInfo.size(); ++i)
	{
		if (!Actor()->HasInfo(m_HasInfo[i]))
		{
#ifdef DEBUG
			if(psAI_Flags.test(aiDialogs) )
				Msg("----rejected: [%s] has info %s", pOwner->Name(), *m_HasInfo[i]);
#endif
			return false;
		}
	}

	for (u32 i = 0; i < m_DontHasInfo.size(); i++)
	{
		if (Actor()->HasInfo(m_DontHasInfo[i]))
		{
#ifdef DEBUG
			if(psAI_Flags.test(aiDialogs) )
				Msg("----rejected: [%s] dont has info %s", pOwner->Name(), *m_DontHasInfo[i]);
#endif
			return false;
		}
	}
	return true;
}


void CDialogScriptHelper::TransferInfo(const CInventoryOwner* pOwner) const
{
	THROW(pOwner);

	for (u32 i = 0; i < m_GiveInfo.size(); ++i)
		Actor()->TransferInfo(m_GiveInfo[i], true);

	for (u32 i = 0; i < m_DisableInfo.size(); ++i)
		Actor()->TransferInfo(m_DisableInfo[i], false);
}

LPCSTR CDialogScriptHelper::GetScriptText(LPCSTR str_to_translate, const CGameObject* pSpeakerGO1,
                                          const CGameObject* pSpeakerGO2, LPCSTR dialog_id, LPCSTR phrase_id)
{
	if (!m_sScriptTextFunc.size())
		return str_to_translate;

	::luabind::functor<LPCSTR> lua_function;
#ifdef DIALOG_UPGRADE
	::luabind::object parameters_table = ::luabind::newtable(ai().script_engine().lua());
	string256 str = {0};
	xr_sprintf(str, sizeof(str), m_sScriptTextFunc.c_str());
	LPCSTR v1 = strchr(str, '(');
	LPCSTR v2 = strchr(str, ')');
	if (v1 && v2 && (v1 < v2))
	{
		str[v1 - str] = '\0';
		str[v2 - str] = '\0';
		LPCSTR parameters_table_str = v1 + 1;
		int n = _GetItemCount(parameters_table_str, ':');
		for (int k = 0; k < n; k++)
		{
			string64 tmp;
			_GetItem(parameters_table_str, k, tmp, sizeof(tmp), ':');
			parameters_table[k + 1] = tmp;
		}
	}
	bool functor_exists = ai().script_engine().functor(str, lua_function);
	THROW3(functor_exists, "Cannot find phrase script text ", m_sScriptTextFunc.c_str());
	return lua_function(pSpeakerGO1->lua_game_object(), pSpeakerGO2->lua_game_object(), dialog_id, phrase_id, "", parameters_table);
#else
	bool functor_exists = ai().script_engine().functor(m_sScriptTextFunc.c_str(), lua_function);
	THROW3(functor_exists, "Cannot find phrase script text ", m_sScriptTextFunc.c_str());

	LPCSTR res = lua_function(pSpeakerGO1->lua_game_object(),
	                          pSpeakerGO2->lua_game_object(),
	                          dialog_id,
	                          phrase_id);

	return res;
#endif
}

bool CDialogScriptHelper::Precondition(const CGameObject* pSpeakerGO, LPCSTR dialog_id, LPCSTR phrase_id) const
{
	bool predicate_result = true;

	if (!CheckInfo(smart_cast<const CInventoryOwner*>(pSpeakerGO)))
	{
#ifdef DEBUG
			if (psAI_Flags.test(aiDialogs))
				Msg("dialog [%s] phrase[%s] rejected by CheckInfo",dialog_id,phrase_id);
#endif
		return false;
	}

	for (u32 i = 0; i < Preconditions().size(); ++i)
	{
		::luabind::functor<bool> lua_function;
		THROW(*Preconditions()[i]);
		bool functor_exists = ai().script_engine().functor(*Preconditions()[i], lua_function);
		THROW3(functor_exists, "Cannot find precondition", *Preconditions()[i]);
		predicate_result = lua_function(pSpeakerGO->lua_game_object());
		if (!predicate_result)
		{
#ifdef DEBUG
			if (psAI_Flags.test(aiDialogs))
				Msg("dialog [%s] phrase[%s] rejected by script predicate", dialog_id, phrase_id);
#endif
			break;
		}
	}
	return predicate_result;
}

void CDialogScriptHelper::Action(const CGameObject* pSpeakerGO, LPCSTR dialog_id, LPCSTR phrase_id) const
{
	for (u32 i = 0; i < Actions().size(); ++i)
	{
		::luabind::functor<void> lua_function;
		THROW(*Actions()[i]);
		bool functor_exists = ai().script_engine().functor(*Actions()[i], lua_function);
		THROW3(functor_exists, "Cannot find phrase dialog script function", *Actions()[i]);
		lua_function(pSpeakerGO->lua_game_object(), dialog_id);
	}
	TransferInfo(smart_cast<const CInventoryOwner*>(pSpeakerGO));
}

bool CDialogScriptHelper::Precondition(const CGameObject* pSpeakerGO1,
                                       const CGameObject* pSpeakerGO2,
                                       LPCSTR dialog_id,
                                       LPCSTR phrase_id,
                                       LPCSTR next_phrase_id) const
{
	bool predicate_result = true;

	if (!CheckInfo(smart_cast<const CInventoryOwner*>(pSpeakerGO1)))
	{
#ifdef DEBUG
		if (psAI_Flags.test(aiDialogs))
			Msg("dialog [%s] phrase[%s] rejected by CheckInfo",dialog_id,phrase_id);
#endif
		return false;
	}
	for (u32 i = 0; i < Preconditions().size(); ++i)
	{
		::luabind::functor<bool> lua_function;
		THROW(*Preconditions()[i]);

#ifdef DIALOG_UPGRADE
		::luabind::object parameters_table = ::luabind::newtable(ai().script_engine().lua());
		string256 str = {0};
		xr_sprintf(str, sizeof(str), Preconditions()[i].c_str());
		LPCSTR v1 = strchr(str, '(');
		LPCSTR v2 = strchr(str, ')');
		if (v1 && v2 && (v1 < v2))
		{
			str[v1 - str] = '\0';
			str[v2 - str] = '\0';
			LPCSTR parameters_table_str = v1 + 1;
			int n = _GetItemCount(parameters_table_str, ':');
			for (int k = 0; k < n; k++)
			{
				string64 tmp;
				_GetItem(parameters_table_str, k, tmp, sizeof(tmp), ':');
				parameters_table[k + 1] = tmp;
			}
		}
		bool is_positive = true;
		string256 lua_function_str = {0};
		GetLuaFunctionStringAndHeaderFlag(str, lua_function_str, sizeof(lua_function_str), is_positive);
		bool functor_exists = ai().script_engine().functor(lua_function_str, lua_function);
		THROW3(functor_exists, "Cannot find phrase precondition", Preconditions()[i].c_str());
		predicate_result = lua_function(pSpeakerGO1->lua_game_object(), pSpeakerGO2->lua_game_object(), dialog_id, phrase_id, next_phrase_id, parameters_table);
		predicate_result = (is_positive == true) ? predicate_result : !predicate_result;
#else
		bool functor_exists = ai().script_engine().functor(*Preconditions()[i], lua_function);
		THROW3(functor_exists, "Cannot find phrase precondition", *Preconditions()[i]);
		predicate_result = lua_function(pSpeakerGO1->lua_game_object(), pSpeakerGO2->lua_game_object(), dialog_id,
		                                phrase_id, next_phrase_id);
#endif
		if (!predicate_result)
		{
#ifdef DEBUG
			if (psAI_Flags.test(aiDialogs))
				Msg("dialog [%s] phrase[%s] rejected by script predicate",dialog_id,phrase_id);
#endif
			break;
		}
	}
	return predicate_result;
}

void CDialogScriptHelper::Action(const CGameObject* pSpeakerGO1, const CGameObject* pSpeakerGO2, LPCSTR dialog_id,
                                 LPCSTR phrase_id) const
{
	TransferInfo(smart_cast<const CInventoryOwner*>(pSpeakerGO1));

	for (u32 i = 0; i < Actions().size(); ++i)
	{
		::luabind::functor<void> lua_function;
		THROW(*Actions()[i]);

#ifdef DIALOG_UPGRADE
		::luabind::object parameters_table = ::luabind::newtable(ai().script_engine().lua());
		string256 str = {0};
		xr_sprintf(str, sizeof(str), Actions()[i].c_str());
		LPCSTR v1 = strchr(str, '(');
		LPCSTR v2 = strchr(str, ')');
		if (v1 && v2 && (v1 < v2))
		{
			str[v1 - str] = '\0';
			str[v2 - str] = '\0';
			LPCSTR parameters_table_str = v1 + 1;
			int n = _GetItemCount(parameters_table_str, ':');
			for (int k = 0; k < n; k++)
			{
				string64 tmp;
				_GetItem(parameters_table_str, k, tmp, sizeof(tmp), ':');
				parameters_table[k + 1] = tmp;
			}
		}
		bool is_positive = true;
		string256 lua_function_str = {0};
		GetLuaFunctionStringAndHeaderFlag(str, lua_function_str, sizeof(lua_function_str), is_positive);
		bool functor_exists = ai().script_engine().functor(lua_function_str, lua_function);
		THROW3(functor_exists, "Cannot find phrase dialog script function", Actions()[i].c_str());
		try
		{
			lua_function(pSpeakerGO1->lua_game_object(), pSpeakerGO2->lua_game_object(), dialog_id, phrase_id, "", parameters_table);
		}
		catch (...)
		{
		}
#else
		bool functor_exists = ai().script_engine().functor(*Actions()[i], lua_function);
		THROW3(functor_exists, "Cannot find phrase dialog script function", *Actions()[i]);
		try
		{
			lua_function(pSpeakerGO1->lua_game_object(), pSpeakerGO2->lua_game_object(), dialog_id, phrase_id);
		}
		catch (...)
		{
		}
#endif
	}
}

#ifdef DIALOG_UPGRADE
void CDialogScriptHelper::GetLuaFunctionStringAndHeaderFlag(char *str, char *dst, int dst_size, bool &is_positive) const
{
	if (str && (str[0] != 0))
	{
		switch (str[0])
		{
		case '!':
			xr_sprintf(dst, dst_size, str + 1);
			is_positive = false;
			break;
		case '=':
			xr_sprintf(dst, dst_size, str + 1);
			is_positive = true;
			break;
		default:
			is_positive = true;
			xr_sprintf(dst, dst_size, str);
			break;
		}
	}
}
#endif

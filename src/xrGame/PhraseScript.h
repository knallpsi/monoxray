///////////////////////////////////////////////////////////////
// PhraseScript.h
// классы для связи диалогов со скриптами
///////////////////////////////////////////////////////////////


#include "InfoPortionDefs.h"

#ifndef DIALOG_UPGRADE
#define DIALOG_UPGRADE
#endif

#pragma once

class CGameObject;
class CInventoryOwner;
class TiXmlNode;
class CUIXml;

typedef TiXmlNode XML_NODE;

class CDialogScriptHelper
{
public:

	void Load(CUIXml* ui_xml, XML_NODE* phrase_node);

	bool Precondition(const CGameObject* pSpeaker, LPCSTR dialog_id, LPCSTR phrase_id) const;
	void Action(const CGameObject* pSpeaker, LPCSTR dialog_id, LPCSTR phrase_id) const;

	bool Precondition(const CGameObject* pSpeaker1, const CGameObject* pSpeaker2, LPCSTR dialog_id, LPCSTR phrase_id,
	                  LPCSTR next_phrase_id) const;
	void Action(const CGameObject* pSpeaker1, const CGameObject* pSpeaker2, LPCSTR dialog_id, LPCSTR phrase_id) const;


	DEFINE_VECTOR(shared_str, PRECONDITION_VECTOR, PRECONDITION_VECTOR_IT);
	const PRECONDITION_VECTOR& Preconditions() const { return m_Preconditions; }

	DEFINE_VECTOR(shared_str, ACTION_NAME_VECTOR, ACTION_NAME_VECTOR_IT);
	const ACTION_NAME_VECTOR& Actions() const { return m_ScriptActions; }


	void AddPrecondition(LPCSTR str);
	void AddAction(LPCSTR str);
	void AddHasInfo(LPCSTR str);
	void AddDontHasInfo(LPCSTR str);
	void AddGiveInfo(LPCSTR str);
	void AddDisableInfo(LPCSTR str);
	void SetScriptText(LPCSTR str) { m_sScriptTextFunc = str; };
	LPCSTR GetScriptText(LPCSTR str_to_translate, const CGameObject* pSpeakerGO1, const CGameObject* pSpeakerGO2,
	                     LPCSTR dialog_id, LPCSTR phrase_id);
protected:
	//загрузка содержания последовательности тагов в контейнер строк 
	template <class T>
	void LoadSequence(CUIXml* ui_xml, XML_NODE* phrase_node, LPCSTR tag, T& str_vector);

	//манипуляции с информацией во время вызовов Precondition и Action 
	virtual bool CheckInfo(const CInventoryOwner* pOwner) const;
	virtual void TransferInfo(const CInventoryOwner* pOwner) const;

	//имя скриптовой функции, которая возвращает какой-то текст
	shared_str m_sScriptTextFunc;

	//скриптовые действия, которые активируется после того как 
	//говорится фраза
	DEFINE_VECTOR(shared_str, ACTION_NAME_VECTOR, ACTION_NAME_VECTOR_IT);
	ACTION_NAME_VECTOR m_ScriptActions;

	DEFINE_VECTOR(shared_str, INFO_VECTOR, INFO_VECTOR_IT);

	INFO_VECTOR m_GiveInfo;
	INFO_VECTOR m_DisableInfo;

	//список скриптовых предикатов, выполнение, которых необходимо
	//для того чтоб фраза стала доступной
	DEFINE_VECTOR(shared_str, PRECONDITION_VECTOR, PRECONDITION_VECTOR_IT);

	PRECONDITION_VECTOR m_Preconditions;
	//проверка наличия/отсутствия информации
	INFO_VECTOR m_HasInfo;
	INFO_VECTOR m_DontHasInfo;

#ifdef DIALOG_UPGRADE
public:
	void GetLuaFunctionStringAndHeaderFlag(char *str, char *dst, int dst_size, bool &is_positive) const;
#endif
};

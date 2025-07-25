#include "stdafx.h"
#include "string_table.h"

#include "ui/xrUIXmlParser.h"
#include "xr_level_controller.h"
#include "..\..\xrEngine\x_ray.h"
#include "MainMenu.h"
#include "UIGameCustom.h"
#include <regex>

STRING_TABLE_DATA* CStringTable::pData = NULL;
BOOL CStringTable::m_bWriteErrorsToLog = FALSE;

CStringTable::CStringTable()
{
	Init();
}

void CStringTable::Destroy()
{
	xr_delete(pData);
}

void CStringTable::rescan()
{
	if (NULL != pData) return;
	Destroy();
	Init();
}

STRING_TABLE_DATA* CStringTable::getPData()
{
	return pData;
}

extern void refresh_npc_names();

// demonized: use english text if locale text string is missing
BOOL use_english_text_for_missing_translations = TRUE;
void CStringTable::Init()
{
	if (NULL != pData) return;

	pData = xr_new<STRING_TABLE_DATA>();

	//имя языка, если не задано (NULL), то первый <text> в <string> в XML
	pData->m_sLanguage = READ_IF_EXISTS(pSettings, r_string, "string_table", "language", "eng");

	// demonized: parse english files first, then they will be replaced by current locale
	if (use_english_text_for_missing_translations && xr_strcmp(pData->m_sLanguage, "eng") != 0) {
		FS_FileSet fset;
		string_path files_mask;
		xr_sprintf(files_mask, "text\\%s\\*.xml", "eng");
		FS.file_list(fset, "$game_config$", FS_ListFiles, files_mask);
		FS_FileSetIt fit = fset.begin();
		FS_FileSetIt fit_e = fset.end();

		for (; fit != fit_e; ++fit)
		{
			string_path fn, ext;
			_splitpath((*fit).name.c_str(), 0, 0, fn, ext);
			xr_strcat(fn, ext);

			Load(fn, "eng");
		}
	}

	//---
	FS_FileSet fset;
	string_path files_mask;
	xr_sprintf(files_mask, "text\\%s\\*.xml", pData->m_sLanguage.c_str());
	FS.file_list(fset, "$game_config$", FS_ListFiles, files_mask);
	FS_FileSetIt fit = fset.begin();
	FS_FileSetIt fit_e = fset.end();

	for (; fit != fit_e; ++fit)
	{
		string_path fn, ext;
		_splitpath((*fit).name.c_str(), 0, 0, fn, ext);
		xr_strcat(fn, ext);

		Load(fn);
	}
#ifdef DEBUG
	Msg("StringTable: loaded %d files", fset.size());
#endif // #ifdef DEBUG
	//---
	ReparseKeyBindings();

	//Discord
	snprintf(discord_strings.mainmenu, 128, xr_ToUTF8(*CStringTable().translate("st_main_menu")));
	snprintf(discord_strings.paused, 128, xr_ToUTF8(*CStringTable().translate("st_pause_menu")));
	snprintf(discord_strings.loading, 128, xr_ToUTF8(*CStringTable().translate("st_loading")));
	snprintf(discord_strings.health, 128, xr_ToUTF8(*CStringTable().translate("st_ui_health_sensor")));
	snprintf(discord_strings.dead, 128, xr_ToUTF8(*CStringTable().translate("st_player_dead")));
	snprintf(discord_strings.livesleft, 128, xr_ToUTF8(*CStringTable().translate("st_hardcore_lives_left")));
	snprintf(discord_strings.livesleftsingle, 128, xr_ToUTF8(*CStringTable().translate("st_hardcore_lives_left_single")));
	snprintf(discord_strings.livespossessed, 128, xr_ToUTF8(*CStringTable().translate("st_azazel_lives_possessed")));
	snprintf(discord_strings.livespossessedsingle, 128, xr_ToUTF8(*CStringTable().translate("st_azazel_lives_possessed_single")));
	snprintf(discord_strings.godmode, 128, xr_ToUTF8(*CStringTable().translate("st_godmode")));

	discord_gameinfo.ex_update = true;
}

void CStringTable::Load(LPCSTR xml_file_full, LPCSTR lang_in)
{
	LPCSTR lang = lang_in ? lang_in : pData->m_sLanguage.c_str();
	CUIXml uiXml;
	string_path _s;
	strconcat(sizeof(_s), _s, "text\\", lang);

	uiXml.Load(CONFIG_PATH, _s, xml_file_full);

	//общий список всех записей таблицы в файле
	int string_num = uiXml.GetNodesNum(uiXml.GetRoot(), "string");

	for (int i = 0; i < string_num; ++i)
	{
		LPCSTR string_name = uiXml.ReadAttrib(uiXml.GetRoot(), "string", i, "id", NULL);

		VERIFY3(pData->m_StringTable.find(string_name) == pData->m_StringTable.end(), "duplicate string table id",
		        string_name);

		LPCSTR string_text = uiXml.Read(uiXml.GetRoot(), "string:text", i, NULL);

		if (m_bWriteErrorsToLog && string_text)
			Msg("[string table] '%s' no translation in '%s'", string_name, lang);

		R_ASSERT3(string_text, "string table entry does not have a text", string_name);

		STRING_VALUE str_val = ParseLine(string_text, string_name, true);

		pData->m_StringTable[string_name] = str_val;
	}
}

void CStringTable::ReparseKeyBindings()
{
	if (!pData) return;
	STRING_TABLE_MAP_IT it = pData->m_string_key_binding.begin();
	STRING_TABLE_MAP_IT it_e = pData->m_string_key_binding.end();

	for (; it != it_e; ++it)
	{
		pData->m_StringTable[it->first] = ParseLine(*it->second, *it->first, false);
	}
}

void CStringTable::ReloadLanguage()
{
	if (0 == xr_strcmp(READ_IF_EXISTS(pSettings, r_string, "string_table", "language", "eng"), *(pData->m_sLanguage)))
		return;

	//reload language
	Destroy();
	Init();

	//reload language in menu
	if (MainMenu()->IsActive())
	{
		MainMenu()->Activate(FALSE);
		MainMenu()->Activate(TRUE);
	}

	if (!g_pGameLevel)
		return;

	//refresh npc names
	refresh_npc_names();

	//reload language in other UIs
	g_hud->OnScreenResolutionChanged();
}

STRING_VALUE CStringTable::ParseLine(LPCSTR str, LPCSTR skey, bool bFirst)
{
	//	LPCSTR str = "1 $$action_left$$ 2 $$action_right$$ 3 $$action_left$$ 4";
	xr_string res;
	int k = 0;
	const char* b;
#define ACTION_STR "$$ACTION_"

	//.	int LEN				= (int)xr_strlen(ACTION_STR);
#define LEN			9

	string256 buff;
	string256 srcbuff;
	bool b_hit = false;

	while ((b = strstr(str + k,ACTION_STR)) != 0)
	{
		buff[0] = 0;
		srcbuff[0] = 0;
		res.append(str + k, b - str - k);
		const char* e = strstr(b + LEN, "$$");

		int len = (int)(e - b - LEN);

		strncpy_s(srcbuff, b + LEN, len);
		srcbuff[len] = 0;
		GetActionAllBinding(srcbuff, buff, sizeof(buff));
		res.append(buff, xr_strlen(buff));

		k = (int)(b - str);
		k += len;
		k += LEN;
		k += 2;
		b_hit = true;
	};

	if (k < (int)xr_strlen(str))
	{
		res.append(str + k);
	}

	if (b_hit && bFirst) pData->m_string_key_binding[skey] = str;

	if (Core.april1)
	{
		res = std::regex_replace(res, std::regex("Anomal"), "Amomaw");
		res = std::regex_replace(res, std::regex("anomal"), "amomaw");
	}
	return STRING_VALUE(res.c_str());
}

STRING_VALUE CStringTable::translate(const STRING_ID& str_id) const
{
	VERIFY(pData);

	if (pData->m_StringTable.find(str_id) != pData->m_StringTable.end())
		return pData->m_StringTable[str_id];
	else
		return str_id;
}

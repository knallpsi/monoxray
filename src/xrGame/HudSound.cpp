#include "stdafx.h"

#include "HudSound.h"

#include "pch_script.h"
#include "script_game_object.h"


float psHUDSoundVolume = 1.0f;
float psHUDStepSoundVolume = 1.0f;

void InitHudSoundSettings()
{
	psHUDSoundVolume = pSettings->r_float("hud_sound", "hud_sound_vol_k");
	psHUDStepSoundVolume = pSettings->r_float("hud_sound", "hud_step_sound_vol_k");
}

void HUD_SOUND_ITEM::LoadSound(LPCSTR section, LPCSTR line,
                               HUD_SOUND_ITEM& hud_snd, int type)
{
	hud_snd.m_activeSnd = NULL;
	hud_snd.sounds.clear();

	string256 sound_line;
	xr_strcpy(sound_line, line);
	int k = 0;
	while (pSettings->line_exist(section, sound_line))
	{
		hud_snd.sounds.push_back(SSnd());
		SSnd& s = hud_snd.sounds.back();

		LoadSound(section, sound_line, s.snd, type, &s.volume, &s.delay);
		xr_sprintf(sound_line, "%s%d", line, ++k);
	} //while
}

void HUD_SOUND_ITEM::LoadSound(LPCSTR section,
                               LPCSTR line,
                               ref_sound& snd,
                               int type,
                               float* volume,
                               float* delay)
{
	LPCSTR str = pSettings->r_string(section, line);
	string256 buf_str;

	int count = _GetItemCount(str);
	R_ASSERT(count);

	_GetItem(str, 0, buf_str);
	snd.create(buf_str, st_Effect, type);


	if (volume != NULL)
	{
		*volume = 1.f;
		if (count > 1)
		{
			_GetItem(str, 1, buf_str);
			if (xr_strlen(buf_str) > 0)
				*volume = (float)atof(buf_str);
		}
	}

	if (delay != NULL)
	{
		*delay = 0;
		if (count > 2)
		{
			_GetItem(str, 2, buf_str);
			if (xr_strlen(buf_str) > 0)
				*delay = (float)atof(buf_str);
		}
	}
}

void HUD_SOUND_ITEM::DestroySound(HUD_SOUND_ITEM& hud_snd)
{
	xr_vector<SSnd>::iterator it = hud_snd.sounds.begin();
	for (; it != hud_snd.sounds.end(); ++it)
		(*it).snd.destroy();
	hud_snd.sounds.clear();

	hud_snd.m_activeSnd = NULL;
}

void HUD_SOUND_ITEM::PlaySound(HUD_SOUND_ITEM& hud_snd,
                               const Fvector& position,
                               const CObject* parent,
                               bool b_hud_mode,
                               bool looped,
                               u8 index,
							   float volume_mult)
{
	if (hud_snd.sounds.empty()) return;

	hud_snd.m_activeSnd = NULL;
	StopSound(hud_snd);

	u32 flags = b_hud_mode ? sm_2D : 0;
	if (looped)
		flags |= sm_Looped;

	//Alundaio: Sanity, don't allow PlaySound of index greater then the size, just play last index
	if (index == u8(-1))
	{
		index = (u8)Random.randI(hud_snd.sounds.size());
	}
	else
	{
		if (index >= (u8)hud_snd.sounds.size())
			index = (u8)hud_snd.sounds.size() - 1;
	}
	//-Alundaio

	hud_snd.m_activeSnd = &hud_snd.sounds[index];

	if (hud_snd.m_b_exclusive)
	{
		hud_snd.m_activeSnd->snd.play_at_pos(const_cast<CObject*>(parent),
		                                     flags & sm_2D ? Fvector().set(0, 0, 0) : position,
		                                     flags,
		                                     hud_snd.m_activeSnd->delay);
	}
	else
	{
		hud_snd.m_activeSnd->snd.play_no_feedback(const_cast<CObject*>(parent),
		                                          flags,
		                                          hud_snd.m_activeSnd->delay,
		                                          flags & sm_2D
			                                          ? &Fvector().set(0, 0, 0)
			                                          : &Fvector().set(position.x, position.y, position.z),
		                                          &volume_mult, 0, 0);
	}

	//hud_snd.m_activeSnd->snd.set_volume		(hud_snd.m_activeSnd->volume * b_hud_mode?psHUDSoundVolume:1.0f);
	hud_snd.m_activeSnd->snd.set_volume(hud_snd.m_activeSnd->volume * (b_hud_mode ? psHUDSoundVolume : 1.0f) * volume_mult);
}

void HUD_SOUND_ITEM::StopSound(HUD_SOUND_ITEM& hud_snd)
{
	xr_vector<SSnd>::iterator it = hud_snd.sounds.begin();
	for (; it != hud_snd.sounds.end(); ++it)
		(*it).snd.stop();
	hud_snd.m_activeSnd = NULL;
}

//----------------------------------------------------------
HUD_SOUND_COLLECTION::~HUD_SOUND_COLLECTION()
{
	xr_vector<HUD_SOUND_ITEM>::iterator it = m_sound_items.begin();
	xr_vector<HUD_SOUND_ITEM>::iterator it_e = m_sound_items.end();

	for (; it != it_e; ++it)
	{
		HUD_SOUND_ITEM::StopSound(*it);
		HUD_SOUND_ITEM::DestroySound(*it);
	}

	m_sound_items.clear();
}

HUD_SOUND_ITEM* HUD_SOUND_COLLECTION::FindSoundItem(LPCSTR alias, bool b_assert)
{
	xr_vector<HUD_SOUND_ITEM>::iterator it = std::find(m_sound_items.begin(), m_sound_items.end(), alias);

	if (it != m_sound_items.end())
		return &*it;
	else
	{
		R_ASSERT3(!b_assert, "sound item not found in collection", alias);
		return NULL;
	}
}

void HUD_SOUND_COLLECTION::PlaySound(LPCSTR alias,
                                     const Fvector& position,
                                     const CObject* parent,
                                     bool hud_mode,
                                     bool looped,
                                     u8 index,
									 float volume_mult)
{
	xr_vector<HUD_SOUND_ITEM>::iterator it = m_sound_items.begin();
	xr_vector<HUD_SOUND_ITEM>::iterator it_e = m_sound_items.end();
	for (; it != it_e; ++it)
	{
		if (it->m_b_exclusive)
			HUD_SOUND_ITEM::StopSound(*it);
	}

	HUD_SOUND_ITEM* snd_item = FindSoundItem(alias, false);
	if (snd_item)
		HUD_SOUND_ITEM::PlaySound(*snd_item, position, parent, hud_mode, looped, index, volume_mult);
}

void HUD_SOUND_COLLECTION::StopSound(LPCSTR alias)
{
	HUD_SOUND_ITEM* snd_item = FindSoundItem(alias, true);
	HUD_SOUND_ITEM::StopSound(*snd_item);
}

void HUD_SOUND_COLLECTION::SetPosition(LPCSTR alias, const Fvector& pos)
{
	HUD_SOUND_ITEM* snd_item = FindSoundItem(alias, false);
	if (snd_item && snd_item->playing())
		snd_item->set_position(pos);
}

void HUD_SOUND_COLLECTION::StopAllSounds()
{
	xr_vector<HUD_SOUND_ITEM>::iterator it = m_sound_items.begin();
	xr_vector<HUD_SOUND_ITEM>::iterator it_e = m_sound_items.end();

	for (; it != it_e; ++it)
	{
		HUD_SOUND_ITEM::StopSound(*it);
	}
}

void HUD_SOUND_COLLECTION::LoadSound(LPCSTR section, LPCSTR line, LPCSTR alias, bool exclusive, int type)
{
	R_ASSERT(NULL==FindSoundItem(alias, false));
	m_sound_items.resize(m_sound_items.size() + 1);
	HUD_SOUND_ITEM& snd_item = m_sound_items.back();
	HUD_SOUND_ITEM::LoadSound(section, line, snd_item, type);
	snd_item.m_alias = alias;
	snd_item.m_b_exclusive = exclusive;
	snd_item.m_type = type;
}

//Alundaio:
/*
It's usage is to play a group of sounds HUD_SOUND_ITEMs as if they were a single layered entity. This is a achieved by
wrapping the class around HUD_SOUND_COLLECTION and tagging them with the same alias. This way, when one for example
sndShot is played, it will play all the sound items with the same alias.
*/
//----------------------------------------------------------
HUD_SOUND_COLLECTION_LAYERED::~HUD_SOUND_COLLECTION_LAYERED()
{
	xr_vector<HUD_SOUND_COLLECTION>::iterator it = m_sound_items.begin();
	xr_vector<HUD_SOUND_COLLECTION>::iterator it_e = m_sound_items.end();

	for (; it != it_e; ++it)
	{
		it->~HUD_SOUND_COLLECTION();
	}

	m_sound_items.clear();
}

void HUD_SOUND_COLLECTION_LAYERED::StopAllSounds()
{
	xr_vector<HUD_SOUND_COLLECTION>::iterator it = m_sound_items.begin();
	xr_vector<HUD_SOUND_COLLECTION>::iterator it_e = m_sound_items.end();

	for (; it != it_e; ++it)
	{
		it->StopAllSounds();
	}
}

void HUD_SOUND_COLLECTION_LAYERED::StopSound(LPCSTR alias)
{
	xr_vector<HUD_SOUND_COLLECTION>::iterator it = m_sound_items.begin();
	xr_vector<HUD_SOUND_COLLECTION>::iterator it_e = m_sound_items.end();

	for (; it != it_e; ++it)
	{
		if (it->m_alias == alias)
			it->StopSound(alias);
	}
}

void HUD_SOUND_COLLECTION_LAYERED::SetPosition(LPCSTR alias, const Fvector& pos)
{
	xr_vector<HUD_SOUND_COLLECTION>::iterator it = m_sound_items.begin();
	xr_vector<HUD_SOUND_COLLECTION>::iterator it_e = m_sound_items.end();

	for (; it != it_e; ++it)
	{
		if (it->m_alias == alias)
			it->SetPosition(alias, pos);
	}
}

void HUD_SOUND_COLLECTION_LAYERED::PlaySound(LPCSTR alias, const Fvector& position, const CObject* parent,
                                             bool hud_mode, bool looped, u8 index, float volume_mult)
{
	LPCSTR alias_to_play = alias;
	::luabind::functor<::luabind::object> funct;
	if (ai().script_engine().functor("_G.COnBeforePlayHudSound", funct))
	{
		const CGameObject* parent_game_object = smart_cast<const CGameObject*>(parent);
		if (parent_game_object)
		{
			auto parent_lua_game_object = parent_game_object->lua_game_object();
			::luabind::object output = funct(alias, parent_lua_game_object);
			if (output && output.type() == LUA_TTABLE)
			{
				LPCSTR section = ::luabind::object_cast<LPCSTR>(output["section"]);
				LPCSTR line = ::luabind::object_cast<LPCSTR>(output["line"]);
				if (!section)
				{
					Msg("!_G.COnBeforePlayHudSound callback, HUD_SOUND_COLLECTION_LAYERED::PlaySound, failed to override sound item %s, no section specified", alias);
				} 
				else if (!line)
				{
					Msg("!_G.COnBeforePlayHudSound callback, HUD_SOUND_COLLECTION_LAYERED::PlaySound, failed to override sound item %s, no line specified", alias);
				} 
				else
				{
					xr_string new_alias = xr_string(section) + "|" + line;
					if (!FindSoundItem(new_alias.c_str(), false))
					{
						auto old_sound = FindSoundItem(alias, false);
						if (old_sound)
						{
							alias_to_play = new_alias.c_str();
							LoadSound(section, line, alias_to_play, old_sound->m_b_exclusive, old_sound->m_type);

							// Crash if override sound was defined but wasn't loaded correctly
							R_ASSERT2(FindSoundItem(alias_to_play, false), make_string("!_G.COnBeforePlayHudSound callback, HUD_SOUND_COLLECTION_LAYERED::PlaySound, failed to override sound item %s with %s, failed to load section %s line %s", alias, alias_to_play, section, line));
						}
						else
						{
							Msg("!_G.COnBeforePlayHudSound callback, HUD_SOUND_COLLECTION_LAYERED::PlaySound, failed to override sound item %s with %s, sound item by original alias %s not found", alias, new_alias.c_str(), alias);
						}
					} 
					else
					{
						alias_to_play = new_alias.c_str();
						//Msg("HUD_SOUND_COLLECTION_LAYERED::PlaySound, overriding sound item %s with %s, already exists", alias, alias_to_play);
					}
				}
			}
		}
	}

	xr_vector<HUD_SOUND_COLLECTION>::iterator it = m_sound_items.begin();
	xr_vector<HUD_SOUND_COLLECTION>::iterator it_e = m_sound_items.end();

	for (; it != it_e; ++it)
	{
		if (it->m_alias == alias_to_play)
			it->PlaySound(alias_to_play, position, parent, hud_mode, looped, index, volume_mult);
	}
}


HUD_SOUND_ITEM* HUD_SOUND_COLLECTION_LAYERED::FindSoundItem(LPCSTR alias, bool b_assert)
{
	xr_vector<HUD_SOUND_COLLECTION>::iterator it = m_sound_items.begin();
	xr_vector<HUD_SOUND_COLLECTION>::iterator it_e = m_sound_items.end();

	for (; it != it_e; ++it)
	{
		if (it->m_alias == alias)
			return it->FindSoundItem(alias, b_assert);
	}
	return (0);
}

void HUD_SOUND_COLLECTION_LAYERED::LoadSound(LPCSTR section, LPCSTR line, LPCSTR alias, bool exclusive, int type)
{
	LoadSound(pSettings, section, line, alias, exclusive, type);
}

void HUD_SOUND_COLLECTION_LAYERED::LoadSound(CInifile const* ini, LPCSTR section, LPCSTR line, LPCSTR alias,
                                             bool exclusive, int type)
{
	if (!ini->line_exist(section, line))
		return;

	LPCSTR str = ini->r_string(section, line);
	string256 buf_str;

	int count = _GetItemCount(str);
	R_ASSERT(count);

	_GetItem(str, 0, buf_str);

	if (ini->section_exist(buf_str))
	{
		string256 sound_line;
		xr_strcpy(sound_line, "snd_1_layer");
		int k = 1;
		while (ini->line_exist(buf_str, sound_line))
		{
			m_sound_items.resize(m_sound_items.size() + 1);
			HUD_SOUND_COLLECTION& snd_item = m_sound_items.back();
			snd_item.LoadSound(buf_str, sound_line, alias, exclusive, type);
			snd_item.m_alias = alias;
			xr_sprintf(sound_line, "snd_%d_layer", ++k);
		}
	}
	else //For compatibility with normal HUD_SOUND_COLLECTION sounds
	{
		m_sound_items.resize(m_sound_items.size() + 1);
		HUD_SOUND_COLLECTION& snd_item = m_sound_items.back();
		snd_item.LoadSound(section, line, alias, exclusive, type);
		snd_item.m_alias = alias;
	}
}
//-Alundaio

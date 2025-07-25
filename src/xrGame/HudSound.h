#pragma once

struct HUD_SOUND_ITEM
{
	HUD_SOUND_ITEM() : m_activeSnd(NULL), m_b_exclusive(false)
	{
	}

	static void LoadSound(LPCSTR section, LPCSTR line,
	                      ref_sound& hud_snd,
	                      int type = sg_SourceType,
	                      float* volume = NULL,
	                      float* delay = NULL);

	static void LoadSound(LPCSTR section,
	                      LPCSTR line,
	                      HUD_SOUND_ITEM& hud_snd,
	                      int type = sg_SourceType);

	static void DestroySound(HUD_SOUND_ITEM& hud_snd);

	static void PlaySound(HUD_SOUND_ITEM& snd,
	                      const Fvector& position,
	                      const CObject* parent,
	                      bool hud_mode,
	                      bool looped = false,
	                      u8 index = u8(-1),
						  float volume_mult = 1.f);

	static void StopSound(HUD_SOUND_ITEM& snd);

	ICF BOOL playing()
	{
		if (m_activeSnd) return m_activeSnd->snd._feedback() ? TRUE : FALSE;
		else return FALSE;
	}

	ICF void set_position(const Fvector& pos)
	{
		if (m_activeSnd)
		{
			if (m_activeSnd->snd._feedback() && !m_activeSnd->snd._feedback()->is_2D())
				m_activeSnd->snd.set_position(pos);
			else m_activeSnd = NULL;
		}
	}

	struct SSnd
	{
		ref_sound snd;
		float delay; //задержка перед проигрыванием
		float volume; //громкость
	};

	shared_str m_alias;
	SSnd* m_activeSnd;
	bool m_b_exclusive;
	int m_type;
	xr_vector<SSnd> sounds;

	bool operator ==(LPCSTR alias) const
	{
		return 0 == stricmp(m_alias.c_str(), alias);
	}
};

class HUD_SOUND_COLLECTION
{
public:
	~HUD_SOUND_COLLECTION();
	shared_str m_alias; //Alundaio: For use when it's part of a layered Collection
	xr_vector<HUD_SOUND_ITEM> m_sound_items; //Alundaio: made public

	HUD_SOUND_ITEM* FindSoundItem(LPCSTR alias, bool b_assert); //AVO: made public to check if sound is loaded
	void PlaySound(LPCSTR alias, const Fvector& position, const CObject* parent, bool hud_mode, bool looped = false,
	               u8 index = u8(-1), float volume_mult = 1.f);

	void StopSound(LPCSTR alias);

	void LoadSound(LPCSTR section, LPCSTR line, LPCSTR alias, bool exclusive = false, int type = sg_SourceType);

	void SetPosition(LPCSTR alias, const Fvector& pos);
	void StopAllSounds();
};

//Alundaio:
class HUD_SOUND_COLLECTION_LAYERED
{
	xr_vector<HUD_SOUND_COLLECTION> m_sound_items;
public:
	~HUD_SOUND_COLLECTION_LAYERED();
	HUD_SOUND_ITEM* FindSoundItem(LPCSTR alias, bool b_assert);
	void PlaySound(LPCSTR alias, const Fvector& position, const CObject* parent, bool hud_mode, bool looped = false,
	               u8 index = u8(-1), float volume_mult = 1.f);
	void StopSound(LPCSTR alias);
	void StopAllSounds();
	void LoadSound(LPCSTR section, LPCSTR line, LPCSTR alias, bool exclusive = false, int type = sg_SourceType);
	void LoadSound(CInifile const* ini, LPCSTR section, LPCSTR line, LPCSTR alias, bool exclusive = false,
	               int type = sg_SourceType);
	void SetPosition(LPCSTR alias, const Fvector& pos);
};
//-Alundaio

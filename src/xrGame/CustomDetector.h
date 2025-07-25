#pragma once
#include "customdevice.h"
#include "../xrEngine/feel_touch.h"
#include "hudsound.h"
#include "customzone.h"
#include "artefact.h"
#include "ai_sounds.h"
//#include "ui/ArtefactDetectorUI.h"

class CCustomZone;
class CInventoryOwner;

struct ITEM_TYPE
{
	Fvector2 freq; //min,max
	HUD_SOUND_ITEM detect_snds;

	shared_str zone_map_location;
	shared_str nightvision_particle;
};

//описание зоны, обнаруженной детектором
struct ITEM_INFO
{
	ITEM_TYPE* curr_ref;
	float snd_time;
	//текущая частота работы датчика
	float cur_period;
	//particle for night-vision mode
	CParticlesObject* pParticle;

	ITEM_INFO();
	~ITEM_INFO();
};

template <typename K>
class CDetectList : public Feel::Touch
{
protected:
	typedef xr_map<shared_str, ITEM_TYPE> TypesMap;
	typedef typename TypesMap::iterator TypesMapIt;
	TypesMap m_TypesMap;
public:
	typedef xr_map<K*, ITEM_INFO> ItemsMap;
	typedef typename ItemsMap::iterator ItemsMapIt;
	ItemsMap m_ItemInfos;

protected:
	virtual void feel_touch_new(CObject* O)
	{
		K* pK = smart_cast<K*>(O);
		R_ASSERT(pK);
		TypesMapIt it = m_TypesMap.find(O->cNameSect());
		R_ASSERT(it!=m_TypesMap.end());
		m_ItemInfos[pK].snd_time = 0.0f;
		m_ItemInfos[pK].curr_ref = &(it->second);
	}

	virtual void feel_touch_delete(CObject* O)
	{
		K* pK = smart_cast<K*>(O);
		R_ASSERT(pK);
		m_ItemInfos.erase(pK);
	}

public:
	void destroy()
	{
		TypesMapIt it = m_TypesMap.begin();
		for (; it != m_TypesMap.end(); ++it)
			HUD_SOUND_ITEM::DestroySound(it->second.detect_snds);
	}

	void clear()
	{
		m_ItemInfos.clear();
		Feel::Touch::feel_touch.clear();
	}

	virtual void load(LPCSTR sect, LPCSTR prefix)
	{
		u32 i = 1;
		string256 temp;
		do
		{
			xr_sprintf(temp, "%s_class_%d", prefix, i);
			if (pSettings->line_exist(sect, temp))
			{
				shared_str item_sect = pSettings->r_string(sect, temp);

				m_TypesMap.insert(std::make_pair(item_sect, ITEM_TYPE()));
				ITEM_TYPE& item_type = m_TypesMap[item_sect];

				xr_sprintf(temp, "%s_freq_%d", prefix, i);
				item_type.freq = pSettings->r_fvector2(sect, temp);

				xr_sprintf(temp, "%s_sound_%d_", prefix, i);
				HUD_SOUND_ITEM::LoadSound(sect, temp, item_type.detect_snds, SOUND_TYPE_ITEM);

				++i;
			}
			else
				break;
		}
		while (true);
	}
};

class CAfList : public CDetectList<CArtefact>
{
protected:
	virtual bool feel_touch_contact(CObject* O);
public:
	CAfList(): m_af_rank(0)
	{
	}

	int m_af_rank;
};

class CCustomDetector : public CCustomDevice
{
	typedef CCustomDevice inherited;
public:
	CCustomDetector();
	virtual ~CCustomDetector();

	virtual void Load(LPCSTR section);

	virtual void OnH_B_Independent(bool just_before_destroy);

	virtual void shedule_Update(u32 dt);

	float m_fAfDetectRadius;

	virtual u32 ef_detector_type() const { return 1; };
protected:
	virtual void UpdateWork();
	virtual void UpdateAf(){};
	virtual void ResetUI(){};

	float m_fAfVisRadius;
	CAfList m_artefacts;
};

class CZoneList : public CDetectList<CCustomZone>
{
protected:
	virtual bool feel_touch_contact(CObject* O);
public:
	CZoneList();
	virtual ~CZoneList();
}; // class CZoneList

#include "stdafx.h"
#include "../xrEngine/xr_ioconsole.h"
#include "customzone.h"
#include "hit.h"
#include "PHDestroyable.h"
#include "actor.h"
#include "ParticlesObject.h"
#include "xrserver_objects_alife_monsters.h"
#include "../xrEngine/LightAnimLibrary.h"
#include "level.h"
#include "game_cl_base.h"
#include "../xrEngine/igame_persistent.h"
#include "../xrengine/xr_collide_form.h"
#include "artefact.h"
#include "ai_object_location.h"
#include "../Include/xrRender/Kinematics.h"
#include "zone_effector.h"
#include "breakableobject.h"
#include "GamePersistent.h"
#include "../../script_game_object.h"

#define WIND_RADIUS (4*Radius())	//расстояние до актера, когда появляется ветер 
#define FASTMODE_DISTANCE (100.f)	//distance to camera from sphere, when zone switches to fast update sequence

extern Fvector4 ps_ssfx_int_grass_params_1;

CCustomZone::CCustomZone(void)
{
	m_zone_flags.zero();

	m_fMaxPower = 100.f;
	m_fAttenuation = 1.f;
	m_fEffectiveRadius = 1.0f;
	m_zone_flags.set(eZoneIsActive, FALSE);
	m_eHitTypeBlowout = ALife::eHitTypeWound;
	m_pIdleParticles = NULL;
	m_pLight = NULL;
	m_pIdleLight = NULL;
	m_pIdleLAnim = NULL;
	m_pBlowLAnim = NULL;


	m_StateTime.resize(eZoneStateMax);
	for (int i = 0; i < eZoneStateMax; i++)
		m_StateTime[i] = 0;


	m_dwAffectFrameNum = 0;
	m_fBlowoutWindPowerMax = m_fStoreWindPower = 0.f;
	m_fDistanceToCurEntity = flt_max;
	m_ef_weapon_type = u32(-1);
	m_owner_id = u32(-1);

	m_actor_effector = NULL;
	m_zone_flags.set(eIdleObjectParticlesDontStop, FALSE);
	m_zone_flags.set(eBlowoutWindActive, FALSE);
	m_zone_flags.set(eFastMode, TRUE);

	m_eZoneState = eZoneStateIdle;
}

CCustomZone::~CCustomZone(void)
{
	g_pGamePersistent->GrassBendersRemoveByIndex(grassbender_id);
	m_idle_sound.destroy();
	m_accum_sound.destroy();
	m_awaking_sound.destroy();
	m_blowout_sound.destroy();
	m_hit_sound.destroy();
	m_entrance_sound.destroy();
	xr_delete(m_actor_effector);
}

void CCustomZone::Load(LPCSTR section)
{
	inherited::Load(section);

	m_iDisableHitTime = pSettings->r_s32(section, "disable_time");
	m_iDisableHitTimeSmall = pSettings->r_s32(section, "disable_time_small");
	m_iDisableIdleTime = pSettings->r_s32(section, "disable_idle_time");
	m_fHitImpulseScale = pSettings->r_float(section, "hit_impulse_scale");
	m_fEffectiveRadius = pSettings->r_float(section, "effective_radius");
	m_eHitTypeBlowout = ALife::g_tfString2HitType(pSettings->r_string(section, "hit_type"));

	m_zone_flags.set(eIgnoreNonAlive, pSettings->r_bool(section, "ignore_nonalive"));
	m_zone_flags.set(eIgnoreSmall, pSettings->r_bool(section, "ignore_small"));
	m_zone_flags.set(eIgnoreArtefact, pSettings->r_bool(section, "ignore_artefacts"));

	//загрузить времена для зоны
	m_StateTime[eZoneStateIdle] = -1;
	m_StateTime[eZoneStateAwaking] = pSettings->r_s32(section, "awaking_time");
	m_StateTime[eZoneStateBlowout] = pSettings->r_s32(section, "blowout_time");
	m_StateTime[eZoneStateAccumulate] = pSettings->r_s32(section, "accamulate_time");

	//////////////////////////////////////////////////////////////////////////
	ISpatial* self = smart_cast<ISpatial*>(this);
	if (self) self->spatial.type |= (STYPE_COLLIDEABLE | STYPE_SHAPE);
	//////////////////////////////////////////////////////////////////////////

	LPCSTR sound_str = NULL;

	// -- Interactive Grass - IDLE
	if (pSettings->line_exist(section, "bend_grass_idle_anim"))
		m_BendGrass_idle_anim = pSettings->r_s8(section, "bend_grass_idle_anim");
	else
		m_BendGrass_idle_anim = -1;

	if (pSettings->line_exist(section, "bend_grass_idle_str"))
		m_BendGrass_idle_str = pSettings->r_float(section, "bend_grass_idle_str");
	else
		m_BendGrass_idle_str = 1.0f;

	if (pSettings->line_exist(section, "bend_grass_idle_radius"))
		m_BendGrass_idle_radius = pSettings->r_float(section, "bend_grass_idle_radius");
	else
		m_BendGrass_idle_radius = 1.0f;

	if (pSettings->line_exist(section, "bend_grass_idle_speed"))
		m_BendGrass_idle_speed = pSettings->r_float(section, "bend_grass_idle_speed");
	else
		m_BendGrass_idle_speed = 1.0f;

	// -- Interactive Grass - ACTIVE
	if (pSettings->line_exist(section, "bend_grass_whenactive_anim"))
		m_BendGrass_whenactive_anim = pSettings->r_s8(section, "bend_grass_whenactive_anim");
	else
		m_BendGrass_whenactive_anim = -1;

	if (pSettings->line_exist(section, "bend_grass_whenactive_speed"))
		m_BendGrass_whenactive_speed = pSettings->r_float(section, "bend_grass_whenactive_speed");
	else
		m_BendGrass_whenactive_speed = -1;

	if (pSettings->line_exist(section, "bend_grass_whenactive_str"))
		m_BendGrass_whenactive_str = pSettings->r_float(section, "bend_grass_whenactive_str");
	else
		m_BendGrass_whenactive_str = -1;

	// -- Interactive Grass - BLOWOUT
	if (pSettings->line_exist(section, "bend_grass_blowout_duration"))
		m_BendGrass_Blowout_time = pSettings->r_u32(section, "bend_grass_blowout_duration");
	else
		m_BendGrass_Blowout_time = -1;

	if (pSettings->line_exist(section, "bend_grass_blowout"))
		m_BendGrass_Blowout = pSettings->r_bool(section, "bend_grass_blowout");

	if (pSettings->line_exist(section, "bend_grass_blowout_speed"))
		m_BendGrass_Blowout_speed = pSettings->r_float(section, "bend_grass_blowout_speed");

	if (pSettings->line_exist(section, "bend_grass_blowout_radius"))
		m_BendGrass_Blowout_radius = pSettings->r_float(section, "bend_grass_blowout_radius");
	// --

	if (pSettings->line_exist(section, "idle_sound"))
	{
		sound_str = pSettings->r_string(section, "idle_sound");
		m_idle_sound.create(sound_str, st_Effect, sg_SourceType);
	}

	if (pSettings->line_exist(section, "accum_sound"))
	{
		sound_str = pSettings->r_string(section, "accum_sound");
		m_accum_sound.create(sound_str, st_Effect, sg_SourceType);
	}
	if (pSettings->line_exist(section, "awake_sound"))
	{
		sound_str = pSettings->r_string(section, "awake_sound");
		m_awaking_sound.create(sound_str, st_Effect, sg_SourceType);
	}

	if (pSettings->line_exist(section, "blowout_sound"))
	{
		sound_str = pSettings->r_string(section, "blowout_sound");
		m_blowout_sound.create(sound_str, st_Effect, sg_SourceType);
	}


	if (pSettings->line_exist(section, "hit_sound"))
	{
		sound_str = pSettings->r_string(section, "hit_sound");
		m_hit_sound.create(sound_str, st_Effect, sg_SourceType);
	}

	if (pSettings->line_exist(section, "entrance_sound"))
	{
		sound_str = pSettings->r_string(section, "entrance_sound");
		m_entrance_sound.create(sound_str, st_Effect, sg_SourceType);
	}


	if (pSettings->line_exist(section, "idle_particles"))
		m_sIdleParticles = pSettings->r_string(section, "idle_particles");
	if (pSettings->line_exist(section, "blowout_particles"))
		m_sBlowoutParticles = pSettings->r_string(section, "blowout_particles");

	m_bBlowoutOnce = FALSE;
	if (pSettings->line_exist(section, "blowout_once"))
		m_bBlowoutOnce = pSettings->r_bool(section, "blowout_once");

	if (pSettings->line_exist(section, "accum_particles"))
		m_sAccumParticles = pSettings->r_string(section, "accum_particles");

	if (pSettings->line_exist(section, "awake_particles"))
		m_sAwakingParticles = pSettings->r_string(section, "awake_particles");


	if (pSettings->line_exist(section, "entrance_small_particles"))
		m_sEntranceParticlesSmall = pSettings->r_string(section, "entrance_small_particles");
	if (pSettings->line_exist(section, "entrance_big_particles"))
		m_sEntranceParticlesBig = pSettings->r_string(section, "entrance_big_particles");

	if (pSettings->line_exist(section, "hit_small_particles"))
		m_sHitParticlesSmall = pSettings->r_string(section, "hit_small_particles");
	if (pSettings->line_exist(section, "hit_big_particles"))
		m_sHitParticlesBig = pSettings->r_string(section, "hit_big_particles");

	if (pSettings->line_exist(section, "idle_small_particles"))
		m_sIdleObjectParticlesBig = pSettings->r_string(section, "idle_big_particles");

	if (pSettings->line_exist(section, "idle_big_particles"))
		m_sIdleObjectParticlesSmall = pSettings->r_string(section, "idle_small_particles");

	if (pSettings->line_exist(section, "idle_particles_dont_stop"))
		m_zone_flags.set(eIdleObjectParticlesDontStop, pSettings->r_bool(section, "idle_particles_dont_stop"));

	if (pSettings->line_exist(section, "postprocess"))
	{
		m_actor_effector = xr_new<CZoneEffector>();
		m_actor_effector->Load(pSettings->r_string(section, "postprocess"));
	};


	if (pSettings->line_exist(section, "bolt_entrance_particles"))
	{
		m_sBoltEntranceParticles = pSettings->r_string(section, "bolt_entrance_particles");
		m_zone_flags.set(eBoltEntranceParticles, (m_sBoltEntranceParticles.size() != 0));
	}

	if (pSettings->line_exist(section, "blowout_particles_time"))
	{
		m_dwBlowoutParticlesTime = pSettings->r_u32(section, "blowout_particles_time");
		if (s32(m_dwBlowoutParticlesTime) > m_StateTime[eZoneStateBlowout])
		{
			m_dwBlowoutParticlesTime = m_StateTime[eZoneStateBlowout];
#ifndef MASTER_GOLD
			Msg("! ERROR: invalid 'blowout_particles_time' in '%s'",section);
#endif // #ifndef MASTER_GOLD
		}
	}
	else
		m_dwBlowoutParticlesTime = 0;

	if (pSettings->line_exist(section, "blowout_light_time"))
	{
		m_dwBlowoutLightTime = pSettings->r_u32(section, "blowout_light_time");
		if (s32(m_dwBlowoutLightTime) > m_StateTime[eZoneStateBlowout])
		{
			m_dwBlowoutLightTime = m_StateTime[eZoneStateBlowout];
#ifndef MASTER_GOLD
			Msg("! ERROR: invalid 'blowout_light_time' in '%s'",section);
#endif // #ifndef MASTER_GOLD
		}
	}
	else
		m_dwBlowoutLightTime = 0;

	if (pSettings->line_exist(section, "blowout_sound_time"))
	{
		m_dwBlowoutSoundTime = pSettings->r_u32(section, "blowout_sound_time");
		if (s32(m_dwBlowoutSoundTime) > m_StateTime[eZoneStateBlowout])
		{
			m_dwBlowoutSoundTime = m_StateTime[eZoneStateBlowout];
#ifndef MASTER_GOLD
			Msg("! ERROR: invalid 'blowout_sound_time' in '%s'",section);
#endif // #ifndef MASTER_GOLD
		}
	}
	else
		m_dwBlowoutSoundTime = 0;

	if (pSettings->line_exist(section, "blowout_explosion_time"))
	{
		m_dwBlowoutExplosionTime = pSettings->r_u32(section, "blowout_explosion_time");
		if (s32(m_dwBlowoutExplosionTime) > m_StateTime[eZoneStateBlowout])
		{
			m_dwBlowoutExplosionTime = m_StateTime[eZoneStateBlowout];
#ifndef MASTER_GOLD
			Msg("! ERROR: invalid 'blowout_explosion_time' in '%s'",section);
#endif // #ifndef MASTER_GOLD
		}
	}
	else
		m_dwBlowoutExplosionTime = 0;

	m_zone_flags.set(eBlowoutWind, pSettings->r_bool(section, "blowout_wind"));
	if (m_zone_flags.test(eBlowoutWind))
	{
		m_dwBlowoutWindTimeStart = pSettings->r_u32(section, "blowout_wind_time_start");
		m_dwBlowoutWindTimePeak = pSettings->r_u32(section, "blowout_wind_time_peak");
		m_dwBlowoutWindTimeEnd = pSettings->r_u32(section, "blowout_wind_time_end");
		R_ASSERT(m_dwBlowoutWindTimeStart < m_dwBlowoutWindTimePeak);
		R_ASSERT(m_dwBlowoutWindTimePeak < m_dwBlowoutWindTimeEnd);

		if ((s32)m_dwBlowoutWindTimeEnd < m_StateTime[eZoneStateBlowout])
		{
			m_dwBlowoutWindTimeEnd = u32(m_StateTime[eZoneStateBlowout] - 1);
#ifndef MASTER_GOLD
			Msg("! ERROR: invalid 'blowout_wind_time_end' in '%s'",section);
#endif // #ifndef MASTER_GOLD
		}


		m_fBlowoutWindPowerMax = pSettings->r_float(section, "blowout_wind_power");
	}

	//загрузить флаг отмены idle анимации при blowout
	if (pSettings->line_exist(section, "blowout_disable_idle"))
	{
		m_zone_flags.set(eBlowoutDisableIdle, pSettings->r_bool(section, "blowout_disable_idle"));
	}

	//загрузить параметры световой вспышки от взрыва
	m_zone_flags.set(eBlowoutLight, pSettings->r_bool(section, "blowout_light"));
	if (m_zone_flags.test(eBlowoutLight))
	{
		sscanf(pSettings->r_string(section, "light_color"), "%f,%f,%f", &m_LightColor.r, &m_LightColor.g,
		       &m_LightColor.b);
		m_fLightRange = pSettings->r_float(section, "light_range");
		m_fLightTime = pSettings->r_float(section, "light_time");
		m_fLightTimeLeft = 0;
		LPCSTR light_anim = (READ_IF_EXISTS(pSettings, r_string, section, "blowout_light_anim", (0)));
		if (light_anim)
			m_pBlowLAnim = LALib.FindItem(light_anim);
		m_fLightHeight = pSettings->r_float(section, "light_height");
	}

	//загрузить параметры idle подсветки
	m_zone_flags.set(eIdleLight, pSettings->r_bool(section, "idle_light"));
	if (m_zone_flags.test(eIdleLight))
	{
		m_fIdleLightRange = pSettings->r_float(section, "idle_light_range");
		LPCSTR light_anim = pSettings->r_string(section, "idle_light_anim");
		m_pIdleLAnim = LALib.FindItem(light_anim);
		m_fIdleLightHeight = pSettings->r_float(section, "idle_light_height");

		m_zone_flags.set(eIdleLightVolumetric, pSettings->r_bool(section, "idle_light_volumetric"));
		volumetric_distance = READ_IF_EXISTS(pSettings, r_float, section, "idle_light_volumetric_distance", .5f);
		volumetric_intensity = READ_IF_EXISTS(pSettings, r_float, section, "idle_light_volumetric_intensity", .5f);
		volumetric_quality = READ_IF_EXISTS(pSettings, r_float, section, "idle_light_volumetric_quality", .5f);

		m_zone_flags.set(eIdleLightShadow, pSettings->r_bool(section, "idle_light_shadow"));
		m_zone_flags.set(eIdleLightR1, pSettings->r_bool(section, "idle_light_r1"));
	}

	bool use = !!READ_IF_EXISTS(pSettings, r_bool, section, "use_secondary_hit", false);
	m_zone_flags.set(eUseSecondaryHit, use);
	if (use)
		m_fSecondaryHitPower = pSettings->r_float(section, "secondary_hit_power");

	m_ef_anomaly_type = pSettings->r_u32(section, "ef_anomaly_type");
	m_ef_weapon_type = pSettings->r_u32(section, "ef_weapon_type");

	m_zone_flags.set(eAffectPickDOF, pSettings->r_bool(section, "pick_dof_effector"));
}

BOOL CCustomZone::net_Spawn(CSE_Abstract* DC)
{
	if (!inherited::net_Spawn(DC))
		return (FALSE);

	CSE_Abstract* e = (CSE_Abstract*)(DC);
	CSE_ALifeCustomZone* Z = smart_cast<CSE_ALifeCustomZone*>(e);
	VERIFY(Z);

	m_fMaxPower = pSettings->r_float(cNameSect(), "max_start_power");
	m_fAttenuation = pSettings->r_float(cNameSect(), "attenuation");
	m_owner_id = Z->m_owner_id;
	if (m_owner_id != u32(-1))
		m_ttl = Device.dwTimeGlobal + 40000; // 40 sec
	else
		m_ttl = u32(-1);

	m_TimeToDisable = Z->m_disabled_time * 1000;
	m_TimeToEnable = Z->m_enabled_time * 1000;
	m_TimeShift = Z->m_start_time_shift * 1000;
	m_StartTime = Device.dwTimeGlobal;
	m_zone_flags.set(eUseOnOffTime, (m_TimeToDisable != 0) && (m_TimeToEnable != 0));

	//добавить источники света
	bool br1 = (0 == psDeviceFlags.test(rsR2 | rsR3 | rsR4)); //Alundaio: rsR4 flag


	bool render_ver_allowed = !br1 || (br1 && m_zone_flags.test(eIdleLightR1));

	if (m_zone_flags.test(eIdleLight) && render_ver_allowed)
	{
		m_pIdleLight = ::Render->light_create();
		m_pIdleLight->set_shadow(!!m_zone_flags.test(eIdleLightShadow));

		if (m_zone_flags.test(eIdleLightVolumetric))
		{
			m_pIdleLight->set_type(IRender_Light::POINT);
			m_pIdleLight->set_volumetric(true);
			m_pIdleLight->set_volumetric_distance(volumetric_distance);
			m_pIdleLight->set_volumetric_intensity(volumetric_intensity);
			m_pIdleLight->set_volumetric_quality(volumetric_quality);
		}
	}
	else
		m_pIdleLight = NULL;

	if (m_zone_flags.test(eBlowoutLight))
	{
		m_pLight = ::Render->light_create();
		m_pLight->set_shadow(true);
	}
	else
		m_pLight = NULL;

	setEnabled(TRUE);

	PlayIdleParticles();

	m_iPreviousStateTime = m_iStateTime = 0;

	m_dwLastTimeMoved = Device.dwTimeGlobal;
	m_vPrevPos.set(Position());


	if (spawn_ini() && spawn_ini()->line_exist("fast_mode", "always_fast"))
	{
		m_zone_flags.set(eAlwaysFastmode, spawn_ini()->r_bool("fast_mode", "always_fast"));
	}
	return (TRUE);
}

void CCustomZone::net_Destroy()
{
	StopIdleParticles();

	inherited::net_Destroy();

	StopWind();

	m_pLight.destroy();
	m_pIdleLight.destroy();

	CParticlesObject::Destroy(m_pIdleParticles);

	if (m_actor_effector)
		m_actor_effector->Stop();

	OBJECT_INFO_VEC_IT i = m_ObjectInfoMap.begin();
	OBJECT_INFO_VEC_IT e = m_ObjectInfoMap.end();
	for (; e != i; ++i)
		exit_Zone(*i);
	m_ObjectInfoMap.clear();
}

void CCustomZone::net_Import(NET_Packet& P)
{
	inherited::net_Import(P);
}

void CCustomZone::net_Export(NET_Packet& P)
{
	inherited::net_Export(P);
}

bool CCustomZone::IdleState()
{
	UpdateOnOffState();

	return false;
}

bool CCustomZone::AwakingState()
{
	if (m_iStateTime >= m_StateTime[eZoneStateAwaking])
	{
		SwitchZoneState(eZoneStateBlowout);
		return true;
	}
	return false;
}

bool CCustomZone::BlowoutState()
{
	if (m_iStateTime >= m_StateTime[eZoneStateBlowout])
	{

		if (m_zone_flags.test(eBlowoutDisableIdle))
			PlayIdleParticles();

		SwitchZoneState(eZoneStateAccumulate);
		if (m_bBlowoutOnce)
		{
			ZoneDisable();
		}

		return true;
	}

	if (m_zone_flags.test(eBlowoutDisableIdle))
		StopIdleParticles();

	return false;
}

bool CCustomZone::AccumulateState()
{
	if (m_iStateTime >= m_StateTime[eZoneStateAccumulate])
	{
		if (m_zone_flags.test(eZoneIsActive))
			SwitchZoneState(eZoneStateBlowout);
		else
			SwitchZoneState(eZoneStateIdle);

		return true;
	}
	return false;
}

void CCustomZone::UpdateWorkload(u32 dt)
{
	m_iPreviousStateTime = m_iStateTime;
	m_iStateTime += (int)dt;

	if (!IsEnabled())
	{
		if (m_actor_effector)
			m_actor_effector->Stop();
		return;
	};

	UpdateIdleLight();

	switch (m_eZoneState)
	{
	case eZoneStateIdle:
		IdleState();
		break;
	case eZoneStateAwaking:
		AwakingState();
		break;
	case eZoneStateBlowout:
		BlowoutState();
		break;
	case eZoneStateAccumulate:
		AccumulateState();
		break;
	case eZoneStateDisabled:
		break;
	default: NODEFAULT;
	}

	if (Level().CurrentEntity())
	{
		Fvector P = Level().CurrentControlEntity()->Position();
		P.y -= 0.9f;
		float radius = 1.0f;
		CalcDistanceTo(P, m_fDistanceToCurEntity, radius);

		if (m_actor_effector)
		{
			m_actor_effector->Update(m_fDistanceToCurEntity, radius, m_eHitTypeBlowout);
		}
	}

	if (m_pLight && m_pLight->get_active())
		UpdateBlowoutLight();

	if (m_zone_flags.test(eUseSecondaryHit) && m_eZoneState != eZoneStateIdle && m_eZoneState != eZoneStateDisabled)
	{
		UpdateSecondaryHit();
	}
}

// called only in "fast-mode"
void CCustomZone::UpdateCL()
{
	inherited::UpdateCL();
	if (m_zone_flags.test(eFastMode))
		UpdateWorkload(Device.dwTimeDelta);
}

// called as usual
void CCustomZone::shedule_Update(u32 dt)
{
	m_zone_flags.set(eZoneIsActive, FALSE);

	if (IsEnabled())
	{
		const Fsphere& s = CFORM()->getSphere();
		Fvector P;
		XFORM().transform_tiny(P, s.P);

		// update
		feel_touch_update(P, s.R);

		//пройтись по всем объектам в зоне
		//и проверить их состояние
		for (OBJECT_INFO_VEC_IT it = m_ObjectInfoMap.begin();
		     m_ObjectInfoMap.end() != it; ++it)
		{
			CGameObject* pObject = (*it).object;
			if (!pObject) continue;
			CEntityAlive* pEntityAlive = smart_cast<CEntityAlive*>(pObject);
			SZoneObjectInfo& info = (*it);

			info.dw_time_in_zone += dt;

			if ((!info.small_object && m_iDisableHitTime != -1 && (int)info.dw_time_in_zone > m_iDisableHitTime) ||
				(info.small_object && m_iDisableHitTimeSmall != -1 && (int)info.dw_time_in_zone > m_iDisableHitTimeSmall
				))
			{
				if (!pEntityAlive || !pEntityAlive->g_Alive())
					info.zone_ignore = true;
			}
			if (m_iDisableIdleTime != -1 && (int)info.dw_time_in_zone > m_iDisableIdleTime)
			{
				if (!pEntityAlive || !pEntityAlive->g_Alive())
					StopObjectIdleParticles(pObject);
			}

			// Ignore object override script callback
			if (pEntityAlive)
			{
				::luabind::functor<bool> funct;
				if (ai().script_engine().functor("_G.CCustomZone_BeforeActivateCallback", funct))
					info.zone_ignore = !funct(this->lua_game_object(), pObject->lua_game_object());
			}

			//если есть хотя бы один не дисабленый объект, то
			//зона считается активной
			if (info.zone_ignore == false)
				m_zone_flags.set(eZoneIsActive,TRUE);
		}

		if (eZoneStateIdle == m_eZoneState)
			CheckForAwaking();

		inherited::shedule_Update(dt);

		// check "fast-mode" border
		float act_distance = Level().CurrentControlEntity()->Position().distance_to(P) - s.R;
		if (act_distance > FASTMODE_DISTANCE && !m_zone_flags.test(eAlwaysFastmode))
			o_switch_2_slow();
		else
			o_switch_2_fast();

		if (!m_zone_flags.test(eFastMode))
			UpdateWorkload(dt);

		if (act_distance < ps_ssfx_int_grass_params_1.w)
			GrassZoneUpdate();
		else
		{
			// Out of range, fadeOut if a grassbender_id is assigned
			if (grassbender_id)
			{
				IGame_Persistent::grass_data& GData = g_pGamePersistent->grass_shader_data;

				// If the ID doesn't match... Just remove the grassbender_id.
				if (GData.id[grassbender_id] == ID())
				{
					GData.str_target[grassbender_id] += g_pGamePersistent->GrassBenderToValue(GData.str_target[grassbender_id], 0.0f, 4.0f, false);

					// Remove ( Don't worry, GrassBenderToValue() it's going to get the == 0 )
					if (GData.str_target[grassbender_id] == 0)
						g_pGamePersistent->GrassBendersRemoveByIndex(grassbender_id);
				}
				else
				{
					grassbender_id = NULL;
				}
			}
		}
	};

	UpdateOnOffState();

	if (!IsGameTypeSingle() && Local())
	{
		if (Device.dwTimeGlobal > m_ttl)
			DestroyObject();
	}
}

void CCustomZone::CheckForAwaking()
{
	if (m_zone_flags.test(eZoneIsActive) && eZoneStateIdle == m_eZoneState)
		SwitchZoneState(eZoneStateAwaking);
}

void CCustomZone::feel_touch_new(CObject* O)
{
	//	if(smart_cast<CActor*>(O) && O == Level().CurrentEntity())
	//					m_pLocalActor	= smart_cast<CActor*>(O);

	CGameObject* pGameObject = smart_cast<CGameObject*>(O);
	CEntityAlive* pEntityAlive = smart_cast<CEntityAlive*>(pGameObject);
	CArtefact* pArtefact = smart_cast<CArtefact*>(pGameObject);

	SZoneObjectInfo object_info;
	object_info.object = pGameObject;

	if (pEntityAlive && pEntityAlive->g_Alive())
		object_info.nonalive_object = false;
	else
		object_info.nonalive_object = true;

	if (pGameObject->Radius() < SMALL_OBJECT_RADIUS)
		object_info.small_object = true;
	else
		object_info.small_object = false;

	if ((object_info.small_object && m_zone_flags.test(eIgnoreSmall)) ||
		(object_info.nonalive_object && m_zone_flags.test(eIgnoreNonAlive)) ||
		(pArtefact && m_zone_flags.test(eIgnoreArtefact)))
		object_info.zone_ignore = true;
	else
		object_info.zone_ignore = false;
	enter_Zone(object_info);
	m_ObjectInfoMap.push_back(object_info);

	if (IsEnabled())
	{
		PlayEntranceParticles(pGameObject);
		PlayObjectIdleParticles(pGameObject);
	}
};

void CCustomZone::feel_touch_delete(CObject* O)
{
	CGameObject* pGameObject = smart_cast<CGameObject*>(O);
	if (!pGameObject->getDestroy())
	{
		StopObjectIdleParticles(pGameObject);
	}

	OBJECT_INFO_VEC_IT it = std::find(m_ObjectInfoMap.begin(), m_ObjectInfoMap.end(), pGameObject);
	if (it != m_ObjectInfoMap.end())
	{
		exit_Zone(*it);
		m_ObjectInfoMap.erase(it);
	}
}

bool CCustomZone::feel_touch_contact(CObject* O)
{
	if (smart_cast<CCustomZone*>(O)) return FALSE;
	if (smart_cast<CBreakableObject*>(O)) return FALSE;
	if (0 == smart_cast<IKinematics*>(O->Visual())) return FALSE;

	if (O->ID() == ID())
		return (FALSE);

	CGameObject* object = smart_cast<CGameObject*>(O);
	if (!object || !object->IsVisibleForZones())
		return (FALSE);

	if (!((CCF_Shape*)CFORM())->Contact(O))
		return (FALSE);

	return (object->feel_touch_on_contact(this));
}


float CCustomZone::RelativePower(float dist, float nearest_shape_radius)
{
	float radius = effective_radius(nearest_shape_radius);
	float power = (radius < dist) ? 0 : (1.f - m_fAttenuation * (dist / radius) * (dist / radius));
	return (power < 0.0f) ? 0.0f : power;
}

float CCustomZone::effective_radius(float nearest_shape_radius)
{
	return /*Radius()*/nearest_shape_radius * m_fEffectiveRadius;
}

float CCustomZone::Power(float dist, float nearest_shape_radius)
{
	return m_fMaxPower * RelativePower(dist, nearest_shape_radius);
}

void CCustomZone::ChangeIdleParticles(LPCSTR name, bool bIdleLight)
{
	StopIdleParticles(true);
	m_sIdleParticles = name;
	PlayIdleParticles(bIdleLight);
}

void CCustomZone::PlayIdleParticles(bool bIdleLight)
{
	if (!m_zone_flags.test(eFastMode)) return;

	if (!m_idle_sound._feedback())
		m_idle_sound.play_at_pos(0, Position(), true);

	if (*m_sIdleParticles)
	{
		if (!m_pIdleParticles)
		{
			m_pIdleParticles = CParticlesObject::Create(m_sIdleParticles.c_str(),FALSE);
			m_pIdleParticles->UpdateParent(XFORM(), zero_vel);

			m_pIdleParticles->UpdateParent(XFORM(), zero_vel);
			m_pIdleParticles->Play(false);
		}
	}
	if (bIdleLight)
		StartIdleLight();
}

void CCustomZone::StopIdleParticles(bool bIdleLight)
{
	m_idle_sound.stop();

	if (m_pIdleParticles)
	{
		m_pIdleParticles->Stop(FALSE);
		CParticlesObject::Destroy(m_pIdleParticles);
	}

	if (bIdleLight)
		StopIdleLight();
}


void CCustomZone::StartIdleLight()
{
	if (!m_zone_flags.test(eFastMode)) return;
	if (m_pIdleLight)
	{
		m_pIdleLight->set_range(m_fIdleLightRange);
		Fvector pos = Position();
		pos.y += m_fIdleLightHeight;
		m_pIdleLight->set_position(pos);
		m_pIdleLight->set_active(true);
	}
}

void CCustomZone::StopIdleLight()
{
	if (m_pIdleLight)
		m_pIdleLight->set_active(false);
}

void CCustomZone::UpdateIdleLight()
{
	if (!m_zone_flags.test(eFastMode)) return;
	if (!m_pIdleLight || !m_pIdleLight->get_active())
		return;


	VERIFY(m_pIdleLAnim);

	int frame = 0;
	u32 clr = m_pIdleLAnim->CalculateBGR(Device.fTimeGlobal, frame); // возвращает в формате BGR
	Fcolor fclr;
	fclr.set((float)color_get_B(clr) / 255.f, (float)color_get_G(clr) / 255.f, (float)color_get_R(clr) / 255.f, 1.f);

	float range = m_fIdleLightRange + 0.25f * ::Random.randF(-1.f, 1.f);
	m_pIdleLight->set_range(range);
	m_pIdleLight->set_color(fclr);

	Fvector pos = Position();
	pos.y += m_fIdleLightHeight;
	m_pIdleLight->set_position(pos);
}


void CCustomZone::PlayBlowoutParticles()
{
	if (!m_sBlowoutParticles) return;
	if (!m_zone_flags.test(eFastMode)) return;

	CParticlesObject* pParticles;
	pParticles = CParticlesObject::Create(*m_sBlowoutParticles,TRUE);
	pParticles->UpdateParent(XFORM(), zero_vel);
	pParticles->Play(false);

	m_fBlowoutTimeLeft = (float)Device.dwTimeGlobal + m_BendGrass_Blowout_time;
}

void CCustomZone::PlayHitParticles(CGameObject* pObject)
{
	if (!m_zone_flags.test(eFastMode)) return;
	m_hit_sound.play_at_pos(0, pObject->Position());

	shared_str particle_str = NULL;

	if (pObject->Radius() < SMALL_OBJECT_RADIUS)
	{
		if (!m_sHitParticlesSmall) return;
		particle_str = m_sHitParticlesSmall;
	}
	else
	{
		if (!m_sHitParticlesBig) return;
		particle_str = m_sHitParticlesBig;
	}

	if (particle_str.size())
	{
		CParticlesPlayer* PP = smart_cast<CParticlesPlayer*>(pObject);
		if (PP)
		{
			u16 play_bone = PP->GetRandomBone();
			if (play_bone != BI_NONE)
				PP->StartParticles(particle_str, play_bone, Fvector().set(0, 1, 0), ID());
		}
	}
}

#include "bolt.h"

void CCustomZone::PlayEntranceParticles(CGameObject* pObject)
{
	if (!m_zone_flags.test(eFastMode)) return;
	m_entrance_sound.play_at_pos(0, pObject->Position());

	LPCSTR particle_str = NULL;

	if (pObject->Radius() < SMALL_OBJECT_RADIUS)
	{
		if (!m_sEntranceParticlesSmall)
			return;

		particle_str = m_sEntranceParticlesSmall.c_str();
	}
	else
	{
		if (!m_sEntranceParticlesBig)
			return;

		particle_str = m_sEntranceParticlesBig.c_str();
	}

	Fvector vel;
	CPhysicsShellHolder* shell_holder = smart_cast<CPhysicsShellHolder*>(pObject);
	if (shell_holder)
		shell_holder->PHGetLinearVell(vel);
	else
		vel.set(0, 0, 0);

	//выбрать случайную косточку на объекте
	CParticlesPlayer* PP = smart_cast<CParticlesPlayer*>(pObject);
	if (PP)
	{
		u16 play_bone = PP->GetRandomBone();

		if (play_bone != BI_NONE)
		{
			CParticlesObject* pParticles = CParticlesObject::Create(particle_str, TRUE);
			Fmatrix xform;
			Fvector dir;
			if (fis_zero(vel.magnitude()))
				dir.set(0, 1, 0);
			else
			{
				dir.set(vel);
				dir.normalize();
			}

			PP->MakeXFORM(pObject, play_bone, dir, Fvector().set(0, 0, 0), xform);
			pParticles->UpdateParent(xform, vel);
			pParticles->Play(false);
		}
	}
	if (m_zone_flags.test(eBoltEntranceParticles) && smart_cast<CBolt*>(pObject))
		PlayBoltEntranceParticles();
}

void CCustomZone::PlayBoltEntranceParticles()
{
	if (!m_zone_flags.test(eFastMode)) return;
	CCF_Shape* Sh = (CCF_Shape*)CFORM();
	const Fmatrix& XF = XFORM();
	Fmatrix PXF;
	xr_vector<CCF_Shape::shape_def>& Shapes = Sh->Shapes();
	Fvector sP0, sP1, vel;

	CParticlesObject* pParticles = NULL;

	xr_vector<CCF_Shape::shape_def>::iterator it = Shapes.begin();
	xr_vector<CCF_Shape::shape_def>::iterator it_e = Shapes.end();

	for (; it != it_e; ++it)
	{
		CCF_Shape::shape_def& s = *it;
		switch (s.type)
		{
		case 0: // sphere
			{
				sP0 = s.data.sphere.P;
				XF.transform_tiny(sP0);


				float ki = 10.0f * s.data.sphere.R;
				float c = 2.0f * s.data.sphere.R;

				float quant_h = (PI_MUL_2 / float(ki)) * c;
				float quant_p = (PI_DIV_2 / float(ki));

				for (float i = 0; i < ki; ++i)
				{
					vel.setHP(::Random.randF(quant_h / 2.0f, quant_h) * i,
					          ::Random.randF(quant_p / 2.0f, quant_p) * i
					);

					vel.mul(s.data.sphere.R);

					sP1.add(sP0, vel);

					PXF.identity();
					PXF.k.normalize(vel);
					Fvector::generate_orthonormal_basis(PXF.k, PXF.j, PXF.i);

					PXF.c = sP1;

					pParticles = CParticlesObject::Create(m_sBoltEntranceParticles.c_str(), TRUE);
					pParticles->UpdateParent(PXF, vel);
					pParticles->Play(false);
				}
			}
			break;
		case 1: // box
			break;
		}
	}
}

void CCustomZone::PlayBulletParticles(Fvector& pos)
{
	if (!m_zone_flags.test(eFastMode)) return;
	m_entrance_sound.play_at_pos(0, pos);

	if (!m_sEntranceParticlesSmall) return;

	CParticlesObject* pParticles;
	pParticles = CParticlesObject::Create(*m_sEntranceParticlesSmall,TRUE);

	Fmatrix M;
	M = XFORM();
	M.c.set(pos);

	pParticles->UpdateParent(M, zero_vel);
	pParticles->Play(false);
}

void CCustomZone::PlayObjectIdleParticles(CGameObject* pObject)
{
	CParticlesPlayer* PP = smart_cast<CParticlesPlayer*>(pObject);
	if (!PP) return;

	shared_str particle_str = NULL;

	//разные партиклы для объектов разного размера
	if (pObject->Radius() < SMALL_OBJECT_RADIUS)
	{
		if (!m_sIdleObjectParticlesSmall) return;
		particle_str = m_sIdleObjectParticlesSmall;
	}
	else
	{
		if (!m_sIdleObjectParticlesBig) return;
		particle_str = m_sIdleObjectParticlesBig;
	}


	//запустить партиклы на объекте
	//. new
	PP->StopParticles(particle_str, BI_NONE, true);

	PP->StartParticles(particle_str, Fvector().set(0, 1, 0), ID());
	if (!IsEnabled())
		PP->StopParticles(particle_str, BI_NONE, true);
}

void CCustomZone::StopObjectIdleParticles(CGameObject* pObject)
{
	if (m_zone_flags.test(eIdleObjectParticlesDontStop) && !pObject->cast_actor())
		return;

	CParticlesPlayer* PP = smart_cast<CParticlesPlayer*>(pObject);
	if (!PP) return;


	OBJECT_INFO_VEC_IT it = std::find(m_ObjectInfoMap.begin(), m_ObjectInfoMap.end(), pObject);
	if (m_ObjectInfoMap.end() == it) return;


	shared_str particle_str = NULL;
	//разные партиклы для объектов разного размера
	if (pObject->Radius() < SMALL_OBJECT_RADIUS)
	{
		if (!m_sIdleObjectParticlesSmall) return;
		particle_str = m_sIdleObjectParticlesSmall;
	}
	else
	{
		if (!m_sIdleObjectParticlesBig) return;
		particle_str = m_sIdleObjectParticlesBig;
	}

	PP->StopParticles(particle_str, BI_NONE, true);
}

void CCustomZone::Hit(SHit* pHDS)
{
	Fmatrix M;
	M.identity();
	M.translate_over(pHDS->p_in_bone_space);
	M.mulA_43(XFORM());
	PlayBulletParticles(M.c);
}

void CCustomZone::StartBlowoutLight()
{
	if (!m_zone_flags.test(eFastMode)) return;
	if (!m_pLight || m_fLightTime <= 0.f) return;

	m_fLightTimeLeft = (float)Device.dwTimeGlobal + m_fLightTime * 1000.0f;

	m_pLight->set_color(m_LightColor.r, m_LightColor.g, m_LightColor.b);
	m_pLight->set_range(m_fLightRange);

	Fvector pos = Position();
	pos.y += m_fLightHeight;
	m_pLight->set_position(pos);
	m_pLight->set_active(true);
}

void CCustomZone::StopBlowoutLight()
{
	m_fLightTimeLeft = 0.f;
	m_pLight->set_active(false);
}

void CCustomZone::UpdateBlowoutLight()
{
	if (!m_zone_flags.test(eFastMode)) return;
	if (m_fLightTimeLeft > (float)Device.dwTimeGlobal)
	{
		float time_k = m_fLightTimeLeft - (float)Device.dwTimeGlobal;

		//		m_fLightTimeLeft -= Device.fTimeDelta;
		clamp(time_k, 0.0f, m_fLightTime * 1000.0f);

		float scale = time_k / (m_fLightTime * 1000.0f);
		scale = powf(scale + EPS_L, 0.15f);
		float r = m_fLightRange * scale;

		if (m_pBlowLAnim)
		{
			int frame = 0;
			u32 clr = m_pBlowLAnim->CalculateBGR(Device.fTimeGlobal, frame);
			Fcolor fclr;
			fclr.set((float)color_get_B(clr) / 255.f, (float)color_get_G(clr) / 255.f, (float)color_get_R(clr) / 255.f,
			         1.f);
			m_pLight->set_color(fclr);
		}
		else
			m_pLight->set_color(m_LightColor.r * scale, m_LightColor.g * scale, m_LightColor.b * scale);

		VERIFY(_valid(r));
		m_pLight->set_range(r);

		Fvector pos = Position();
		pos.y += m_fLightHeight;
		m_pLight->set_position(pos);
	}
	else
		StopBlowoutLight();
}

void CCustomZone::AffectObjects()
{
	if (m_dwAffectFrameNum == Device.dwFrame)
		return;

	m_dwAffectFrameNum = Device.dwFrame;

	if (Device.dwPrecacheFrame)
		return;


	OBJECT_INFO_VEC_IT it;
	for (it = m_ObjectInfoMap.begin(); m_ObjectInfoMap.end() != it; ++it)
	{
		if (!(*it).object->getDestroy())
			Affect(&(*it));
	}
}

void CCustomZone::UpdateBlowout()
{
	if (!m_zone_flags.test(eFastMode)) return;
	if (m_dwBlowoutParticlesTime >= (u32)m_iPreviousStateTime &&
		m_dwBlowoutParticlesTime < (u32)m_iStateTime)
		PlayBlowoutParticles();

	if (m_dwBlowoutLightTime >= (u32)m_iPreviousStateTime &&
		m_dwBlowoutLightTime < (u32)m_iStateTime)
		StartBlowoutLight();

	if (m_dwBlowoutSoundTime >= (u32)m_iPreviousStateTime &&
		m_dwBlowoutSoundTime < (u32)m_iStateTime)
		m_blowout_sound.play_at_pos(0, Position());

	if (m_zone_flags.test(eBlowoutWind) && m_dwBlowoutWindTimeStart >= (u32)m_iPreviousStateTime &&
		m_dwBlowoutWindTimeStart < (u32)m_iStateTime)
		StartWind();

	UpdateWind();


	if (m_dwBlowoutExplosionTime >= (u32)m_iPreviousStateTime &&
		m_dwBlowoutExplosionTime < (u32)m_iStateTime)
	{
		AffectObjects();
		
		if (m_BendGrass_Blowout)
			g_pGamePersistent->GrassBendersAddExplosion(ID(), Position(), Fvector().set(0, -99, 0), 1.33f, m_BendGrass_Blowout_speed, 1.0f, m_BendGrass_Blowout_radius);
	}
}

void CCustomZone::OnMove()
{
	if (m_dwLastTimeMoved == 0)
	{
		m_dwLastTimeMoved = Device.dwTimeGlobal;
		m_vPrevPos.set(Position());
	}
	else
	{
		float time_delta = float(Device.dwTimeGlobal - m_dwLastTimeMoved) / 1000.f;
		m_dwLastTimeMoved = Device.dwTimeGlobal;

		Fvector vel;

		if (fis_zero(time_delta))
			vel = zero_vel;
		else
		{
			vel.sub(Position(), m_vPrevPos);
			vel.div(time_delta);
		}

		if (m_pIdleParticles)
			m_pIdleParticles->UpdateParent(XFORM(), vel);

		if (m_pLight && m_pLight->get_active())
			m_pLight->set_position(Position());

		if (m_pIdleLight && m_pIdleLight->get_active())
			m_pIdleLight->set_position(Position());

		if (grassbender_id)
		{
			// Check ID, just in case...
			if (g_pGamePersistent->grass_shader_data.id[grassbender_id] == ID())
				g_pGamePersistent->grass_shader_data.pos[grassbender_id] = Position();
		}
	}
}

void CCustomZone::MoveScript(Fvector pos)
{
	XFORM().translate_over(pos);

	OnMove();
}

void CCustomZone::OnEvent(NET_Packet& P, u16 type)
{
	switch (type)
	{
	case GE_ZONE_STATE_CHANGE:
		{
			u8 S;
			P.r_u8(S);
			OnStateSwitch(EZoneState(S));
			break;
		}
	}
	inherited::OnEvent(P, type);
};

void CCustomZone::OnStateSwitch(EZoneState new_state)
{
	if (new_state == eZoneStateDisabled)
		Disable();
	else
		Enable();

	if (new_state == eZoneStateAccumulate)
		PlayAccumParticles();

	if (new_state == eZoneStateAwaking)
		PlayAwakingParticles();

	m_eZoneState = new_state;
	m_iPreviousStateTime = m_iStateTime = 0;
};

void CCustomZone::SwitchZoneState(EZoneState new_state)
{
	if (OnServer())
	{
		// !!! Just single entry for given state !!!
		NET_Packet P;
		u_EventGen(P, GE_ZONE_STATE_CHANGE, ID());
		P.w_u8(u8(new_state));
		u_EventSend(P);
	};

	m_iPreviousStateTime = m_iStateTime = 0;
}

bool CCustomZone::Enable()
{
	if (IsEnabled()) return false;

	o_switch_2_fast();

	for (OBJECT_INFO_VEC_IT it = m_ObjectInfoMap.begin();
	     m_ObjectInfoMap.end() != it; ++it)
	{
		CGameObject* pObject = (*it).object;
		if (!pObject) continue;
		PlayEntranceParticles(pObject);
		PlayObjectIdleParticles(pObject);
	}
	PlayIdleParticles();
	return true;
};

bool CCustomZone::Disable()
{
	if (!IsEnabled()) return false;
	o_switch_2_slow();

	for (OBJECT_INFO_VEC_IT it = m_ObjectInfoMap.begin(); m_ObjectInfoMap.end() != it; ++it)
	{
		CGameObject* pObject = (*it).object;
		if (!pObject)
			continue;

		StopObjectIdleParticles(pObject);
	}
	StopIdleParticles();
	if (m_actor_effector)
		m_actor_effector->Stop();

	return false;
};

void CCustomZone::ZoneEnable()
{
	SwitchZoneState(eZoneStateIdle);
};

void CCustomZone::ZoneDisable()
{
	SwitchZoneState(eZoneStateDisabled);
};

void CCustomZone::StartWind()
{
	if (m_fDistanceToCurEntity > WIND_RADIUS) return;

	m_zone_flags.set(eBlowoutWindActive, TRUE);

	m_fStoreWindPower = g_pGamePersistent->Environment().wind_strength_factor;
	clamp(g_pGamePersistent->Environment().wind_strength_factor, 0.f, 1.f);
}

void CCustomZone::StopWind()
{
	if (!m_zone_flags.test(eBlowoutWindActive)) return;
	m_zone_flags.set(eBlowoutWindActive, FALSE);
	g_pGamePersistent->Environment().wind_strength_factor = m_fStoreWindPower;
}

void CCustomZone::UpdateWind()
{
	if (!m_zone_flags.test(eBlowoutWindActive)) return;

	if (m_fDistanceToCurEntity > WIND_RADIUS || m_dwBlowoutWindTimeEnd < (u32)m_iStateTime)
	{
		StopWind();
		return;
	}

	if (m_dwBlowoutWindTimePeak > (u32)m_iStateTime)
	{
		g_pGamePersistent->Environment().wind_strength_factor = m_fBlowoutWindPowerMax + (m_fStoreWindPower -
				m_fBlowoutWindPowerMax) *
			float(m_dwBlowoutWindTimePeak - (u32)m_iStateTime) /
			float(m_dwBlowoutWindTimePeak - m_dwBlowoutWindTimeStart);
		clamp(g_pGamePersistent->Environment().wind_strength_factor, 0.f, 1.f);
	}
	else
	{
		g_pGamePersistent->Environment().wind_strength_factor = m_fBlowoutWindPowerMax + (m_fStoreWindPower -
				m_fBlowoutWindPowerMax) *
			float((u32)m_iStateTime - m_dwBlowoutWindTimePeak) /
			float(m_dwBlowoutWindTimeEnd - m_dwBlowoutWindTimePeak);
		clamp(g_pGamePersistent->Environment().wind_strength_factor, 0.f, 1.f);
	}
}

u32 CCustomZone::ef_anomaly_type() const
{
	return (m_ef_anomaly_type);
}

u32 CCustomZone::ef_weapon_type() const
{
	VERIFY(m_ef_weapon_type != u32(-1));
	return (m_ef_weapon_type);
}

void CCustomZone::CreateHit(u16 id_to,
                            u16 id_from,
                            const Fvector& hit_dir,
                            float hit_power,
                            s16 bone_id,
                            const Fvector& pos_in_bone,
                            float hit_impulse,
                            ALife::EHitType hit_type)
{
	if (OnServer())
	{
		if (m_owner_id != u32(-1))
			id_from = (u16)m_owner_id;

		NET_Packet l_P;
		Fvector hdir = hit_dir;
		SHit Hit = SHit(hit_power, hdir, this, bone_id, pos_in_bone, hit_impulse, hit_type, 0.0f, false);
		Hit.GenHeader(GE_HIT, id_to);
		Hit.whoID = id_from;
		Hit.weaponID = this->ID();
		Hit.Write_Packet(l_P);
		u_EventSend(l_P);
	};
}

void CCustomZone::net_Relcase(CObject* O)
{
	CGameObject* GO = smart_cast<CGameObject*>(O);
	OBJECT_INFO_VEC_IT it = std::find(m_ObjectInfoMap.begin(), m_ObjectInfoMap.end(), GO);
	if (it != m_ObjectInfoMap.end())
	{
		exit_Zone(*it);
		m_ObjectInfoMap.erase(it);
	}
	if (GO->ID() == m_owner_id) m_owner_id = u32(-1);

	if (m_actor_effector && m_actor_effector->m_pActor && m_actor_effector->m_pActor->ID() == GO->ID())
		m_actor_effector->Stop();

	inherited::net_Relcase(O);
}

void CCustomZone::enter_Zone(SZoneObjectInfo& io)
{
	if (m_zone_flags.test(eAffectPickDOF) && Level().CurrentEntity())
	{
		if (io.object->ID() == Level().CurrentEntity()->ID())
			GamePersistent().SetPickableEffectorDOF(true);
	}
}

void CCustomZone::exit_Zone(SZoneObjectInfo& io)
{
	StopObjectIdleParticles(io.object);

	if (m_zone_flags.test(eAffectPickDOF) && Level().CurrentEntity())
	{
		if (io.object->ID() == Level().CurrentEntity()->ID())
			GamePersistent().SetPickableEffectorDOF(false);
	}
}

void CCustomZone::PlayAccumParticles()
{
	if (!m_zone_flags.test(eFastMode)) return;
	if (m_sAccumParticles.size())
	{
		CParticlesObject* pParticles;
		pParticles = CParticlesObject::Create(*m_sAccumParticles,TRUE);
		pParticles->UpdateParent(XFORM(), zero_vel);
		pParticles->Play(false);
	}

	if (m_accum_sound._handle())
		m_accum_sound.play_at_pos(0, Position());
}

void CCustomZone::PlayAwakingParticles()
{
	if (!m_zone_flags.test(eFastMode)) return;
	if (m_sAwakingParticles.size())
	{
		CParticlesObject* pParticles;
		pParticles = CParticlesObject::Create(*m_sAwakingParticles,TRUE);
		pParticles->UpdateParent(XFORM(), zero_vel);
		pParticles->Play(false);
	}

	if (m_awaking_sound._handle())
		m_awaking_sound.play_at_pos(0, Position());
}

void CCustomZone::UpdateOnOffState()
{
	if (!m_zone_flags.test(eUseOnOffTime)) return;

	bool dest_state;
	u32 t = (Device.dwTimeGlobal - m_StartTime + m_TimeShift) % (m_TimeToEnable + m_TimeToDisable);
	if (t < m_TimeToEnable)
	{
		dest_state = true;
	}
	else if (t >= (m_TimeToEnable + m_TimeToDisable))
	{
		dest_state = true;
	}
	else
	{
		dest_state = false;
		VERIFY(t<(m_TimeToEnable+m_TimeToDisable));
	}

	if ((eZoneStateDisabled == m_eZoneState) && dest_state)
	{
		GoEnabledState();
	}
	else if ((eZoneStateIdle == m_eZoneState) && !dest_state)
	{
		GoDisabledState();
	}
}

void CCustomZone::GoDisabledState()
{
	//switch to disable	
	NET_Packet P;
	u_EventGen(P, GE_ZONE_STATE_CHANGE, ID());
	P.w_u8(u8(eZoneStateDisabled));
	u_EventSend(P);

	OBJECT_INFO_VEC_IT it = m_ObjectInfoMap.begin();
	OBJECT_INFO_VEC_IT it_e = m_ObjectInfoMap.end();

	for (; it != it_e; ++it)
		exit_Zone(*it);

	m_ObjectInfoMap.clear();
	feel_touch.clear();
}

void CCustomZone::GoEnabledState()
{
	//switch to idle	
	NET_Packet P;
	u_EventGen(P, GE_ZONE_STATE_CHANGE, ID());
	P.w_u8(u8(eZoneStateIdle));
	u_EventSend(P);
}

bool CCustomZone::feel_touch_on_contact(CObject* O)
{
	if ((spatial.type | STYPE_VISIBLEFORAI) != spatial.type)
		return (false);

	return (inherited::feel_touch_on_contact(O));
}

BOOL CCustomZone::AlwaysTheCrow()
{
	bool b_idle = ZoneState() == eZoneStateIdle || ZoneState() == eZoneStateDisabled;
	if (!b_idle || (m_zone_flags.test(eAlwaysFastmode) && IsEnabled()))
		return TRUE;
	else
		return inherited::AlwaysTheCrow();
}

void CCustomZone::CalcDistanceTo(const Fvector& P, float& dist, float& radius)
{
	R_ASSERT(CFORM()->Type()==cftShape);
	CCF_Shape* Sh = (CCF_Shape*)CFORM();

	dist = P.distance_to(Position());
	float sr = CFORM()->getSphere().R;
	//quick test
	if (Sh->Shapes().size() == 1)
	{
		radius = sr;
		return;
	}
	/*
		//2nd quick test
		Fvector				SC;
		float				dist2;
		XF.transform_tiny	(SC,CFORM()->getSphere().P);
		dist2				= P.distance_to(SC);
		if(dist2>sr)
		{
			radius		= sr;
			return;
		}
	*/
	//full test
	const Fmatrix& XF = XFORM();
	xr_vector<CCF_Shape::shape_def>& Shapes = Sh->Shapes();
	CCF_Shape::shape_def* nearest_s = NULL;
	float nearest = flt_max;


	Fvector sP;

	xr_vector<CCF_Shape::shape_def>::iterator it = Shapes.begin();
	xr_vector<CCF_Shape::shape_def>::iterator it_e = Shapes.end();
	for (; it != it_e; ++it)
	{
		CCF_Shape::shape_def& s = *it;
		float d = 0.0f;
		switch (s.type)
		{
		case 0: // sphere
			sP = s.data.sphere.P;
			break;
		case 1: // box
			sP = s.data.box.c;
			break;
		}

		XF.transform_tiny(sP);
		d = P.distance_to(sP);
		if (d < nearest)
		{
			nearest = d;
			nearest_s = &s;
		}
	}
	R_ASSERT(nearest_s);

	dist = nearest;

	if (nearest_s->type == 0)
		radius = nearest_s->data.sphere.R;
	else
	{
		float r1 = nearest_s->data.box.i.magnitude();
		float r2 = nearest_s->data.box.j.magnitude();
		float r3 = nearest_s->data.box.k.magnitude();
		radius = _max(r1, r2);
		radius = _max(radius, r3);
	}
}

// Lain: added Start/Stop idle light calls
void CCustomZone::o_switch_2_fast()
{
	if (m_zone_flags.test(eFastMode)) return;
	m_zone_flags.set(eFastMode, TRUE);
	StartIdleLight();
	PlayIdleParticles(true);
	processing_activate();
}

void CCustomZone::o_switch_2_slow()
{
	if (!m_zone_flags.test(eFastMode)) return;
	m_zone_flags.set(eFastMode, FALSE);
	if (!light_in_slow_mode())
	{
		StopIdleLight();
	}
	StopIdleParticles(true);
	processing_deactivate();
}

void CCustomZone::save(NET_Packet& output_packet)
{
	inherited::save(output_packet);
	output_packet.w_u8(static_cast<u8>(m_eZoneState));
}

void CCustomZone::load(IReader& input_packet)
{
	inherited::load(input_packet);

	CCustomZone::EZoneState temp = static_cast<CCustomZone::EZoneState>(input_packet.r_u8());

	if (temp == eZoneStateDisabled)
		m_eZoneState = eZoneStateDisabled;
	else
		m_eZoneState = eZoneStateIdle;
}

void CCustomZone::GrassZoneUpdate()
{
	if (m_BendGrass_idle_anim == -1 && m_BendGrass_whenactive_anim == -1)
		return;

	IGame_Persistent::grass_data& GData = g_pGamePersistent->grass_shader_data;
	bool IsActive;
	s8 targetAnim = -1;

	// If m_BendGrass_Blowout_time is not set, use m_eZoneState to detect activation
	if (m_BendGrass_Blowout_time <= -1)
		IsActive = m_eZoneState != eZoneStateIdle;
	else
		IsActive = m_fBlowoutTimeLeft > (float)Device.dwTimeGlobal;

	// Target animation depending if Zone is active
	if (IsActive)
		targetAnim = (m_BendGrass_whenactive_anim > -1) ? m_BendGrass_whenactive_anim : m_BendGrass_idle_anim;
	else
		targetAnim = m_BendGrass_idle_anim;

	// Update grass bender if the animation is > -1
	if (targetAnim > 0 || (grassbender_id > 0 && GData.anim[grassbender_id] > 0))
		g_pGamePersistent->GrassBendersUpdate(ID(), grassbender_id, grassbender_frame, Position(), m_BendGrass_idle_radius, 0.0f, false);
	else
		g_pGamePersistent->GrassBendersRemoveByIndex(grassbender_id);

	// Return if grassbender_id doesn't exist
	if (grassbender_id <= 0)
		return;

	// Animation transition, diminish intensity to 0 and change.
	if (GData.anim[grassbender_id] != targetAnim)
	{
		GData.str_target[grassbender_id] += g_pGamePersistent->GrassBenderToValue(GData.str_target[grassbender_id], 0.0f, 7.5f, false);

		if (GData.str_target[grassbender_id] <= 0.05f)
			GData.anim[grassbender_id] = targetAnim;

		return;
	}

	// Apply settings when needed
	if (IsActive)
	{
		if (m_BendGrass_whenactive_speed >= 0)
			GData.speed[grassbender_id] += g_pGamePersistent->GrassBenderToValue(GData.speed[grassbender_id], m_BendGrass_whenactive_speed, 10.0f, true);

		if (m_BendGrass_whenactive_str >= 0)
			GData.str_target[grassbender_id] += g_pGamePersistent->GrassBenderToValue(GData.str_target[grassbender_id], m_BendGrass_whenactive_str, 10.0f, true);
	}
	else
	{
		GData.speed[grassbender_id] += g_pGamePersistent->GrassBenderToValue(GData.speed[grassbender_id], m_BendGrass_idle_speed, 10.0f, true);
		GData.str_target[grassbender_id] += g_pGamePersistent->GrassBenderToValue(GData.str_target[grassbender_id], m_BendGrass_idle_str, 10.0f, true);
	}
}

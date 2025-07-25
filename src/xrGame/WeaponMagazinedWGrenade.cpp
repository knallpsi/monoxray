#include "stdafx.h"
#include "weaponmagazinedwgrenade.h"
#include "entity.h"
#include "ParticlesObject.h"
#include "GrenadeLauncher.h"
#include "xrserver_objects_alife_items.h"
#include "ExplosiveRocket.h"
#include "Actor.h"
#include "xr_level_controller.h"
#include "level.h"
#include "object_broker.h"
#include "game_base_space.h"
#include "../xrphysics/MathUtils.h"
#include "player_hud.h"
#include "../build_config_defines.h"

#ifdef DEBUG
#	include "phdebug.h"
#endif

CWeaponMagazinedWGrenade::CWeaponMagazinedWGrenade(ESoundTypes eSoundType) : CWeaponMagazined(eSoundType)
{
	m_ammoType2 = 0;
	m_bGrenadeMode = false;
}

CWeaponMagazinedWGrenade::~CWeaponMagazinedWGrenade()
{
}

BOOL g_aimmode_remember = 1;

void CWeaponMagazinedWGrenade::Load(LPCSTR section)
{
	inherited::Load(section);
	CRocketLauncher::Load(section);

	//// Sounds
	m_sounds.LoadSound(section, "snd_shoot_grenade", "sndShotG", true, m_eSoundShot);
	m_sounds.LoadSound(section, "snd_reload_grenade", "sndReloadG", true, m_eSoundReload);
	m_sounds.LoadSound(section, "snd_switch", "sndSwitch", true, m_eSoundReload);

	m_sFlameParticles2 = pSettings->r_string(section, "grenade_flame_particles");

	if (m_eGrenadeLauncherStatus == ALife::eAddonPermanent)
	{
		CRocketLauncher::m_fLaunchSpeed = pSettings->r_float(section, "grenade_vel");
	}
	LoadLauncherKoeffs();

	// load ammo classes SECOND (grenade_class)
	m_ammoTypes2.clear();
	LPCSTR S = pSettings->r_string(section, "grenade_class");
	if (S && S[0])
	{
		string128 _ammoItem;
		int count = _GetItemCount(S);
		for (int it = 0; it < count; ++it)
		{
			_GetItem(S, it, _ammoItem);
			m_ammoTypes2.push_back(_ammoItem);
		}
	}

	iMagazineSize2 = iMagazineSize;
}

void CWeaponMagazinedWGrenade::net_Destroy()
{
	inherited::net_Destroy();
}

BOOL CWeaponMagazinedWGrenade::net_Spawn(CSE_Abstract* DC)
{
	CSE_ALifeItemWeapon* const weapon = smart_cast<CSE_ALifeItemWeapon*>(DC);
	R_ASSERT(weapon);
	if (IsGameTypeSingle())
	{
		inherited::net_Spawn_install_upgrades(weapon->m_upgrades);
	}

	BOOL l_res = inherited::net_Spawn(DC);

	UpdateGrenadeVisibility(!!iAmmoElapsed);
	SetPending(FALSE);

	iAmmoElapsed2 = weapon->a_elapsed_grenades.grenades_count;
	m_ammoType2 = weapon->a_elapsed_grenades.grenades_type;

	m_DefaultCartridge2.Load(m_ammoTypes2[m_ammoType2].c_str(), m_ammoType2);

	if (!IsGameTypeSingle())
	{
		if (!m_bGrenadeMode && IsGrenadeLauncherAttached() && !getRocketCount() && iAmmoElapsed2)
		{
			m_magazine2.push_back(m_DefaultCartridge2);

			shared_str grenade_name = m_DefaultCartridge2.m_ammoSect;
			shared_str fake_grenade_name = pSettings->r_string(grenade_name, "fake_grenade_name");

			CRocketLauncher::SpawnRocket(*fake_grenade_name, this);
		}
	}
	else
	{
		xr_vector<CCartridge>* pM = NULL;
		bool b_if_grenade_mode = (m_bGrenadeMode && iAmmoElapsed && !getRocketCount());
		if (b_if_grenade_mode)
			pM = &m_magazine;

		bool b_if_simple_mode = (!m_bGrenadeMode && m_magazine2.size() && !getRocketCount());
		if (b_if_simple_mode)
			pM = &m_magazine2;

		if (b_if_grenade_mode || b_if_simple_mode)
		{
			shared_str fake_grenade_name = pSettings->r_string(pM->back().m_ammoSect, "fake_grenade_name");

			CRocketLauncher::SpawnRocket(*fake_grenade_name, this);
		}
	}
	return l_res;
}

void CWeaponMagazinedWGrenade::switch2_Reload()
{
	VERIFY(GetState() == eReload);
	if (m_bGrenadeMode)
	{
		m_needReload = true;
		PlaySound("sndReloadG", get_LastFP2());
		PlayHUDMotion("anm_reload_g", TRUE, this, GetState());
		SetPending(TRUE);
	}
	else
		inherited::switch2_Reload();
}

void CWeaponMagazinedWGrenade::OnShot()
{
	if (m_bGrenadeMode)
	{
		PlayAnimShoot();
		PlaySound("sndShotG", get_LastFP2());
		AddShotEffector();
		StartFlameParticles2();

#ifndef EXTENDED_WEAPON_CALLBACKS
		CGameObject* object = smart_cast<CGameObject*>(H_Parent());
		if (object)
			object->callback(GameObject::eOnWeaponFired)(object->lua_game_object(), this->lua_game_object(), iAmmoElapsed);
#endif
	}
	else
		inherited::OnShot();
}

void CWeaponMagazinedWGrenade::PlayAnimFireModeSwitch()
{
	if (IsGrenadeLauncherAttached())
	{
		if (!m_bGrenadeMode)
		{
			if (!m_bHasDifferentFireModes) return;
			if (m_aFireModes.size() <= 1) return;
			if (GetState() != eIdle) return;

			if (HudAnimationExist("anm_switch_mode_w_gl"))
			{
				SetPending(TRUE);
				iAmmoElapsed == 0 && HudAnimationExist("anm_switch_mode_w_gl_empty")
					? PlayHUDMotion("anm_switch_mode_w_gl_empty", TRUE, this, eSwitchMode)
					: PlayHUDMotion("anm_switch_mode_w_gl", TRUE, this, eSwitchMode);
			}
			else
				UpdateFireMode();

			if (m_sounds.FindSoundItem("sndSwitchMode", false))
				PlaySound("sndSwitchMode", get_LastFP());
		}
	}
	else
	{
		inherited::PlayAnimFireModeSwitch();
	}
}

bool CWeaponMagazinedWGrenade::SwitchMode(bool force)
{
	bool bUsefulStateToSwitch = (eIdle == GetState() && !IsPending());

	if (!force && !bUsefulStateToSwitch)
		return false;

	if (!IsGrenadeLauncherAttached())
		return false;

	//OnZoomOut();

	SetPending(TRUE);

	PerformSwitchGL();

	PlaySound("sndSwitch", get_LastFP());

	PlayAnimModeSwitch();

	m_BriefInfo_CalcFrame = 0;

	return true;
}

extern BOOL useSeparateUBGLKeybind;
void CWeaponMagazinedWGrenade::PerformSwitchGL()
{
	m_bGrenadeMode = !m_bGrenadeMode;

	if (useSeparateUBGLKeybind) {
		u8 newzoomtype = 0;

		if (g_aimmode_remember)
		{
			newzoomtype = zoomTypeBeforeLauncher;
		}

		SetZoomType(m_bGrenadeMode ? 2 : newzoomtype);
	} else {
		SetZoomType(m_bGrenadeMode ? 2 : 0);
	}	

	UpdateUIScope();

	iMagazineSize = m_bGrenadeMode ? 1 : iMagazineSize2;

	m_ammoTypes.swap(m_ammoTypes2);

	swap(m_ammoType, m_ammoType2);
	swap(m_DefaultCartridge, m_DefaultCartridge2);

	m_magazine.swap(m_magazine2);
	iAmmoElapsed = (int)m_magazine.size();

	if (m_bGrenadeMode && !getRocketCount())
	{
		shared_str fake_grenade_name = pSettings->r_string(m_ammoTypes[m_ammoType].c_str(), "fake_grenade_name");

		CRocketLauncher::SpawnRocket(*fake_grenade_name, this);
	}

	m_BriefInfo_CalcFrame = 0;
}

void CWeaponMagazinedWGrenade::SetAmmoElapsed2(int ammo_count)
{
	iAmmoElapsed2 = ammo_count;

	u32 uAmmo = u32(iAmmoElapsed2);

	if (uAmmo != m_magazine2.size())
	{
		if (uAmmo > m_magazine2.size())
		{
			CCartridge l_cartridge;
			l_cartridge.Load(m_ammoTypes2[m_ammoType2].c_str(), m_ammoType2, m_APk);
			while (uAmmo > m_magazine2.size())
				m_magazine2.push_back(l_cartridge);
		}
		else
		{
			while (uAmmo < m_magazine2.size())
				m_magazine2.pop_back();
		};
	};
}

void CWeaponMagazinedWGrenade::AmmoTypeForEach2(const ::luabind::functor<bool> &funct)
{
	for (u8 i = 0; i < u8(m_ammoTypes2.size()); ++i)
	{
		if (funct(i, *m_ammoTypes2[i]))
			break;
	}
}

bool CWeaponMagazinedWGrenade::Action(u16 cmd, u32 flags)
{
	if (m_bGrenadeMode && cmd == kWPN_FIRE)
	{
		if (IsPending())
			return false;

		if (ParentIsActor() && Actor()->is_safemode())
		{
			Actor()->set_safemode(false);

			if (iAmmoElapsed)
				return false;
		}

		if (flags & CMD_START)
		{
			if (iAmmoElapsed)
				LaunchGrenade();
			else
				Reload();

			if (GetState() == eIdle)
				OnEmptyClick();
		}
		return true;
	}
	if (inherited::Action(cmd, flags))
		return true;

	/*switch (cmd)
	{
	case kWPN_FUNC:
	{
	    if (flags&CMD_START && !IsPending())
	        SwitchState(eSwitch);
	    return true;
	}
	}*/
	return false;
}

#include "inventory.h"
#include "inventoryOwner.h"

void CWeaponMagazinedWGrenade::state_Fire(float dt)
{
	VERIFY(fOneShotTime > 0.f);

	//режим стрельбы подствольника
	if (m_bGrenadeMode)
	{
		/*
		fTime					-=dt;
		while (fTime<=0 && (iAmmoElapsed>0) && (IsWorking() || m_bFireSingleShot))
		{
		++m_iShotNum;
		OnShot			();

		// Ammo
		if(Local())
		{
		VERIFY				(m_magazine.size());
		m_magazine.pop_back	();
		--iAmmoElapsed;

		VERIFY((u32)iAmmoElapsed == m_magazine.size());
		}
		}
		UpdateSounds				();
		if(m_iShotNum == m_iQueueSize)
		FireEnd();
		*/
	}
		//режим стрельбы очередями
	else
		inherited::state_Fire(dt);
}

void CWeaponMagazinedWGrenade::OnEvent(NET_Packet& P, u16 type)
{
	inherited::OnEvent(P, type);
	u16 id;
	switch (type)
	{
	case GE_OWNERSHIP_TAKE:
		{
			P.r_u16(id);
			CRocketLauncher::AttachRocket(id, this);
		}
		break;
	case GE_OWNERSHIP_REJECT:
	case GE_LAUNCH_ROCKET:
		{
			bool bLaunch = (type == GE_LAUNCH_ROCKET);
			P.r_u16(id);
			CRocketLauncher::DetachRocket(id, bLaunch);
			if (bLaunch)
			{
				PlayAnimShoot();
				PlaySound("sndShotG", get_LastFP2());
				AddShotEffector();
				StartFlameParticles2();
			}
			break;
		}
	}
}

void CWeaponMagazinedWGrenade::LaunchGrenade()
{
	if (!getRocketCount()) return;
	R_ASSERT(m_bGrenadeMode);
	{
#ifdef CROCKETLAUNCHER_CHANGE
		LPCSTR ammo_name = m_ammoTypes[m_ammoType].c_str();
		float launch_speed = READ_IF_EXISTS(pSettings, r_float, ammo_name, "ammo_grenade_vel", CRocketLauncher::m_fLaunchSpeed);
#endif
		Fvector p1, d;
		p1.set(get_LastFP2());
		d.set(get_LastFD());
		CEntity* E = smart_cast<CEntity*>(H_Parent());

		if (E)
		{
			CInventoryOwner* io = smart_cast<CInventoryOwner*>(H_Parent());
			if (NULL == io->inventory().ActiveItem())
			{
				Log("current_state", GetState());
				Log("next_state", GetNextState());
				Log("item_sect", cNameSect().c_str());
				Log("H_Parent", H_Parent()->cNameSect().c_str());
			}
			E->g_fireParams(this, p1, d);
		}
		if (IsGameTypeSingle())
			p1.set(get_LastFP2());

		Fmatrix launch_matrix;
		launch_matrix.identity();
		launch_matrix.k.set(d);
		Fvector::generate_orthonormal_basis(launch_matrix.k,
		                                    launch_matrix.j,
		                                    launch_matrix.i);

		launch_matrix.c.set(p1);

		if (IsGameTypeSingle() && IsZoomed() && smart_cast<CActor*>(H_Parent()))
		{
			H_Parent()->setEnabled(FALSE);
			setEnabled(FALSE);

			collide::rq_result RQ;
			BOOL HasPick = Level().ObjectSpace.RayPick(p1, d, 300.0f, collide::rqtStatic, RQ, this);

			setEnabled(TRUE);
			H_Parent()->setEnabled(TRUE);

			if (HasPick)
			{
				Fvector Transference;
				Transference.mul(d, RQ.range);
				Fvector res[2];
#ifdef		DEBUG
				//.				DBG_OpenCashedDraw();
				//.				DBG_DrawLine(p1,Fvector().add(p1,d),D3DCOLOR_XRGB(255,0,0));
#endif
#ifdef CROCKETLAUNCHER_CHANGE
				u8 canfire0 = TransferenceAndThrowVelToThrowDir(Transference, launch_speed, EffectiveGravity(), res);
#else
				u8 canfire0 = TransferenceAndThrowVelToThrowDir(Transference,
				                                                CRocketLauncher::m_fLaunchSpeed,
				                                                EffectiveGravity(),
				                                                res);
#endif
#ifdef DEBUG
				//.				if(canfire0>0)DBG_DrawLine(p1,Fvector().add(p1,res[0]),D3DCOLOR_XRGB(0,255,0));
				//.				if(canfire0>1)DBG_DrawLine(p1,Fvector().add(p1,res[1]),D3DCOLOR_XRGB(0,0,255));
				//.				DBG_ClosedCashedDraw(30000);
#endif

				if (canfire0 != 0)
				{
					d = res[0];
				};
			}
		};

		d.normalize();
#ifdef CROCKETLAUNCHER_CHANGE
		d.mul(launch_speed);
#else
		d.mul(CRocketLauncher::m_fLaunchSpeed);
#endif
		VERIFY2(_valid(launch_matrix), "CWeaponMagazinedWGrenade::SwitchState. Invalid launch_matrix!");
		CRocketLauncher::LaunchRocket(launch_matrix, d, zero_vel);

		CExplosiveRocket* pGrenade = smart_cast<CExplosiveRocket*>(getCurrentRocket());
		VERIFY(pGrenade);
		pGrenade->SetInitiator(H_Parent()->ID());

		if (Local() && OnServer())
		{
			VERIFY(m_magazine.size());
			m_magazine.pop_back();
			--iAmmoElapsed;
			VERIFY((u32) iAmmoElapsed == m_magazine.size());

			NET_Packet P;
			u_EventGen(P, GE_LAUNCH_ROCKET, ID());
			P.w_u16(getCurrentRocket()->ID());
			u_EventSend(P);
		};
	}
}

void CWeaponMagazinedWGrenade::FireEnd()
{
	if (m_bGrenadeMode)
	{
		CWeapon::FireEnd();
	}
	else
		inherited::FireEnd();
}

void CWeaponMagazinedWGrenade::OnMagazineEmpty()
{
	if (GetState() == eIdle)
	{
		OnEmptyClick();
	}
}

void CWeaponMagazinedWGrenade::ReloadMagazine()
{
	inherited::ReloadMagazine();

	//перезарядка подствольного гранатомета
	if (iAmmoElapsed && !getRocketCount() && m_bGrenadeMode)
	{
		shared_str fake_grenade_name = pSettings->r_string(m_ammoTypes[m_ammoType].c_str(), "fake_grenade_name");

		CRocketLauncher::SpawnRocket(*fake_grenade_name, this);
	}
}

void CWeaponMagazinedWGrenade::OnStateSwitch(u32 S, u32 oldState)
{
	switch (S)
	{
	case eSwitch:
		{
			if (!SwitchMode())
			{
				SwitchState(eIdle);
				return;
			}
		}
		break;
	}

	inherited::OnStateSwitch(S, oldState);
	UpdateGrenadeVisibility(!!iAmmoElapsed || S == eReload);
}

void CWeaponMagazinedWGrenade::OnAnimationEnd(u32 state)
{
	switch (state)
	{
	case eSwitch:
		{
			SetPending(FALSE);
			SwitchState(eIdle);
		}
		break;
	case eFire:
		{
			if (m_bGrenadeMode)
				Reload();
		}
		break;
	}
	inherited::OnAnimationEnd(state);
}

void CWeaponMagazinedWGrenade::OnH_B_Independent(bool just_before_destroy)
{
	inherited::OnH_B_Independent(just_before_destroy);

	SetPending(FALSE);
	if (m_bGrenadeMode)
	{
		SetState(eIdle);
		SetPending(FALSE);
	}
}

bool CWeaponMagazinedWGrenade::CanAttach(PIItem pIItem)
{
	CGrenadeLauncher* pGrenadeLauncher = smart_cast<CGrenadeLauncher*>(pIItem);

	if (pGrenadeLauncher &&
		ALife::eAddonAttachable == m_eGrenadeLauncherStatus &&
		0 == (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) &&
		!xr_strcmp(*m_sGrenadeLauncherName, pIItem->object().cNameSect()))
		return true;
	else
		return inherited::CanAttach(pIItem);
}

bool CWeaponMagazinedWGrenade::CanDetach(LPCSTR item_section_name)
{
	if (ALife::eAddonAttachable == m_eGrenadeLauncherStatus &&
		0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) &&
		!xr_strcmp(*m_sGrenadeLauncherName, item_section_name))
		return true;
	else
		return inherited::CanDetach(item_section_name);
}

bool CWeaponMagazinedWGrenade::Attach(PIItem pIItem, bool b_send_event)
{
	CGrenadeLauncher* pGrenadeLauncher = smart_cast<CGrenadeLauncher*>(pIItem);

	if (pGrenadeLauncher &&
		ALife::eAddonAttachable == m_eGrenadeLauncherStatus &&
		0 == (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) &&
		!xr_strcmp(*m_sGrenadeLauncherName, pIItem->object().cNameSect()))
	{
		m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher;

		CRocketLauncher::m_fLaunchSpeed = pGrenadeLauncher->GetGrenadeVel();

		//уничтожить подствольник из инвентаря
		if (b_send_event)
		{
			if (OnServer())
				pIItem->object().DestroyObject();
		}
		InitAddons();
		UpdateAddonsVisibility();

		if (GetState() == eIdle)
			PlayAnimIdle();

		return true;
	}
	else
		return inherited::Attach(pIItem, b_send_event);
}

bool CWeaponMagazinedWGrenade::Detach(LPCSTR item_section_name, bool b_spawn_item)
{
	if (ALife::eAddonAttachable == m_eGrenadeLauncherStatus &&
		0 != (m_flagsAddOnState & CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) &&
		!xr_strcmp(*m_sGrenadeLauncherName, item_section_name))
	{
		m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher;

		// Now we need to unload GL's magazine
		if (!m_bGrenadeMode)
		{
			PerformSwitchGL();
		}
		UnloadMagazine();
		PerformSwitchGL();

		UpdateAddonsVisibility();

		if (GetState() == eIdle)
			PlayAnimIdle();

		InitAddons();

		return CInventoryItemObject::Detach(item_section_name, b_spawn_item);
	}
	else
		return inherited::Detach(item_section_name, b_spawn_item);
}

void CWeaponMagazinedWGrenade::InitAddons()
{
	inherited::InitAddons();

	if (GrenadeLauncherAttachable())
	{
		if (IsGrenadeLauncherAttached())
		{
			CRocketLauncher::m_fLaunchSpeed = pSettings->r_float(*m_sGrenadeLauncherName, "grenade_vel");
			ApplyLauncherKoeffs();
		}
		else
		{
			ResetLauncherKoeffs();
		}
	}
}

void CWeaponMagazinedWGrenade::LoadLauncherKoeffs()
{
	if (m_eGrenadeLauncherStatus == ALife::eAddonAttachable)
	{
		LPCSTR sect = GetGrenadeLauncherName().c_str();
		m_launcher_koef.cam_dispersion = READ_IF_EXISTS(pSettings, r_float, sect, "cam_dispersion_k", 1.0f);
		m_launcher_koef.cam_disper_inc = READ_IF_EXISTS(pSettings, r_float, sect, "cam_dispersion_inc_k", 1.0f);
		m_launcher_koef.pdm_base = READ_IF_EXISTS(pSettings, r_float, sect, "PDM_disp_base_k", 1.0f);
		m_launcher_koef.pdm_accel = READ_IF_EXISTS(pSettings, r_float, sect, "PDM_disp_accel_k", 1.0f);
		m_launcher_koef.pdm_vel = READ_IF_EXISTS(pSettings, r_float, sect, "PDM_disp_vel_k", 1.0f);
		m_launcher_koef.crosshair_inertion = READ_IF_EXISTS(pSettings, r_float, sect, "crosshair_inertion_k", 1.0f);
		m_launcher_koef.zoom_rotate_time = READ_IF_EXISTS(pSettings, r_float, sect, "zoom_rotate_time_k", 1.0f);
	}

	clamp(m_launcher_koef.cam_dispersion, 0.01f, 2.0f);
	clamp(m_launcher_koef.cam_disper_inc, 0.01f, 2.0f);
	clamp(m_launcher_koef.pdm_base, 0.01f, 2.0f);
	clamp(m_launcher_koef.pdm_accel, 0.01f, 2.0f);
	clamp(m_launcher_koef.pdm_vel, 0.01f, 2.0f);
	clamp(m_launcher_koef.crosshair_inertion, 0.01f, 2.0f);
	clamp(m_launcher_koef.zoom_rotate_time, 0.01f, 2.0f);
}

void CWeaponMagazinedWGrenade::ApplyLauncherKoeffs()
{
	cur_launcher_koef = m_launcher_koef;
}

void CWeaponMagazinedWGrenade::ResetLauncherKoeffs()
{
	cur_launcher_koef.Reset();
}

bool CWeaponMagazinedWGrenade::UseScopeTexture()
{
	if (IsGrenadeLauncherAttached() && m_bGrenadeMode) return false;

	return true;
};

float CWeaponMagazinedWGrenade::CurrentZoomFactor()
{
	if (IsGrenadeLauncherAttached() && m_bGrenadeMode) return m_zoom_params.m_fScopeZoomFactor;
	return inherited::CurrentZoomFactor();
}

//виртуальные функции для проигрывания анимации HUD
void CWeaponMagazinedWGrenade::PlayAnimShow()
{
	VERIFY(GetState() == eShowing);
	if (IsGrenadeLauncherAttached())
	{
		if (!m_bGrenadeMode)
			HUD_VisualBulletUpdate();

		if (!m_bGrenadeMode)
			iAmmoElapsed == 0 && HudAnimationExist("anm_show_empty_w_gl")
			? PlayHUDMotion("anm_show_empty_w_gl", FALSE, this, GetState(), 1.f, 0.f, false)
			: PlayHUDMotion("anm_show_w_gl", FALSE, this, GetState(), 1.f, 0.f, false);
		else
			iAmmoElapsed == 0 && HudAnimationExist("anm_show_empty_g")
			? PlayHUDMotion("anm_show_empty_g", FALSE, this, GetState(), 1.f, 0.f, false)
			: PlayHUDMotion("anm_show_g", FALSE, this, GetState(), 1.f, 0.f, false);
	}
	else
		inherited::PlayAnimShow();
}

void CWeaponMagazinedWGrenade::PlayAnimHide()
{
	VERIFY(GetState() == eHiding);

	if (IsGrenadeLauncherAttached())
		if (!m_bGrenadeMode)
			iAmmoElapsed == 0 && HudAnimationExist("anm_hide_empty_w_gl")
			? PlayHUDMotion("anm_hide_empty_w_gl", TRUE, this, GetState())
			: PlayHUDMotion("anm_hide_w_gl", TRUE, this, GetState());
		else
			iAmmoElapsed == 0 && HudAnimationExist("anm_hide_empty_g")
			? PlayHUDMotion("anm_hide_empty_g", TRUE, this, GetState())
			: PlayHUDMotion("anm_hide_g", TRUE, this, GetState());

	else
		inherited::PlayAnimHide();
}

void CWeaponMagazinedWGrenade::PlayAnimReload()
{
	VERIFY(GetState() == eReload);

#ifdef NEW_ANIMS //AVO: use new animations
	if (IsGrenadeLauncherAttached())
	{
		if (bMisfire)
		{
			if (HudAnimationExist("anm_reload_misfire_w_gl"))
			{
				PlayHUDMotion("anm_reload_misfire_w_gl", TRUE, this, GetState());
				bClearJamOnly = true;
				return;
			}
			else
				PlayHUDMotion("anm_reload_w_gl", TRUE, this, GetState());
		}
		else
		{
			if (iAmmoElapsed == 0)
			{
				if (HudAnimationExist("anm_reload_empty_w_gl"))
					PlayHUDMotion("anm_reload_empty_w_gl", TRUE, this, GetState());
				else
					PlayHUDMotion("anm_reload_w_gl", TRUE, this, GetState());
			}
			else
			{
				PlayHUDMotion("anm_reload_w_gl", TRUE, this, GetState());
			}
		}
	}
	else
		inherited::PlayAnimReload();
#else
    if (IsGrenadeLauncherAttached())
        PlayHUDMotion("anm_reload_w_gl", TRUE, this, GetState());
    else
        inherited::PlayAnimReload();
#endif //-NEW_ANIMS
}

void CWeaponMagazinedWGrenade::PlayAnimIdle()
{
	if (GetState() == eSwitch)
		return;

	if (IsGrenadeLauncherAttached())
	{
		if (IsZoomed())
		{
			if (m_bGrenadeMode)
				iAmmoElapsed == 0 && HudAnimationExist("anm_idle_empty_g_aim")
				? PlayHUDMotion("anm_idle_empty_g_aim", TRUE, NULL, GetState())
				: PlayHUDMotion("anm_idle_g_aim", TRUE, NULL, GetState());
			else
				iAmmoElapsed == 0 && HudAnimationExist("anm_idle_empty_w_gl_aim")
				? PlayHUDMotion("anm_idle_empty_w_gl_aim", TRUE, NULL, GetState())
				: PlayHUDMotion("anm_idle_w_gl_aim", TRUE, NULL, GetState());
		}
		else
		{
			int act_state = 0;
			CActor* pActor = smart_cast<CActor*>(H_Parent());
			if (pActor && pActor->AnyMove())
			{
				CEntity::SEntityState st;
				pActor->g_State(st);
				if (pActor->is_safemode())
					act_state = 0;
				else if (st.bSprint)
				{
					act_state = 1;
				}
				else if (!st.bCrouch)
				{
					act_state = 2;
				}
				else if (st.bCrouch)
				{
					act_state = 3;
				}
			}

			if (m_bGrenadeMode)
			{
				if (act_state == 0)
					iAmmoElapsed == 0 && HudAnimationExist("anm_idle_empty_g")
					? PlayHUDMotion("anm_idle_empty_g", TRUE, NULL, GetState())
					: PlayHUDMotion("anm_idle_g", TRUE, NULL, GetState());
				else if (act_state == 1)
					iAmmoElapsed == 0 && HudAnimationExist("anm_idle_sprint_empty_g")
					? PlayHUDMotion("anm_idle_sprint_empty_g", TRUE, NULL, GetState())
					: PlayHUDMotion("anm_idle_sprint_g", TRUE, NULL, GetState());
				else if (act_state == 2)
				{
					iAmmoElapsed == 0 && HudAnimationExist("anm_idle_moving_empty_g")
						? PlayHUDMotion("anm_idle_moving_empty_g", TRUE, NULL, GetState())
						: PlayHUDMotion("anm_idle_moving_g", TRUE, NULL, GetState());
				}
				else if (act_state == 3)
				{
#ifdef NEW_ANIMS //AVO: custom move animation
					iAmmoElapsed == 0 && HudAnimationExist("anm_idle_moving_crouch_empty_g")
						? PlayHUDMotion("anm_idle_moving_crouch_empty_g", TRUE, NULL, GetState())
						: iAmmoElapsed == 0 && HudAnimationExist("anm_idle_moving_empty_g") ? PlayHUDMotion("anm_idle_moving_empty_g", TRUE, NULL, GetState(), .7f) : PlayHUDMotion("anm_idle_moving_g", TRUE, NULL, GetState(), .7f);
#endif //-NEW_ANIMS
				}
			}
			else
			{
				if (act_state == 0)
					iAmmoElapsed == 0 && HudAnimationExist("anm_idle_empty_w_gl")
						? PlayHUDMotion("anm_idle_empty_w_gl", TRUE, NULL, GetState())
						: PlayHUDMotion("anm_idle_w_gl", TRUE, NULL, GetState());
				else if (act_state == 1)
					iAmmoElapsed == 0 && HudAnimationExist("anm_idle_sprint_empty_w_gl")
						? PlayHUDMotion("anm_idle_sprint_empty_w_gl", TRUE, NULL, GetState())
						: PlayHUDMotion("anm_idle_sprint_w_gl", TRUE, NULL, GetState());
				else if (act_state == 2)
				{
					iAmmoElapsed == 0 && HudAnimationExist("anm_idle_moving_empty_w_gl")
						? PlayHUDMotion("anm_idle_moving_empty_w_gl", TRUE, NULL, GetState())
						: PlayHUDMotion("anm_idle_moving_w_gl", TRUE, NULL, GetState());
				}
				else if (act_state == 3)
				{
#ifdef NEW_ANIMS //AVO: custom move animation
					iAmmoElapsed == 0 && HudAnimationExist("anm_idle_moving_crouch_empty_w_gl")
						? PlayHUDMotion("anm_idle_moving_crouch_empty_w_gl", TRUE, NULL, GetState())
						: iAmmoElapsed == 0 && HudAnimationExist("anm_idle_moving_empty_w_gl") ? PlayHUDMotion("anm_idle_moving_empty_w_gl", TRUE, NULL, GetState(), .7f) : PlayHUDMotion("anm_idle_moving_w_gl", TRUE, NULL, GetState(), .7f);
#endif //-NEW_ANIMS
				}
			}
		}
	}
	else
		inherited::PlayAnimIdle();
}

void CWeaponMagazinedWGrenade::PlayAnimShoot()
{
	if (m_bGrenadeMode)
	{
		if (iAmmoElapsed > 1 || !HudAnimationExist("anm_shot_g_l"))
		{
			if (!IsZoomed() || !HudAnimationExist("anm_shots_g_aim"))
				PlayHUDMotion("anm_shots_g", TRUE, this, GetState(), 1.f, 0.f, false);
			else
				PlayHUDMotion("anm_shots_g_aim", TRUE, this, GetState(), 1.f, 0.f, false);
		}
		else
		{
			if(!IsZoomed() || !HudAnimationExist("anm_shot_g_l_aim"))
				PlayHUDMotion("anm_shot_g_l", TRUE, this, GetState(), 1.f, 0.f, false);
			else
				PlayHUDMotion("anm_shot_g_l_aim", TRUE, this, GetState(), 1.f, 0.f, false);
		}		
	}
	else
	{
		VERIFY(GetState() == eFire);
		if (IsGrenadeLauncherAttached())
			if (iAmmoElapsed > 1 || !HudAnimationExist("anm_shot_w_gl_l"))
			{
				if (!IsZoomed() || !HudAnimationExist("anm_shots_w_gl_aim"))
					PlayHUDMotion("anm_shots_w_gl", TRUE, this, GetState(), 1.f, 0.f, false);
				else
					PlayHUDMotion("anm_shots_w_gl_aim", TRUE, this, GetState(), 1.f, 0.f, false);
			}
			else
			{
				if (!IsZoomed() || !HudAnimationExist("anm_shot_w_gl_l_aim"))
					PlayHUDMotion("anm_shot_w_gl_l", TRUE, this, GetState(), 1.f, 0.f, false);
				else
					PlayHUDMotion("anm_shot_w_gl_l_aim", TRUE, this, GetState(), 1.f, 0.f, false);
			}
		else
			inherited::PlayAnimShoot();
	}
}

void CWeaponMagazinedWGrenade::PlayAnimModeSwitch()
{
	if (m_bGrenadeMode)
		iAmmoElapsed == 0 && HudAnimationExist("anm_switch_g_empty")
		? PlayHUDMotion("anm_switch_g_empty", TRUE, this, eSwitch)
		: HudAnimationExist("anm_switch_g") ? PlayHUDMotion("anm_switch_g", TRUE, this, eSwitch) : SwitchState(eSwitch);
	else
		iAmmoElapsed == 0 && HudAnimationExist("anm_switch_empty")
		? PlayHUDMotion("anm_switch_empty", TRUE, this, eSwitch)
		: HudAnimationExist("anm_switch") ? PlayHUDMotion("anm_switch", TRUE, this, eSwitch) : SwitchState(eSwitch);
}

bool CWeaponMagazinedWGrenade::TryPlayAnimBore()
{
	if (IsGrenadeLauncherAttached())
	{
		if (m_bGrenadeMode)
		{
			if (iAmmoElapsed == 0 && HudAnimationExist("anm_bore_empty_g"))
			{
				PlayHUDMotion("anm_bore_empty_g", TRUE, NULL, GetState());
				return true;
			}

			if (HudAnimationExist("anm_bore_g"))
			{
				PlayHUDMotion("anm_bore_g", TRUE, NULL, GetState());
				return true;
			}
		}
		else
		{
			if (iAmmoElapsed == 0 && HudAnimationExist("anm_bore_empty_w_gl"))
			{
				PlayHUDMotion("anm_bore_empty_w_gl", TRUE, NULL, GetState());
				return true;
			}

			if (HudAnimationExist("anm_bore_w_gl"))
			{
				PlayHUDMotion("anm_bore_w_gl", TRUE, NULL, GetState());
				return true;
			}
		}
	}
	else
		return inherited::TryPlayAnimBore();

	return false;
}

void CWeaponMagazinedWGrenade::UpdateSoundsPositionsImpl()
{
	inherited::UpdateSoundsPositionsImpl();
	auto& P = get_LastFP();
	m_sounds.SetPosition("sndShotG", P);
	m_sounds.SetPosition("sndReloadG", P);
	m_sounds.SetPosition("sndSwitch", P);
}

void CWeaponMagazinedWGrenade::UpdateGrenadeVisibility(bool visibility)
{
	if (!GetHUDmode()) return;
	HudItemData()->set_bone_visible("grenade", visibility, TRUE);
}

void CWeaponMagazinedWGrenade::save(NET_Packet& output_packet)
{
	inherited::save(output_packet);
	save_data(m_bGrenadeMode, output_packet);
	save_data(m_magazine2.size(), output_packet);
}

void CWeaponMagazinedWGrenade::load(IReader& input_packet)
{
	inherited::load(input_packet);
	bool b;
	load_data(b, input_packet);
	if (b != m_bGrenadeMode)
		SwitchMode(true);

	if (b && !m_bGrenadeMode) {
		Msg("[%s] ERROR: CWeaponMagazinedWGrenade::load: m_bGrenadeMode = %d, failed to switch to grenade mode", Name(), m_bGrenadeMode);
		return;
	}

	u32 sz;
	load_data(sz, input_packet);

	CCartridge l_cartridge;
	l_cartridge.Load(m_ammoTypes2[m_ammoType2].c_str(), m_ammoType2);

	while (sz > m_magazine2.size())
		m_magazine2.push_back(l_cartridge);
}

void CWeaponMagazinedWGrenade::net_Export(NET_Packet& P)
{
	P.w_u8(m_bGrenadeMode ? 1 : 0);

	inherited::net_Export(P);
}

void CWeaponMagazinedWGrenade::net_Import(NET_Packet& P)
{
	bool NewMode = FALSE;
	NewMode = !!P.r_u8();
	if (NewMode != m_bGrenadeMode)
		SwitchMode();

	inherited::net_Import(P);
}

float CWeaponMagazinedWGrenade::Weight() const
{
	float res = inherited::Weight();
	res += GetMagazineWeight(m_magazine2);

	return res;
}

bool CWeaponMagazinedWGrenade::IsNecessaryItem(const shared_str& item_sect)
{
	return (std::find(m_ammoTypes.begin(), m_ammoTypes.end(), item_sect) != m_ammoTypes.end() ||
		std::find(m_ammoTypes2.begin(), m_ammoTypes2.end(), item_sect) != m_ammoTypes2.end()
	);
}

u8 CWeaponMagazinedWGrenade::GetCurrentHudOffsetIdx()
{
	bool b_aiming = ((IsZoomed() && m_zoom_params.m_fZoomRotationFactor <= 1.f) ||
		(!IsZoomed() && m_zoom_params.m_fZoomRotationFactor > 0.f));

	if (Actor()->is_safemode())
		return 4;
	else if (!IsZoomed())
		return 0;
	else if (m_bGrenadeMode)
		return 2;
	else if (m_zoomtype == 1)
		return 3;
	else
		return 1;
}

bool CWeaponMagazinedWGrenade::install_upgrade_ammo_class(LPCSTR section, bool test)
{
	LPCSTR str;

	bool result = process_if_exists(section, "ammo_mag_size", &CInifile::r_s32, iMagazineSize2, test);
	iMagazineSize = m_bGrenadeMode ? 1 : iMagazineSize2;

	//	ammo_class = ammo_5.45x39_fmj, ammo_5.45x39_ap  // name of the ltx-section of used ammo
	bool result2 = process_if_exists_set(section, "ammo_class", &CInifile::r_string, str, test);
	if (result2 && !test)
	{
		xr_vector<shared_str>& ammo_types = m_bGrenadeMode ? m_ammoTypes2 : m_ammoTypes;
		ammo_types.clear();
		for (int i = 0, count = _GetItemCount(str); i < count; ++i)
		{
			string128 ammo_item;
			_GetItem(str, i, ammo_item);
			ammo_types.push_back(ammo_item);
		}

		m_ammoType = 0;
		m_ammoType2 = 0;
	}
	result |= result2;

	return result2;
}

bool CWeaponMagazinedWGrenade::install_upgrade_impl(LPCSTR section, bool test)
{
	LPCSTR str;
	bool result = inherited::install_upgrade_impl(section, test);

	//	grenade_class = ammo_vog-25, ammo_vog-25p          // name of the ltx-section of used grenades
	bool result2 = process_if_exists_set(section, "grenade_class", &CInifile::r_string, str, test);
	if (result2 && !test)
	{
		xr_vector<shared_str>& ammo_types = !m_bGrenadeMode ? m_ammoTypes2 : m_ammoTypes;
		ammo_types.clear();
		for (int i = 0, count = _GetItemCount(str); i < count; ++i)
		{
			string128 ammo_item;
			_GetItem(str, i, ammo_item);
			ammo_types.push_back(ammo_item);
		}

		m_ammoType = 0;
		m_ammoType2 = 0;
	}
	result |= result2;

	result |= process_if_exists(section, "launch_speed", &CInifile::r_float, m_fLaunchSpeed, test);

	result2 = process_if_exists_set(section, "snd_shoot_grenade", &CInifile::r_string, str, test);
	if (result2 && !test)
	{
		m_sounds.LoadSound(section, "snd_shoot_grenade", "sndShotG", false, m_eSoundShot);
	}
	result |= result2;

	result2 = process_if_exists_set(section, "snd_reload_grenade", &CInifile::r_string, str, test);
	if (result2 && !test)
	{
		m_sounds.LoadSound(section, "snd_reload_grenade", "sndReloadG", true, m_eSoundReload);
	}
	result |= result2;

	result2 = process_if_exists_set(section, "snd_switch", &CInifile::r_string, str, test);
	if (result2 && !test)
	{
		m_sounds.LoadSound(section, "snd_switch", "sndSwitch", true, m_eSoundReload);
	}
	result |= result2;

	return result;
}

void CWeaponMagazinedWGrenade::net_Spawn_install_upgrades(Upgrades_type saved_upgrades)
{
	// do not delete this
	// this is intended behaviour
}

#include "string_table.h"

bool CWeaponMagazinedWGrenade::GetBriefInfo(II_BriefInfo& info)
{
	VERIFY(m_pInventory);
	/*
	    if(!inherited::GetBriefInfo(info))
	    return false;
	    */
	string32 int_str;
	int ae = GetAmmoElapsed();
	xr_sprintf(int_str, "%d", ae);
	info.cur_ammo._set(int_str);

	if (bHasBulletsToHide && !m_bGrenadeMode)
	{
		last_hide_bullet = ae >= bullet_cnt ? bullet_cnt : bullet_cnt - ae - 1;
		if (ae == 0) last_hide_bullet = -1;
	}

	if (HasFireModes())
	{
		if (m_iQueueSize == WEAPON_ININITE_QUEUE)
			info.fire_mode._set("A");
		else
		{
			xr_sprintf(int_str, "%d", m_iQueueSize);
			info.fire_mode._set(int_str);
		}
	}
	if (m_pInventory->ModifyFrame() <= m_BriefInfo_CalcFrame)
		return false;

	GetSuitableAmmoTotal();

	u32 at_size = m_bGrenadeMode ? m_ammoTypes2.size() : m_ammoTypes.size();
	if (unlimited_ammo() || at_size == 0)
	{
		info.fmj_ammo._set("--");
		info.ap_ammo._set("--");
		info.third_ammo._set("--"); //Alundaio
	}
	else
	{
		//Alundaio: Added third ammo type and cleanup
		info.fmj_ammo._set("");
		info.ap_ammo._set("");
		info.third_ammo._set("");

		u8 ammo_type = m_bGrenadeMode ? m_ammoType2 : m_ammoType;
		xr_sprintf(int_str, "%d", m_bGrenadeMode ? GetAmmoCount2(ammo_type) : GetAmmoCount(ammo_type));

		if (m_ammoType == 0)
			info.fmj_ammo._set(int_str);
		else if (m_ammoType == 1)
			info.ap_ammo._set(int_str);
		else
			info.third_ammo._set(int_str);

		//Alundaio: Added third ammo type and cleanup
		info.fmj_ammo._set("");
		info.ap_ammo._set("");
		info.third_ammo._set("");

		if (at_size >= 1)
		{
			xr_sprintf(int_str, "%d", m_bGrenadeMode ? GetAmmoCount2(0) : GetAmmoCount(0));
			info.fmj_ammo._set(int_str);
		}
		if (at_size >= 2)
		{
			xr_sprintf(int_str, "%d", m_bGrenadeMode ? GetAmmoCount2(1) : GetAmmoCount(1));
			info.ap_ammo._set(int_str);
		}
		if (at_size >= 3)
		{
			xr_sprintf(int_str, "%d", m_bGrenadeMode ? GetAmmoCount2(2) : GetAmmoCount(2));
			info.third_ammo._set(int_str);
		}
		//-Alundaio
	}

	if (ae != 0 && m_magazine.size() != 0)
	{
		LPCSTR ammo_type = m_ammoTypes[m_magazine.back().m_LocalAmmoType].c_str();
		info.name._set(CStringTable().translate(pSettings->r_string(ammo_type, "inv_name_short")));
		info.icon._set(ammo_type);
	}
	else
	{
		LPCSTR ammo_type = m_ammoTypes[m_ammoType].c_str();
		info.name._set(CStringTable().translate(pSettings->r_string(ammo_type, "inv_name_short")));
		info.icon._set(ammo_type);
	}

	if (!IsGrenadeLauncherAttached())
	{
		info.grenade = "";
		return false;
	}

	int total2 = m_bGrenadeMode ? GetAmmoCount(0) : GetAmmoCount2(0);
	if (unlimited_ammo())
		xr_sprintf(int_str, "--");
	else
	{
		if (total2)
			xr_sprintf(int_str, "%d", total2);
		else
			xr_sprintf(int_str, "X");
	}
	info.grenade = int_str;

	return true;
}

int CWeaponMagazinedWGrenade::GetAmmoCount2(u8 ammo2_type) const
{
	VERIFY(m_pInventory);
	R_ASSERT(ammo2_type < m_ammoTypes2.size());

	return GetAmmoCount_forType(m_ammoTypes2[ammo2_type]);
}

#ifdef CROCKETLAUNCHER_CHANGE
void CWeaponMagazinedWGrenade::UnloadRocket()
{
	while (getRocketCount() > 0)
	{
		NET_Packet P;
		u_EventGen(P, GE_OWNERSHIP_REJECT, ID());
		P.w_u16(u16(getCurrentRocket()->ID()));
		u_EventSend(P);
        dropCurrentRocket();
	}
}
#endif
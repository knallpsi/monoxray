#include "pch_script.h"
#include "gamepersistent.h"
#include "../xrEngine/fmesh.h"
#include "../xrEngine/xr_ioconsole.h"
#include "../xrEngine/gamemtllib.h"
#include "../Include/xrRender/Kinematics.h"
#include "profiler.h"
#include "MainMenu.h"
#include "script_wallmarks_manager.h"
#include "UICursor.h"
#include "game_base_space.h"
#include "level.h"
#include "ParticlesObject.h"
#include "game_base_space.h"
#include "stalker_animation_data_storage.h"
#include "stalker_velocity_holder.h"

#include "ActorEffector.h"
#include "actor.h"
#include "spectator.h"

#include "UI/UItextureMaster.h"

#include "../xrEngine/xrSASH.h"
#include "ai_space.h"
#include "../xrServerEntities/script_engine.h"

#include "holder_custom.h"
#include "game_cl_base.h"
#include "xrserver_objects_alife_monsters.h"
#include "../xrServerEntities/xrServer_Object_Base.h"
#include "UI/UIGameTutorial.h"

#include "../xrEngine/xr_input.h"

#ifndef MASTER_GOLD
#	include "custommonster.h"
#endif // MASTER_GOLD

#ifndef _EDITOR
#	include "ai_debug.h"
#endif // _EDITOR
#include "gametype_chooser.h"

//#ifdef DEBUG_MEMORY_MANAGER
//	static	void *	ode_alloc	(size_t size)								{ return Memory.mem_alloc(size,"ODE");			}
//	static	void *	ode_realloc	(void *ptr, size_t oldsize, size_t newsize)	{ return Memory.mem_realloc(ptr,newsize,"ODE");	}
//	static	void	ode_free	(void *ptr, size_t size)					{ return xr_free(ptr);							}
//#else // DEBUG_MEMORY_MANAGER
//	static	void *	ode_alloc	(size_t size)								{ return xr_malloc(size);			}
//	static	void *	ode_realloc	(void *ptr, size_t oldsize, size_t newsize)	{ return xr_realloc(ptr,newsize);	}
//	static	void	ode_free	(void *ptr, size_t size)					{ return xr_free(ptr);				}
//#endif // DEBUG_MEMORY_MANAGER

CGamePersistent::CGamePersistent(void)
{
	m_bPickableDOF = false;
	m_game_params.m_e_game_type = eGameIDNoGame;
	ambient_effect_next_time = 0;
	ambient_effect_stop_time = 0;
	ambient_particles = 0;

	ambient_effect_wind_start = 0.f;
	ambient_effect_wind_in_time = 0.f;
	ambient_effect_wind_end = 0.f;
	ambient_effect_wind_out_time = 0.f;
	ambient_effect_wind_on = false;

	ZeroMemory(ambient_sound_next_time, sizeof(ambient_sound_next_time));


	m_pUI_core = NULL;
	m_pMainMenu = NULL;
	m_intro = NULL;
	m_intro_event.bind(this, &CGamePersistent::start_logo_intro);
#ifdef DEBUG
    m_frame_counter = 0;
    m_last_stats_frame = u32(-2);
#endif
	// 
	//dSetAllocHandler			(ode_alloc		);
	//dSetReallocHandler			(ode_realloc	);
	//dSetFreeHandler				(ode_free		);

	// 
	BOOL bDemoMode = (0 != strstr(Core.Params, "-demomode "));
	if (bDemoMode)
	{
		string256 fname;
		LPCSTR name = strstr(Core.Params, "-demomode ") + 10;
		sscanf(name, "%s", fname);
		R_ASSERT2(fname[0], "Missing filename for 'demomode'");
		Msg("- playing in demo mode '%s'", fname);
		pDemoFile = FS.r_open(fname);
		Device.seqFrame.Add(this);
		eDemoStart = Engine.Event.Handler_Attach("GAME:demo", this);
		uTime2Change = 0;
	}
	else
	{
		pDemoFile = NULL;
		eDemoStart = NULL;
	}

	eQuickLoad = Engine.Event.Handler_Attach("Game:QuickLoad", this);
	Fvector3* DofValue = Console->GetFVectorPtr("r2_dof");
	SetBaseDof(*DofValue);

	m_pWallmarksManager = nullptr;
}

CGamePersistent::~CGamePersistent(void)
{
	FS.r_close(pDemoFile);
	Device.seqFrame.Remove(this);
	Engine.Event.Handler_Detach(eDemoStart, this);
	Engine.Event.Handler_Detach(eQuickLoad, this);
}

void CGamePersistent::RegisterModel(IRenderVisual* V)
{
	// Check types
	switch (V->getType())
	{
	case MT_SKELETON_ANIM:
	case MT_SKELETON_RIGID:
		{
			u16 def_idx = GMLib.GetMaterialIdx("default_object");
			R_ASSERT2(GMLib.GetMaterialByIdx(def_idx)->Flags.is(SGameMtl::flDynamic),
			          "'default_object' - must be dynamic");
			IKinematics* K = smart_cast<IKinematics*>(V);
			VERIFY(K);
			int cnt = K->LL_BoneCount();
			for (u16 k = 0; k < cnt; k++)
			{
				CBoneData& bd = K->LL_GetData(k);
				if (*(bd.game_mtl_name))
				{
					bd.game_mtl_idx = GMLib.GetMaterialIdx(*bd.game_mtl_name);
					R_ASSERT2(GMLib.GetMaterialByIdx(bd.game_mtl_idx)->Flags.is(SGameMtl::flDynamic),
					          "Required dynamic game material");
				}
				else
				{
					bd.game_mtl_idx = def_idx;
				}
			}
		}
		break;
	}
}

extern void clean_game_globals();
extern void init_game_globals();

void CGamePersistent::OnAppStart()
{
	// load game materials
	GMLib.Load();
	init_game_globals();
	__super::OnAppStart();
	m_pUI_core = xr_new<ui_core>();
	m_pMainMenu = xr_new<CMainMenu>();
	m_pWallmarksManager = xr_new<ScriptWallmarksManager>();
}


void CGamePersistent::OnAppEnd()
{
	if (m_pMainMenu->IsActive())
		m_pMainMenu->Activate(false);

	xr_delete(m_pMainMenu);
	xr_delete(m_pUI_core);
	xr_delete(m_pWallmarksManager);

	__super::OnAppEnd();

	clean_game_globals();

	GMLib.Unload();
}

void CGamePersistent::Start(LPCSTR op)
{
	__super::Start(op);
}

void CGamePersistent::Disconnect()
{
	// destroy ambient particles
	CParticlesObject::Destroy(ambient_particles);

	__super::Disconnect();
	// stop all played emitters
	::Sound->stop_emitters();
	m_game_params.m_e_game_type = eGameIDNoGame;
}

#include "xr_level_controller.h"

void CGamePersistent::OnGameStart()
{
	__super::OnGameStart();
	UpdateGameType();
}

LPCSTR GameTypeToString(EGameIDs gt, bool bShort)
{
	switch (gt)
	{
	case eGameIDSingle:
		return "single";
		break;
	case eGameIDDeathmatch:
		return (bShort) ? "dm" : "deathmatch";
		break;
	case eGameIDTeamDeathmatch:
		return (bShort) ? "tdm" : "teamdeathmatch";
		break;
	case eGameIDArtefactHunt:
		return (bShort) ? "ah" : "artefacthunt";
		break;
	case eGameIDCaptureTheArtefact:
		return (bShort) ? "cta" : "capturetheartefact";
		break;
	case eGameIDDominationZone:
		return (bShort) ? "dz" : "dominationzone";
		break;
	case eGameIDTeamDominationZone:
		return (bShort) ? "tdz" : "teamdominationzone";
		break;
	default:
		return "---";
	}
}

EGameIDs ParseStringToGameType(LPCSTR str)
{
	if (!xr_strcmp(str, "single"))
		return eGameIDSingle;
	else if (!xr_strcmp(str, "deathmatch") || !xr_strcmp(str, "dm"))
		return eGameIDDeathmatch;
	else if (!xr_strcmp(str, "teamdeathmatch") || !xr_strcmp(str, "tdm"))
		return eGameIDTeamDeathmatch;
	else if (!xr_strcmp(str, "artefacthunt") || !xr_strcmp(str, "ah"))
		return eGameIDArtefactHunt;
	else if (!xr_strcmp(str, "capturetheartefact") || !xr_strcmp(str, "cta"))
		return eGameIDCaptureTheArtefact;
	else if (!xr_strcmp(str, "dominationzone"))
		return eGameIDDominationZone;
	else if (!xr_strcmp(str, "teamdominationzone"))
		return eGameIDTeamDominationZone;
	else
		return eGameIDNoGame; //EGameIDs
}

void CGamePersistent::UpdateGameType()
{
	__super::UpdateGameType();

	m_game_params.m_e_game_type = ParseStringToGameType(m_game_params.m_game_type);


	if (m_game_params.m_e_game_type == eGameIDSingle)
		g_current_keygroup = _sp;
	else
		g_current_keygroup = _mp;
}

void CGamePersistent::OnGameEnd()
{
	__super::OnGameEnd();

	xr_delete(g_stalker_animation_data_storage);
	xr_delete(g_stalker_velocity_holder);
}

void CGamePersistent::WeathersUpdate()
{
	if (g_pGameLevel && !g_dedicated_server)
	{
		CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
		BOOL bIndoor = TRUE;
		if (actor) bIndoor = actor->renderable_ROS()->get_luminocity_hemi() < 0.05f;

		int data_set = (Random.randF() < (1.f - Environment().CurrentEnv->weight)) ? 0 : 1;

		CEnvDescriptor* const _env = Environment().Current[data_set];
		VERIFY(_env);

		CEnvAmbient* env_amb = Environment().m_paused ? Environment().CurrentEnv->env_ambient : _env->env_ambient;
		if (env_amb)
		{
			CEnvAmbient::SSndChannelVec& vec = env_amb->get_snd_channels();
			CEnvAmbient::SSndChannelVecIt I = vec.begin();
			CEnvAmbient::SSndChannelVecIt E = vec.end();

			for (u32 idx = 0; I != E; ++I, ++idx)
			{
				CEnvAmbient::SSndChannel& ch = **I;
				R_ASSERT(idx<32);
				if (ambient_sound_next_time[idx] == 0) //first
				{
					ambient_sound_next_time[idx] = Device.dwTimeGlobal + ch.get_rnd_sound_first_time();
				}
				else if (Device.dwTimeGlobal > ambient_sound_next_time[idx])
				{
					ref_sound& snd = ch.get_rnd_sound();

					Fvector pos;
					float angle = ::Random.randF(PI_MUL_2);
					pos.x = _cos(angle);
					pos.y = 0;
					pos.z = _sin(angle);
					pos.normalize().mul(ch.get_rnd_sound_dist()).add(Device.vCameraPosition);
					pos.y += 10.f;
					snd.play_at_pos(0, pos);

#ifdef DEBUG
                    if (!snd._handle() && strstr(Core.Params, "-nosound"))
                        continue;
#endif // DEBUG

					VERIFY(snd._handle());
					u32 _length_ms = iFloor(snd.get_length_sec() * 1000.0f);
					ambient_sound_next_time[idx] = Device.dwTimeGlobal + _length_ms + ch.get_rnd_sound_time();
					//					Msg("- Playing ambient sound channel [%s] file[%s]",ch.m_load_section.c_str(),snd._handle()->file_name());
				}
			}
			/*
			            if (Device.dwTimeGlobal > ambient_sound_next_time)
			            {
			            ref_sound* snd			= env_amb->get_rnd_sound();
			            ambient_sound_next_time	= Device.dwTimeGlobal + env_amb->get_rnd_sound_time();
			            if (snd)
			            {
			            Fvector	pos;
			            float	angle		= ::Random.randF(PI_MUL_2);
			            pos.x				= _cos(angle);
			            pos.y				= 0;
			            pos.z				= _sin(angle);
			            pos.normalize		().mul(env_amb->get_rnd_sound_dist()).add(Device.vCameraPosition);
			            pos.y				+= 10.f;
			            snd->play_at_pos	(0,pos);
			            }
			            }
			            */
			// start effect
			if ((FALSE == bIndoor) && (0 == ambient_particles) && Device.dwTimeGlobal > ambient_effect_next_time)
			{
				CEnvAmbient::SEffect* eff = env_amb->get_rnd_effect();
				if (eff)
				{
					Environment().wind_gust_factor = eff->wind_gust_factor;
					ambient_effect_next_time = Device.dwTimeGlobal + env_amb->get_rnd_effect_time();
					ambient_effect_stop_time = Device.dwTimeGlobal + eff->life_time;
					ambient_effect_wind_start = Device.fTimeGlobal;
					ambient_effect_wind_in_time = Device.fTimeGlobal + eff->wind_blast_in_time;
					ambient_effect_wind_end = Device.fTimeGlobal + eff->life_time / 1000.f;
					ambient_effect_wind_out_time = Device.fTimeGlobal + eff->life_time / 1000.f + eff->
						wind_blast_out_time;
					ambient_effect_wind_on = true;

					ambient_particles = CParticlesObject::Create(eff->particles.c_str(), FALSE, false);
					Fvector pos;
					pos.add(Device.vCameraPosition, eff->offset);
					ambient_particles->play_at_pos(pos);
					if (eff->sound._handle()) eff->sound.play_at_pos(0, pos);


					Environment().wind_blast_strength_start_value = Environment().wind_strength_factor;
					Environment().wind_blast_strength_stop_value = eff->wind_blast_strength;

					if (Environment().wind_blast_strength_start_value == 0.f)
					{
						Environment().wind_blast_start_time.set(0.f, eff->wind_blast_direction.x,
						                                        eff->wind_blast_direction.y,
						                                        eff->wind_blast_direction.z);
					}
					else
					{
						Environment().wind_blast_start_time.set(0.f, Environment().wind_blast_direction.x,
						                                        Environment().wind_blast_direction.y,
						                                        Environment().wind_blast_direction.z);
					}
					Environment().wind_blast_stop_time.set(0.f, eff->wind_blast_direction.x,
					                                       eff->wind_blast_direction.y, eff->wind_blast_direction.z);
				}
			}
		}
		if (Device.fTimeGlobal >= ambient_effect_wind_start && Device.fTimeGlobal <= ambient_effect_wind_in_time &&
			ambient_effect_wind_on)
		{
			float delta = ambient_effect_wind_in_time - ambient_effect_wind_start;
			float t;
			if (delta != 0.f)
			{
				float cur_in = Device.fTimeGlobal - ambient_effect_wind_start;
				t = cur_in / delta;
			}
			else
			{
				t = 0.f;
			}
			Environment().wind_blast_current.slerp(Environment().wind_blast_start_time,
			                                       Environment().wind_blast_stop_time, t);

			Environment().wind_blast_direction.set(Environment().wind_blast_current.x,
			                                       Environment().wind_blast_current.y,
			                                       Environment().wind_blast_current.z);
			Environment().wind_strength_factor = Environment().wind_blast_strength_start_value + t * (Environment().
				wind_blast_strength_stop_value - Environment().wind_blast_strength_start_value);
		}

		// stop if time exceed or indoor
		if (bIndoor || Device.dwTimeGlobal >= ambient_effect_stop_time)
		{
			if (ambient_particles) ambient_particles->Stop();

			Environment().wind_gust_factor = 0.f;
		}

		if (Device.fTimeGlobal >= ambient_effect_wind_end && ambient_effect_wind_on)
		{
			Environment().wind_blast_strength_start_value = Environment().wind_strength_factor;
			Environment().wind_blast_strength_stop_value = 0.f;

			ambient_effect_wind_on = false;
		}

		if (Device.fTimeGlobal >= ambient_effect_wind_end && Device.fTimeGlobal <= ambient_effect_wind_out_time)
		{
			float delta = ambient_effect_wind_out_time - ambient_effect_wind_end;
			float t;
			if (delta != 0.f)
			{
				float cur_in = Device.fTimeGlobal - ambient_effect_wind_end;
				t = cur_in / delta;
			}
			else
			{
				t = 0.f;
			}
			Environment().wind_strength_factor = Environment().wind_blast_strength_start_value + t * (Environment().
				wind_blast_strength_stop_value - Environment().wind_blast_strength_start_value);
		}
		if (Device.fTimeGlobal > ambient_effect_wind_out_time && ambient_effect_wind_out_time != 0.f)
		{
			Environment().wind_strength_factor = 0.0;
		}

		// if particles not playing - destroy
		if (ambient_particles && !ambient_particles->IsPlaying())
			CParticlesObject::Destroy(ambient_particles);
	}
}

bool allow_intro()
{
#ifdef MASTER_GOLD
	if (g_SASH.IsRunning())
#else	// #ifdef MASTER_GOLD
    if ((0 != strstr(Core.Params, "-nointro")) || g_SASH.IsRunning())
#endif	// #ifdef MASTER_GOLD
	{
		return false;
	}
	else
		return true;
}

bool allow_logo() // AVO: skip NVIDIA and other logos at load time
{
	if (0 != strstr(Core.Params, "-skiplogo"))
	{
		return false;
	}
	else
	{
		return true;
	}
}

void CGamePersistent::start_logo_intro()
{
	if (Device.dwPrecacheFrame == 0)
	{
		m_intro_event.bind(this, &CGamePersistent::update_logo_intro);
		if (!g_dedicated_server && 0 == xr_strlen(m_game_params.m_game_or_spawn) && NULL == g_pGameLevel)
		{
			VERIFY(NULL == m_intro);
			m_intro = xr_new<CUISequencer>();
			if (allow_logo()) // AVO: skip NVIDIA and other logos at load time
			{
				m_intro->Start("intro_logo");
				Msg("intro_start intro_logo");
			}
			Console->Hide();
		}
	}
}

void CGamePersistent::update_logo_intro()
{
	if (m_intro && (false == m_intro->IsActive()))
	{
		m_intro_event = 0;
		xr_delete(m_intro);
		Msg("intro_delete ::update_logo_intro");
		Console->Execute("main_menu on");
	}
	else if (!m_intro)
	{
		m_intro_event = 0;
	}
}

void CGamePersistent::game_loaded()
{
	if (Device.dwPrecacheFrame <= 2)
	{
		if (g_pGameLevel &&
			g_pGameLevel->bReady &&
			(allow_intro() && psDeviceFlags2.test(rsKeypress)) &&
			load_screen_renderer.b_need_user_input &&
			m_game_params.m_e_game_type == eGameIDSingle)
		{
			VERIFY(NULL == m_intro);
			m_intro = xr_new<CUISequencer>();
			m_intro->Start("game_loaded");
			Msg("intro_start game_loaded");
			m_intro->m_on_destroy_event.bind(this, &CGamePersistent::update_game_loaded);

			// demonized: Reset mouse state on loading the game
			pInput->resetMouseState();

			// demonized
			// Callback for when loading screen happens and "Press Any Key to Continue" prompt appears
			::luabind::functor<void> funct;
			if (ai().script_engine().functor("_G.OnLoadingScreenKeyPrompt", funct))
			{
				funct();
			}
		}

		else if (	g_pGameLevel &&
					g_pGameLevel->bReady &&
					m_game_params.m_e_game_type == eGameIDSingle
				 )
		{
			Msg("intro_start game_loaded");

			::luabind::functor<void> funct;
			if (ai().script_engine().functor("_G.OnLoadingScreenKeyPrompt", funct))
			{
				funct();
			}
		}

		m_intro_event = 0;
	}
}

namespace crash_saving {
	extern void (*save_impl)();
	extern void _save_impl();
}

void CGamePersistent::update_game_loaded()
{
	xr_delete(m_intro);
	Msg("intro_delete ::update_game_loaded");
	start_game_intro();

	// demonized
	// Enable crash saving here
	crash_saving::save_impl = &crash_saving::_save_impl;

	// Callback for when player dismisses loading screen after "Press Any Key to Continue" pressed
	::luabind::functor<void> funct;
	if (ai().script_engine().functor("_G.OnLoadingScreenDismissed", funct))
	{
		funct();
	}
}

void CGamePersistent::start_game_intro()
{
	if (!allow_intro())
	{
		m_intro_event = 0;
		return;
	}

	if (g_pGameLevel && g_pGameLevel->bReady && Device.dwPrecacheFrame <= 2)
	{
		m_intro_event.bind(this, &CGamePersistent::update_game_intro);
		if (0 == stricmp(m_game_params.m_new_or_load, "new"))
		{
			VERIFY(NULL == m_intro);
			m_intro = xr_new<CUISequencer>();
			m_intro->Start("intro_game");
			Msg("intro_start intro_game");
		}
	}
}

void CGamePersistent::update_game_intro()
{
	if (m_intro && (false == m_intro->IsActive()))
	{
		xr_delete(m_intro);
		Msg("intro_delete ::update_game_intro");
		m_intro_event = 0;
	}
	else if (!m_intro)
	{
		m_intro_event = 0;
	}
}

extern CUISequencer* g_tutorial;
extern CUISequencer* g_tutorial2;

void CGamePersistent::OnFrame()
{
	if (Device.dwPrecacheFrame == 5 && m_intro_event.empty())
	{
		m_intro_event.bind(this, &CGamePersistent::game_loaded);
	}

	if (g_tutorial2)
	{
		g_tutorial2->Destroy();
		xr_delete(g_tutorial2);
	}

	if (g_tutorial && !g_tutorial->IsActive())
	{
		xr_delete(g_tutorial);
	}
	if (0 == Device.dwFrame % 200)
		CUITextureMaster::FreeCachedShaders();

#ifdef DEBUG
    ++m_frame_counter;
#endif
	if (!g_dedicated_server && !m_intro_event.empty()) m_intro_event();

	if (!g_dedicated_server && Device.dwPrecacheFrame == 0 && !m_intro && m_intro_event.empty())
		load_screen_renderer.stop();

	if (!m_pMainMenu->IsActive())
		m_pMainMenu->DestroyInternal(false);

	if (!g_pGameLevel) return;
	if (!g_pGameLevel->bReady) return;

	if (Device.Paused())
	{
		if (Level().IsDemoPlay())
		{
			CSpectator* tmp_spectr = smart_cast<CSpectator*>(Level().CurrentControlEntity());
			if (tmp_spectr)
			{
				tmp_spectr->UpdateCL(); //updating spectator in pause (pause ability of demo play)
			}
		}
#ifndef MASTER_GOLD
        if (Level().CurrentViewEntity() && IsGameTypeSingle())
        {
            if (!g_actor || (g_actor->ID() != Level().CurrentViewEntity()->ID()))
            {
                CCustomMonster	*custom_monster = smart_cast<CCustomMonster*>(Level().CurrentViewEntity());
                if (custom_monster) // can be spectator in multiplayer
                    custom_monster->UpdateCamera();
            }
            else
            {
                CCameraBase* C = NULL;
                if (g_actor)
                {
                    if (!Actor()->Holder())
                        C = Actor()->cam_Active();
                    else
                        C = Actor()->Holder()->Camera();

                    Actor()->Cameras().UpdateFromCamera(C);
                    Actor()->Cameras().ApplyDevice(VIEWPORT_NEAR);
#ifdef DEBUG
                    if (psActorFlags.test(AF_NO_CLIP))
                    {
                        Actor()->dbg_update_cl = 0;
                        Actor()->dbg_update_shedule = 0;
                        Device.dwTimeDelta = 0;
                        Device.fTimeDelta = 0.01f;
                        Actor()->UpdateCL();
                        Actor()->shedule_Update(0);
                        Actor()->dbg_update_cl = 0;
                        Actor()->dbg_update_shedule = 0;

                        CSE_Abstract* e = Level().Server->ID_to_entity(Actor()->ID());
                        VERIFY(e);
                        CSE_ALifeCreatureActor*	s_actor = smart_cast<CSE_ALifeCreatureActor*>(e);
                        VERIFY(s_actor);
                        xr_vector<u16>::iterator it = s_actor->children.begin();
                        for (; it != s_actor->children.end(); it++)
                        {
                            CObject* obj = Level().Objects.net_Find(*it);
                            if (obj && Engine.Sheduler.Registered(obj))
                            {
                                obj->dbg_update_shedule = 0;
                                obj->dbg_update_cl = 0;
                                obj->shedule_Update(0);
                                obj->UpdateCL();
                                obj->dbg_update_shedule = 0;
                                obj->dbg_update_cl = 0;
                            }
                        }
                    }
#endif // DEBUG
                }
            }
        }
#else // MASTER_GOLD
		if (g_actor && IsGameTypeSingle())
		{
			CCameraBase* C = NULL;
			if (!Actor()->Holder())
				C = Actor()->cam_Active();
			else
				C = Actor()->Holder()->Camera();

			Actor()->Cameras().UpdateFromCamera(C);
			Actor()->Cameras().ApplyDevice(VIEWPORT_NEAR);
		}
#endif // MASTER_GOLD
	}
	__super::OnFrame();

	if (!Device.Paused())
		Engine.Sheduler.Update();

	// update weathers ambient
	if (!Device.Paused())
		WeathersUpdate();

	if (0 != pDemoFile)
	{
		if (Device.dwTimeGlobal > uTime2Change)
		{
			// Change level + play demo
			if (pDemoFile->elapsed() < 3) pDemoFile->seek(0); // cycle

			// Read params
			string512 params;
			pDemoFile->r_string(params, sizeof(params));
			string256 o_server, o_client, o_demo;
			u32 o_time;
			sscanf(params, "%[^,],%[^,],%[^,],%d", o_server, o_client, o_demo, &o_time);

			// Start _new level + demo
			Engine.Event.Defer("KERNEL:disconnect");
			Engine.Event.Defer("KERNEL:start", size_t(xr_strdup(_Trim(o_server))), size_t(xr_strdup(_Trim(o_client))));
			Engine.Event.Defer("GAME:demo", size_t(xr_strdup(_Trim(o_demo))), u64(o_time));
			uTime2Change = 0xffffffff; // Block changer until Event received
		}
	}

#ifdef DEBUG
    if ((m_last_stats_frame + 1) < m_frame_counter)
        profiler().clear();
#endif
	UpdateDof();
}

void CGamePersistent::ImGui_OnRender(LPCSTR name)
{
	if (!g_pGameLevel || !g_pGameLevel->bReady) return;

	::luabind::functor<void> imgui_render;
	if (ai().script_engine().functor("callbacks_gameobject.on_imgui_render", imgui_render))
	{
		imgui_render(name);
	}
}

#include "game_sv_single.h"
#include "xrServer.h"
#include "UIGameCustom.h"
#include "ui/UIMainIngameWnd.h"
#include "ui/UIPdaWnd.h"

void CGamePersistent::OnEvent(EVENT E, u64 P1, u64 P2)
{
	if (E == eQuickLoad)
	{
		if (Device.Paused())
			Device.Pause(FALSE, TRUE, TRUE, "eQuickLoad");

		if (CurrentGameUI())
		{
			CurrentGameUI()->HideShownDialogs();
			CurrentGameUI()->UIMainIngameWnd->reset_ui();
			CurrentGameUI()->GetPdaMenu().Reset();
		}

		if (g_tutorial)
			g_tutorial->Stop();

		if (g_tutorial2)
			g_tutorial2->Stop();

		LPSTR saved_name = (LPSTR)(P1);

		Level().remove_objects();
		game_sv_Single* game = smart_cast<game_sv_Single*>(Level().Server->game);
		R_ASSERT(game);
		game->restart_simulator(saved_name);
		xr_free(saved_name);
		return;
	}
	else if (E == eDemoStart)
	{
		string256 cmd;
		LPCSTR demo = LPCSTR(P1);
		xr_sprintf(cmd, "demo_play %s", demo);
		Console->Execute(cmd);
		xr_free(demo);
		uTime2Change = Device.TimerAsync() + u32(P2) * 1000;
	}
}

void CGamePersistent::Statistics(CGameFont* F)
{
#ifdef DEBUG
#	ifndef _EDITOR
    m_last_stats_frame = m_frame_counter;
    profiler().show_stats(F, !!psAI_Flags.test(aiStats));
#	endif
#endif
}

float CGamePersistent::MtlTransparent(u32 mtl_idx)
{
	return GMLib.GetMaterialByIdx((u16)mtl_idx)->fVisTransparencyFactor;
}

static BOOL bRestorePause = FALSE;
static BOOL bEntryFlag = TRUE;

void CGamePersistent::OnAppActivate()
{
	bool bIsMP = (g_pGameLevel && Level().game && GameID() != eGameIDSingle);
	bIsMP &= !Device.Paused();

	if (!bIsMP)
	{
		Device.Pause(FALSE, !bRestorePause, TRUE, "CGP::OnAppActivate");
	}
	else
	{
		Device.Pause(FALSE, TRUE, TRUE, "CGP::OnAppActivate MP");
	}

	bEntryFlag = TRUE;
}

void CGamePersistent::OnAppDeactivate()
{
	if (!bEntryFlag) return;

	bool bIsMP = (g_pGameLevel && Level().game && GameID() != eGameIDSingle);

	bRestorePause = FALSE;

	if (!bIsMP)
	{
		bRestorePause = Device.Paused();
		Device.Pause(TRUE, TRUE, TRUE, "CGP::OnAppDeactivate");
	}
	else
	{
		Device.Pause(TRUE, FALSE, TRUE, "CGP::OnAppDeactivate MP");
	}
	bEntryFlag = FALSE;
}


bool CGamePersistent::OnRenderPPUI_query()
{
	return MainMenu()->OnRenderPPUI_query();
	// enable PP or not
}

extern void draw_wnds_rects();

void CGamePersistent::OnRenderPPUI_main()
{
	// always
	MainMenu()->OnRenderPPUI_main();
	draw_wnds_rects();
}

void CGamePersistent::OnRenderPPUI_PP()
{
	MainMenu()->OnRenderPPUI_PP();
}

#include "string_table.h"
#include "../xrEngine/x_ray.h"

void CGamePersistent::LoadTitle(bool change_tip, shared_str map_name)
{
	pApp->LoadStage();
	if (change_tip)
	{
		string512 buff;
		u8 tip_num;
		::luabind::functor<u8> m_functor;
		bool is_single = !xr_strcmp(m_game_params.m_game_type, "single");
		if (is_single)
		{
			R_ASSERT(ai().script_engine().functor("loadscreen.get_tip_number", m_functor));
			tip_num = m_functor(map_name.c_str());
		}
		else
		{
			R_ASSERT(ai().script_engine().functor("loadscreen.get_mp_tip_number", m_functor));
			tip_num = m_functor(map_name.c_str());
		}

		if (tip_num < 1)
			return;

		//		tip_num = 83;
		xr_sprintf(buff, "%s%d:", CStringTable().translate("ls_tip_number").c_str(), tip_num);
		shared_str tmp = buff;

		if (is_single)
			xr_sprintf(buff, "ls_tip_%d", tip_num);
		else
			xr_sprintf(buff, "ls_mp_tip_%d", tip_num);

		pApp->LoadTitleInt(CStringTable().translate("ls_header").c_str(), tmp.c_str(),
		                   CStringTable().translate(buff).c_str());
	}
}

bool CGamePersistent::CanBePaused()
{
	return IsGameTypeSingle() || (g_pGameLevel && Level().IsDemoPlay());
}

void CGamePersistent::SetPickableEffectorDOF(bool bSet)
{
	m_bPickableDOF = bSet;
	if (!bSet)
		RestoreEffectorDOF();
}

void CGamePersistent::GetCurrentDof(Fvector3& dof)
{
	dof = m_dof[1];
}

void CGamePersistent::SetBaseDof(const Fvector3& dof)
{
	m_dof[0] = m_dof[1] = m_dof[2] = m_dof[3] = dof;
}

void CGamePersistent::SetEffectorDOF(const Fvector& needed_dof)
{
	if (m_bPickableDOF) return;
	m_dof[0] = needed_dof;
	m_dof[2] = m_dof[1]; //current
}

void CGamePersistent::RestoreEffectorDOF()
{
	SetEffectorDOF(m_dof[3]);
}

#include "hudmanager.h"

//	m_dof		[4];	// 0-dest 1-current 2-from 3-original
void CGamePersistent::UpdateDof()
{
	static float diff_far = pSettings->r_float("zone_pick_dof", "far"); //70.0f;
	static float diff_near = pSettings->r_float("zone_pick_dof", "near"); //-70.0f;

	if (m_bPickableDOF)
	{
		Fvector pick_dof;
		pick_dof.y = HUD().GetRQ().range;
		pick_dof.x = pick_dof.y + diff_near;
		pick_dof.z = pick_dof.y + diff_far;
		m_dof[0] = pick_dof;
		m_dof[2] = m_dof[1]; //current
	}
	if (m_dof[1].similar(m_dof[0]))
		return;

	float td = Device.fTimeDelta;
	Fvector diff;
	diff.sub(m_dof[0], m_dof[2]);
	diff.mul(td / 0.2f); //0.2 sec
	m_dof[1].add(diff);
	(m_dof[0].x < m_dof[2].x) ? clamp(m_dof[1].x, m_dof[0].x, m_dof[2].x) : clamp(m_dof[1].x, m_dof[2].x, m_dof[0].x);
	(m_dof[0].y < m_dof[2].y) ? clamp(m_dof[1].y, m_dof[0].y, m_dof[2].y) : clamp(m_dof[1].y, m_dof[2].y, m_dof[0].y);
	(m_dof[0].z < m_dof[2].z) ? clamp(m_dof[1].z, m_dof[0].z, m_dof[2].z) : clamp(m_dof[1].z, m_dof[2].z, m_dof[0].z);
}

#include "ui\uimainingamewnd.h"

void CGamePersistent::OnSectorChanged(int sector)
{
	if (CurrentGameUI())
		CurrentGameUI()->UIMainIngameWnd->OnSectorChanged(sector);
}

void CGamePersistent::OnAssetsChanged()
{
	IGame_Persistent::OnAssetsChanged();
	CStringTable().rescan();
}

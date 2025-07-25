#include "pch_script.h"
#include "../xrEngine/xr_ioconsole.h"
#include "../xrEngine/xr_ioc_cmd.h"
#include "../xrEngine/customhud.h"
#include "../xrEngine/fdemorecord.h"
#include "../xrEngine/fdemoplay.h"
#include "xrMessages.h"
#include "xrserver.h"
#include "level.h"
#include "script_debugger.h"
#include "ai_debug.h"
#include "alife_simulator.h"
#include "game_cl_base.h"
#include "game_cl_single.h"
#include "game_sv_single.h"
#include "hit.h"
#include "PHDestroyable.h"
#include "actor.h"
#include "Actor_Flags.h"
#include "customzone.h"
#include "script_engine.h"
#include "script_engine_space.h"
#include "script_process.h"
#include "xrServer_Objects.h"
#include "ui/UIMainIngameWnd.h"
//#include "../xrphysics/PhysicsGamePars.h"
#include "../xrphysics/iphworld.h"
#include "string_table.h"
#include "autosave_manager.h"
#include "ai_space.h"
#include "ai/monsters/BaseMonster/base_monster.h"
#include "date_time.h"
#include "mt_config.h"
#include "ui/UIOptConCom.h"
#include "UIGameSP.h"
#include "ui/UIActorMenu.h"
#include "ui/UIStatic.h"
#include "zone_effector.h"
#include "GameTask.h"
#include "MainMenu.h"
#include "saved_game_wrapper.h"
#include "level_graph.h"
//#include "../xrEngine/resourcemanager.h"
//#include "../xrEngine/doug_lea_memory_allocator.h"
#include "cameralook.h"
#include "character_hit_animations_params.h"
#include "inventory_upgrade_manager.h"

#include "ai_debug_variables.h"
#include "../xrphysics/console_vars.h"
#include <sstream>
#include "..\xrCore\LocatorAPI.h"
#ifdef DEBUG
#	include "PHDebug.h"
#	include "ui/UIDebugFonts.h"
#	include "game_graph.h"
#	include "CharacterPhysicsSupport.h"
#endif // DEBUG


#include "..\..\xrEngine\x_ray.h"

#include "NewZoomFlag.h"
float n_zoom_step_count = 3.0f;

string_path g_last_saved_game;

#ifdef DEBUG
extern float air_resistance_epsilon;
#endif // #ifdef DEBUG

extern void show_smart_cast_stats();
extern void clear_smart_cast_stats();
extern void release_smart_cast_stats();

extern u64 g_qwStartGameTime;
extern u64 g_qwEStartGameTime;

ENGINE_API
extern float psHUD_FOV_def;
extern float psSqueezeVelocity;

// Lua
extern int psLUA_GCSTEP;
extern BOOL lua_debug;

float g_end_modif = 0.f;

extern int x_m_x;
extern int x_m_z;
extern BOOL net_cl_inputguaranteed;
extern BOOL net_sv_control_hit;
extern int g_dwInputUpdateDelta;
#ifdef DEBUG
extern	BOOL	g_ShowAnimationInfo;
#endif // DEBUG
extern BOOL g_bShowHitSectors;
//extern	BOOL	g_bDebugDumpPhysicsStep	;
extern ESingleGameDifficulty g_SingleGameDifficulty;
extern BOOL g_show_wnd_rect2;
//-----------------------------------------------------------
extern float g_fTimeFactor;
extern float f_weapon_deterioration;
extern float f_power_loss_bias;
extern float f_power_loss_factor;
//extern  BOOL	g_old_style_ui_hud;

extern float g_smart_cover_factor;
extern int g_upgrades_log;
extern float g_smart_cover_animation_speed_factor;

extern BOOL g_ai_use_old_vision;
float g_aim_predict_time = 0.40f;
float g_head_bob_factor = 1.00f;
float streff;

extern BOOL g_ai_die_in_anomaly; //Alundaio

extern BOOL g_telekinetic_objects_include_corpses; // Tosox

extern BOOL g_allow_weapon_control_inertion_factor; // momopate
extern BOOL g_allow_outfit_control_inertion_factor;
extern BOOL g_render_short_tracers;
extern BOOL g_fix_avelocity_spread;
extern BOOL g_apply_pdm_to_ads;
extern BOOL g_smooth_ads_transition;
extern BOOL g_allow_silencer_hide_tracer;

//demonized: new console vars
extern BOOL firstPersonDeath;
extern BOOL pseudogiantCanDamageObjects;
extern BOOL use_english_text_for_missing_translations;
namespace crash_saving {
	extern BOOL enabled;
	extern int saveCountMax;
}
extern BOOL pda_map_zoom_in_to_mouse;
extern BOOL pda_map_zoom_out_to_mouse;
extern BOOL mouseWheelChangeWeapon;
extern BOOL mouseWheelInvertZoom;
extern BOOL mouseWheelInvertChangeWeapons;
extern BOOL monsterStuckFix;
extern BOOL logTimestamps;
extern float f_Freelook_cam_limit;
extern int MOUSEBUFFERSIZE;
extern int KEYBOARDBUFFERSIZE;
extern BOOL print_bone_warnings;
extern BOOL print_dltx_warnings;
extern BOOL poltergeist_spawn_corpse_on_death;
extern BOOL useNewZoomDeltaAlgorithm;
extern BOOL g_aimmode_remember;
extern BOOL g_freelook_while_reloading;
extern BOOL useSeparateUBGLKeybind;
extern float g_gunsnd_indoor;
extern float g_gunsnd_indoor_volume;
extern int g_nearwall;
extern int g_nearwall_trace;
extern BOOL drawPickupItemNames;
extern BOOL fun_allowed;
extern BOOL progressiveStaminaCost;
extern BOOL NPCsLookAtActor;
extern float NPCsLookAtActorMinDistance;

extern BOOL mt_UpdateWeaponSounds;

extern BOOL alifeObjectHangingLampIgnoreMatchConfiguration;

extern BOOL spawn_antifreeze;
extern BOOL spawn_antifreeze_debug;

extern float IK_CALC_DIST;
extern float IK_ALWAYS_CALC_DIST;
extern BOOL r_optimize_calculate_bones;

extern CrosshairSettings g_crosshair_camera_near;
extern CrosshairSettings g_crosshair_camera_far;
extern CrosshairSettings g_crosshair_weapon_near;
extern CrosshairSettings g_crosshair_weapon_far;
extern CrosshairSettings g_crosshair_device_near;
extern CrosshairSettings g_crosshair_device_far;

#define Concat2(a, b) #a ## b
#define Concat3(a, b, c) #a ## b ## #c

#define CrosshairBaseCommands(crosshair, suffix) \
	CMD3(CCC_Mask, Concat2(g_crosshair_, suffix), &crosshair.flags, CROSSHAIR_SHOW); \
	CMD3(CCC_Mask, Concat3(g_crosshair_, suffix, _recon), &crosshair.flags, CROSSHAIR_RECON); \
	CMD4(CCC_Float, Concat3(g_crosshair_, suffix, _recon_max_opacity), &crosshair.recon_max_opacity, 0.f, 1.f); \
	CMD3(CCC_Mask, Concat3(g_crosshair_, suffix, _use_shader), &crosshair.flags, CROSSHAIR_USE_SHADER); \
	CMD3(CCC_String, Concat3(g_crosshair_, suffix, _shader ), crosshair.shader, 32); \
	CMD3(CCC_String, Concat3(g_crosshair_, suffix, _texture), crosshair.texture, 32); \
	CMD4(CCC_Float, Concat3(g_crosshair_, suffix, _size), &crosshair.size, 1.f, 64.f); \
	CMD4(CCC_Float, Concat3(g_crosshair_, suffix, _depth), &crosshair.depth, 0.f, 300.f); \
	CMD2(CCC_Color, Concat3(g_crosshair_, suffix, _color), &crosshair.color);

#define CrosshairDistanceCommands(crosshair, suffix) \
	CMD3(CCC_Mask, Concat3(g_crosshair_, suffix, _distance_lerp), &crosshair.flags, CROSSHAIR_DISTANCE_LERP); \
	CMD4(CCC_Float, Concat3(g_crosshair_, suffix, _distance_lerp_rate), &crosshair.distance_lerp_rate, 1.f, 100.f);

#define CrosshairOpacityCommands(crosshair, suffix) \
	CMD4(CCC_Float, Concat3(g_crosshair_, suffix, _occluded_opacity), &crosshair.occluded_opacity, 0.f, 1.f); \
	CMD4(CCC_Float, Concat3(g_crosshair_, suffix, _occlusion_fade_rate), &crosshair.occlusion_fade_rate, 1.f, 100.f);

#define CrosshairLineCommands(crosshair, suffix) \
	CMD3(CCC_Mask, Concat3(g_crosshair_, suffix, _line), &crosshair.flags, CROSSHAIR_LINE);

#define CrosshairCameraFarCommands(crosshair, suffix) \
	CrosshairBaseCommands(crosshair, suffix);

#define CrosshairCameraNearCommands(crosshair, suffix) \
	CrosshairBaseCommands(crosshair, suffix); \
	CrosshairDistanceCommands(crosshair, suffix);

#define CrosshairFarCommands(crosshair, suffix) \
	CrosshairBaseCommands(crosshair, suffix); \
	CrosshairLineCommands(crosshair, suffix)

#define CrosshairNearCommands(crosshair, suffix) \
	CrosshairBaseCommands(crosshair, suffix); \
	CrosshairDistanceCommands(crosshair, suffix); \
	CrosshairOpacityCommands(crosshair, suffix); \
	CrosshairLineCommands(crosshair, suffix)

extern float recon_show_speed;
extern float recon_hide_speed;
extern float recon_mindist;
extern float recon_maxdist;
extern float recon_minspeed;
extern float recon_maxspeed;

extern float wallmark_range_static;
extern float wallmark_range_skeleton;

ENGINE_API extern float g_console_sensitive;

u32 g_dead_body_collision = 1;

xr_token dead_body_collision_tokens[] =
{
	{ "full", 2 },
	{ "actor_only", 1 },
	{ "off", 0 },
	{ 0, 0 }
};

void register_mp_console_commands();
//-----------------------------------------------------------

BOOL g_bCheckTime = FALSE;
int net_cl_inputupdaterate = 50;
Flags32 g_mt_config = {
	mtLevelPath | mtDetailPath | mtObjectHandler | mtSoundPlayer | mtAiVision | mtBullets | mtLUA_GC | mtLevelSounds |
	mtALife | mtMap
};
#ifdef DEBUG
Flags32	dbg_net_Draw_Flags = { 0 };
#endif

#ifdef DEBUG
BOOL	g_bDebugNode = FALSE;
u32		g_dwDebugNodeSource = 0;
u32		g_dwDebugNodeDest = 0;
extern	BOOL	g_bDrawBulletHit;
extern	BOOL	g_bDrawFirstBulletCrosshair;

float	debug_on_frame_gather_stats_frequency = 0.f;
#endif
#ifdef DEBUG
extern LPSTR	dbg_stalker_death_anim;
extern BOOL		b_death_anim_velocity;
extern BOOL		death_anim_debug;
extern BOOL		dbg_imotion_draw_skeleton;
extern BOOL		dbg_imotion_draw_velocity;
extern BOOL		dbg_imotion_collide_debug;
extern float	dbg_imotion_draw_velocity_scale;
#endif
int g_AI_inactive_time = 0;
Flags32 g_uCommonFlags;

enum E_COMMON_FLAGS
{
	flAiUseTorchDynamicLights = 1
};

CUIOptConCom g_OptConCom;

#ifndef PURE_ALLOC
//#	ifndef USE_MEMORY_MONITOR
//#		define SEVERAL_ALLOCATORS
//#	endif // USE_MEMORY_MONITOR
#endif // PURE_ALLOC

#ifdef SEVERAL_ALLOCATORS
extern		u32 game_lua_memory_usage();
#endif // SEVERAL_ALLOCATORS

typedef void (*full_memory_stats_callback_type)();
extern XRCORE_API full_memory_stats_callback_type g_full_memory_stats_callback;

static void full_memory_stats()
{
	Msg("* [x-ray]: Full Memory Stats");
	Memory.mem_compact();
	size_t _process_heap = ::Memory.mem_usage();
#ifdef SEVERAL_ALLOCATORS
	u32		_game_lua = game_lua_memory_usage();
	u32		_render = ::Render->memory_usage();
#endif // SEVERAL_ALLOCATORS
	int _eco_strings = (int)g_pStringContainer->stat_economy();
	int _eco_smem = (int)g_pSharedMemoryContainer->stat_economy();
	u32 m_base = 0, c_base = 0, m_lmaps = 0, c_lmaps = 0;

	//if (Device.Resources)	Device.Resources->_GetMemoryUsage	(m_base,c_base,m_lmaps,c_lmaps);
	//	Resource check moved to m_pRender
	if (Device.m_pRender) Device.m_pRender->ResourcesGetMemoryUsage(m_base, c_base, m_lmaps, c_lmaps);

	log_vminfo();

	Msg("* [ D3D ]: textures[%d K]", (m_base + m_lmaps) / 1024);

#ifndef SEVERAL_ALLOCATORS
	Msg("* [x-ray]: process heap[%u K]", _process_heap / 1024);
#else // SEVERAL_ALLOCATORS
	Msg("* [x-ray]: process heap[%u K], game lua[%d K], render[%d K]", _process_heap / 1024, _game_lua / 1024, _render / 1024);
#endif // SEVERAL_ALLOCATORS

	Msg("* [x-ray]: economy: strings[%d K], smem[%d K]", _eco_strings / 1024, _eco_smem);

#ifdef FS_DEBUG
	Msg("* [x-ray]: file mapping: memory[%d K], count[%d]", g_file_mapped_memory / 1024, g_file_mapped_count);
	dump_file_mappings();
#endif // DEBUG
}

class CCC_MemStats : public IConsole_Command
{
public:
	CCC_MemStats(LPCSTR N) : IConsole_Command(N)
	{
		bEmptyArgsHandled = TRUE;
		g_full_memory_stats_callback = &full_memory_stats;
	};

	virtual void Execute(LPCSTR args)
	{
		full_memory_stats();
	}
};
#ifdef DEBUG
class CCC_MemCheckpoint : public IConsole_Command
{
public:
	CCC_MemCheckpoint(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = FALSE; };
	virtual void Execute(LPCSTR args)
	{
		memory_monitor::make_checkpoint(args);
	}
	virtual void	Save(IWriter *F) {}
};

#endif // #ifdef DEBUG
// console commands
class CCC_GameDifficulty : public CCC_Token
{
public:
	CCC_GameDifficulty(LPCSTR N) : CCC_Token(N, (u32*)&g_SingleGameDifficulty, difficulty_type_token)
	{
	};

	virtual void Execute(LPCSTR args)
	{
		CCC_Token::Execute(args);
		if (g_pGameLevel && Level().game)
		{
			//#ifndef	DEBUG
			if (GameID() != eGameIDSingle)
			{
				Msg("For this game type difficulty level is disabled.");
				return;
			};
			//#endif

			game_cl_Single* game = smart_cast<game_cl_Single*>(Level().game);
			VERIFY(game);
			game->OnDifficultyChanged();
		}
	}

	virtual void Info(TInfo& I)
	{
		xr_strcpy(I, "game difficulty");
	}
};

#ifdef DEBUG
class CCC_ALifePath : public IConsole_Command
{
public:
	CCC_ALifePath(LPCSTR N) : IConsole_Command(N) {};
	virtual void Execute(LPCSTR args)
	{
		if (!ai().get_level_graph())
			Msg("! there is no graph!");
		else
		{
			int id1 = -1, id2 = -1;
			sscanf(args, "%d %d", &id1, &id2);
			if ((-1 != id1) && (-1 != id2))
				if (_max(id1, id2) > (int)ai().game_graph().header().vertex_count() - 1)
					Msg("! there are only %d vertexes!", ai().game_graph().header().vertex_count());
				else
					if (_min(id1, id2) < 0)
						Msg("! invalid vertex number (%d)!", _min(id1, id2));
					else
					{
						//						Sleep				(1);
//						CTimer				timer;
//						timer.Start			();
//						float				fValue = ai().m_tpAStar->ffFindMinimalPath(id1,id2);
						//						Msg					("* %7.2f[%d] : %11I64u cycles (%.3f microseconds)",fValue,ai().m_tpAStar->m_tpaNodes.size(),timer.GetElapsed_ticks(),timer.GetElapsed_ms()*1000.f);
					}
			else
				Msg("! not enough parameters!");
		}
	}
};
#endif // DEBUG

class CCC_ALifeTimeFactor : public IConsole_Command
{
public:
	CCC_ALifeTimeFactor(LPCSTR N) : IConsole_Command(N)
	{
	};

	virtual void Execute(LPCSTR args)
	{
		float id1 = 0.0f;
		sscanf(args, "%f", &id1);
		if (id1 < EPS_L)
			Msg("Invalid time factor! (%.4f)", id1);
		else
		{
			if (!OnServer())
				return;

			Level().SetGameTimeFactor(id1);
		}
	}

	virtual void Save(IWriter* F)
	{
	};

	virtual void Status(TStatus& S)
	{
		if (!g_pGameLevel) return;

		float v = Level().GetGameTimeFactor();
		xr_sprintf(S, sizeof(S), "%3.5f", v);
		while (xr_strlen(S) && ('0' == S[xr_strlen(S) - 1])) S[xr_strlen(S) - 1] = 0;
	}

	virtual void Info(TInfo& I)
	{
		if (!OnServer()) return;
		float v = Level().GetGameTimeFactor();
		xr_sprintf(I, sizeof(I), " value = %3.5f", v);
	}

	virtual void fill_tips(vecTips& tips, u32 mode)
	{
		if (!OnServer()) return;
		float v = Level().GetGameTimeFactor();

		TStatus str;
		xr_sprintf(str, sizeof(str), "%3.5f  (current)  [0.0,1000.0]", v);
		tips.push_back(str);
		IConsole_Command::fill_tips(tips, mode);
	}
};

class CCC_ALifeSwitchDistance : public IConsole_Command
{
public:
	CCC_ALifeSwitchDistance(LPCSTR N) : IConsole_Command(N)
	{
	};

	virtual void Execute(LPCSTR args)
	{
		if ((GameID() == eGameIDSingle) && ai().get_alife())
		{
			float id1 = 0.0f;
			sscanf(args, "%f", &id1);
			if (id1 < 2.0f)
				Msg("Invalid online distance! (%.4f)", id1);
			else
			{
				NET_Packet P;
				P.w_begin(M_SWITCH_DISTANCE);
				P.w_float(id1);
				Level().Send(P, net_flags(TRUE, TRUE));
			}
		}
		else
			Log("!Not a single player game!");
	}
};

class CCC_ALifeProcessTime : public IConsole_Command
{
public:
	CCC_ALifeProcessTime(LPCSTR N) : IConsole_Command(N)
	{
	};

	virtual void Execute(LPCSTR args)
	{
		if ((GameID() == eGameIDSingle) && ai().get_alife())
		{
			game_sv_Single* tpGame = smart_cast<game_sv_Single *>(Level().Server->game);
			VERIFY(tpGame);
			int id1 = 0;
			sscanf(args, "%d", &id1);
			if (id1 < 1)
				Msg("Invalid process time! (%d)", id1);
			else
				tpGame->alife().set_process_time(id1);
		}
		else
			Log("!Not a single player game!");
	}
};

class CCC_ALifeObjectsPerUpdate : public IConsole_Command
{
public:
	CCC_ALifeObjectsPerUpdate(LPCSTR N) : IConsole_Command(N)
	{
	};

	virtual void Execute(LPCSTR args)
	{
		if ((GameID() == eGameIDSingle) && ai().get_alife())
		{
			game_sv_Single* tpGame = smart_cast<game_sv_Single *>(Level().Server->game);
			VERIFY(tpGame);
			int id1 = 0;
			sscanf(args, "%d", &id1);
			tpGame->alife().objects_per_update(id1);
		}
		else
			Log("!Not a single player game!");
	}
};

class CCC_ALifeSwitchFactor : public IConsole_Command
{
public:
	CCC_ALifeSwitchFactor(LPCSTR N) : IConsole_Command(N)
	{
	};

	virtual void Execute(LPCSTR args)
	{
		if ((GameID() == eGameIDSingle) && ai().get_alife())
		{
			game_sv_Single* tpGame = smart_cast<game_sv_Single *>(Level().Server->game);
			VERIFY(tpGame);
			float id1 = 0;
			sscanf(args, "%f", &id1);
			clamp(id1, .1f, 1.f);
			tpGame->alife().set_switch_factor(id1);
		}
		else
			Log("!Not a single player game!");
	}
};

//-----------------------------------------------------------------------
xr_unordered_set<CDemoRecord*> pDemoRecords;
class CCC_DemoRecord : public IConsole_Command
{
public:

	CCC_DemoRecord(LPCSTR N) : IConsole_Command(N)
	{
	};

	virtual void Execute(LPCSTR args)
	{
#ifndef	DEBUG
		//if (GameID() != eGameIDSingle)
		//{
		//	Msg("For this game type Demo Record is disabled.");
		//	return;
		//};
#endif
		Console->Hide();

		LPSTR fn_;
		STRCONCAT(fn_, args, ".xrdemo");
		string_path fn;
		FS.update_path(fn, "$game_saves$", fn_);

		auto pDemoRecord = xr_new<CDemoRecord>(fn, &pDemoRecords);
		g_pGameLevel->Cameras().AddCamEffector(pDemoRecord);
	}
};

class CCC_DemoRecordReturnCtrlInputs : public IConsole_Command
{
public:

	CCC_DemoRecordReturnCtrlInputs(LPCSTR N) : IConsole_Command(N)
	{
	};

	virtual void Execute(LPCSTR args)
	{
#ifndef	DEBUG
		//if (GameID() != eGameIDSingle)
		//{
		//	Msg("For this game type Demo Record is disabled.");
		//	return;
		//};
#endif
		Console->Hide();
		std::istringstream iss(args);
		std::string arg1;
		float boundary = 15.f; // default value

		if (!(iss >> arg1)) {
			Msg("not enough parameters. e.g. demo_record_return_ctrl_inputs [filename] [boundary]");
			return;
		}

		// Try to parse an optional second argument
		std::string arg2;
		if (iss >> arg2) {
			std::istringstream iss2(arg2);
			float num;
			if (iss2 >> num) {
				boundary = num;
			}
		}
		LPSTR fn_;
		STRCONCAT(fn_, arg1.c_str(), ".xrdemo");
		string_path fn;
		FS.update_path(fn, "$game_saves$", fn_);
		float life_time = 60 * 60 * 1000;
		auto pDemoRecord = xr_new<CDemoRecord>(fn, &pDemoRecords, FALSE, life_time, TRUE);
		pDemoRecord->SetCameraBoundary(boundary);
		g_pGameLevel->Cameras().AddCamEffector(pDemoRecord);
	}
};

class CCC_DemoRecordBlockedInput : public IConsole_Command
{
public:

	CCC_DemoRecordBlockedInput(LPCSTR N) : IConsole_Command(N)
	{
	};

	virtual void Execute(LPCSTR args)
	{
#ifndef	DEBUG
		//if (GameID() != eGameIDSingle)
		//{
		//	Msg("For this game type Demo Record is disabled.");
		//	return;
		//};
#endif
		Console->Hide();

		LPSTR fn_;
		STRCONCAT(fn_, args, ".xrdemo");
		string_path fn;
		FS.update_path(fn, "$game_saves$", fn_);

		auto pDemoRecord = xr_new<CDemoRecord>(fn, &pDemoRecords, TRUE);
		g_pGameLevel->Cameras().AddCamEffector(pDemoRecord);
	}
};

class CCC_DemoRecordStop : public IConsole_Command
{
public:

	CCC_DemoRecordStop(LPCSTR N) : IConsole_Command(N)
	{
		bEmptyArgsHandled = true;
	};

	virtual void Execute(LPCSTR args)
	{
		for (auto pDemoRecord : pDemoRecords) {
			pDemoRecord->StopDemo();
		}
		pDemoRecords.clear();
	}
};

class CCC_DemoRecordSetPos : public CCC_Vector3
{
	static Fvector p;
public:

	CCC_DemoRecordSetPos(LPCSTR N) : CCC_Vector3(N, &p, Fvector().set(-FLT_MAX, -FLT_MAX, -FLT_MAX),
	                                             Fvector().set(FLT_MAX, FLT_MAX, FLT_MAX))
	{
	};

	virtual void Execute(LPCSTR args)
	{
#ifndef	DEBUG
		//if (GameID() != eGameIDSingle)
		//{
		//	Msg("For this game type Demo Record is disabled.");
		//	return;
		//};
#endif
		CDemoRecord::GetGlobalPosition(p);
		CCC_Vector3::Execute(args);
		CDemoRecord::SetGlobalPosition(p);
	}

	virtual void Save(IWriter* F) { ; }
};

Fvector CCC_DemoRecordSetPos::p = {0, 0, 0};

class CCC_DemoRecordSetDir : public CCC_Vector3
{
	static Fvector d;
public:

	CCC_DemoRecordSetDir(LPCSTR N) : CCC_Vector3(N, &d, Fvector().set(-FLT_MAX, -FLT_MAX, -FLT_MAX),
		Fvector().set(FLT_MAX, FLT_MAX, FLT_MAX))
	{
	};

	virtual void Execute(LPCSTR args)
	{
#ifndef	DEBUG
		//if (GameID() != eGameIDSingle)
		//{
		//	Msg("For this game type Demo Record is disabled.");
		//	return;
		//};
#endif
		CDemoRecord::GetGlobalDirection(d);
		CCC_Vector3::Execute(args);
		CDemoRecord::SetGlobalDirection(d);
	}

	virtual void Save(IWriter* F) { ; }
};

Fvector CCC_DemoRecordSetDir::d = { 0, 0, 0 };

class CCC_DemoPlay : public IConsole_Command
{
public:
	CCC_DemoPlay(LPCSTR N) :
		IConsole_Command(N)
	{
		bEmptyArgsHandled = TRUE;
	};

	virtual void Execute(LPCSTR args)
	{
#ifndef	DEBUG
		//if (GameID() != eGameIDSingle)
		//{
		//	Msg("For this game type Demo Play is disabled.");
		//	return;
		//};
#endif
		if (0 == g_pGameLevel)
		{
			Msg("! There are no level(s) started");
		}
		else
		{
			Console->Hide();
			string_path fn;
			u32 loops = 0;
			LPSTR comma = strchr(const_cast<LPSTR>(args), ',');
			if (comma)
			{
				loops = atoi(comma + 1);
				*comma = 0; //. :)
			}
			strconcat(sizeof(fn), fn, args, ".xrdemo");
			FS.update_path(fn, "$game_saves$", fn);
			g_pGameLevel->Cameras().AddCamEffector(xr_new<CDemoPlay>(fn, 1.0f, loops));
		}
	}
};

// First Person Death
extern float offsetH;
extern float offsetP;
extern float offsetB;
extern float offsetX;
extern float offsetY;
extern float offsetZ;
extern float viewportNearOffset;
extern int firstPersonDeathPositionSmoothing;
extern int firstPersonDeathDirectionSmoothing;

class CCC_FPDDirectionOffset : public CCC_Vector3
{
	static Fvector d;
public:

	CCC_FPDDirectionOffset(LPCSTR N) : CCC_Vector3(N, &d, Fvector().set(-FLT_MAX, -FLT_MAX, -FLT_MAX),
		Fvector().set(FLT_MAX, FLT_MAX, FLT_MAX))
	{
	};

	virtual void Execute(LPCSTR args)
	{
		CCC_Vector3::Execute(args);
		offsetH = d.x;
		offsetP = d.y;
		offsetB = d.z;
	}
};
Fvector CCC_FPDDirectionOffset::d = { 0, 0, 0 };

class CCC_FPDPositionOffset : public CCC_Vector3
{
	static Fvector d;
public:

	CCC_FPDPositionOffset(LPCSTR N) : CCC_Vector3(N, &d, Fvector().set(-FLT_MAX, -FLT_MAX, -FLT_MAX),
		Fvector().set(FLT_MAX, FLT_MAX, FLT_MAX))
	{
	};

	virtual void Execute(LPCSTR args)
	{
		CCC_Vector3::Execute(args);
		offsetX = d.x;
		offsetY = d.y;
		offsetZ = d.z;
	}
};
Fvector CCC_FPDPositionOffset::d = { 0, 0, 0 };

// helper functions --------------------------------------------

bool valid_saved_game_name(LPCSTR file_name)
{
	LPCSTR I = file_name;
	LPCSTR E = file_name + xr_strlen(file_name);
	for (; I != E; ++I)
	{
		if (!strchr("/\\:*?\"<>|^()[]%", *I))
			continue;

		return (false);
	};

	return (true);
}

void get_files_list(xr_vector<shared_str>& files, LPCSTR dir, LPCSTR file_ext)
{
	VERIFY(dir && file_ext);
	files.clear_not_free();

	FS_Path* P = FS.get_path(dir);
	P->m_Flags.set(FS_Path::flNeedRescan, TRUE);
	FS.m_Flags.set(CLocatorAPI::flNeedCheck, TRUE);
	FS.rescan_pathes();

	LPCSTR fext;
	STRCONCAT(fext, "*", file_ext);

	FS_FileSet files_set;
	FS.file_list(files_set, dir, FS_ListFiles, fext);
	u32 len_str_ext = xr_strlen(file_ext);

	FS_FileSetIt itb = files_set.begin();
	FS_FileSetIt ite = files_set.end();

	for (; itb != ite; ++itb)
	{
		LPCSTR fn_ext = (*itb).name.c_str();
		VERIFY(xr_strlen(fn_ext) > len_str_ext);
		string_path fn;
		strncpy_s(fn, sizeof(fn), fn_ext, xr_strlen(fn_ext) - len_str_ext);
		files.push_back(fn);
	}
	FS.m_Flags.set(CLocatorAPI::flNeedCheck, FALSE);
}

#include "UIGameCustom.h"

class CCC_ALifeSave : public IConsole_Command
{
public:
	CCC_ALifeSave(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR args)
	{
#if 0
		if (!Level().autosave_manager().ready_for_autosave()) {
			Msg("! Cannot save the game right now!");
			return;
		}
#endif
		if (!IsGameTypeSingle())
		{
			Msg("for single-mode only");
			return;
		}
		if (!g_actor || !Actor()->g_Alive())
		{
			Msg("cannot make saved game because actor is dead :(");
			return;
		}

		Console->Execute("stat_memory");

		string_path S, S1;
		S[0] = 0;
		strncpy_s(S, sizeof(S), args, _MAX_PATH - 1);

#ifdef DEBUG
		CTimer					timer;
		timer.Start();
#endif
		if (!xr_strlen(S))
		{
			strconcat(sizeof(S), S, Core.UserName, " - ", "quicksave");
			NET_Packet net_packet;
			net_packet.w_begin(M_SAVE_GAME);
			net_packet.w_stringZ(S);
			net_packet.w_u8(0);
			Level().Send(net_packet, net_flags(TRUE));
		}
		else
		{
			if (!valid_saved_game_name(S))
			{
				Msg("! Save failed: invalid file name - %s", S);
				return;
			}

			NET_Packet net_packet;
			net_packet.w_begin(M_SAVE_GAME);
			net_packet.w_stringZ(S);
			net_packet.w_u8(1);
			Level().Send(net_packet, net_flags(TRUE));
		}
#ifdef DEBUG
		Msg("Game save overhead  : %f milliseconds", timer.GetElapsed_sec()*1000.f);
#endif
		StaticDrawableWrapper* _s = CurrentGameUI()->AddCustomStatic("game_saved", true);
		LPSTR save_name;
		STRCONCAT(save_name, CStringTable().translate("st_game_saved").c_str(), ": ", S);
		_s->wnd()->TextItemControl()->SetText(save_name);

		xr_strcat(S, ".dds");
		FS.update_path(S1, "$game_saves$", S);

#ifdef DEBUG
		timer.Start();
#endif
		MainMenu()->Screenshot(IRender_interface::SM_FOR_GAMESAVE, S1);

#ifdef DEBUG
		Msg("Screenshot overhead : %f milliseconds", timer.GetElapsed_sec()*1000.f);
#endif
	} //virtual void Execute

	virtual void fill_tips(vecTips& tips, u32 mode)
	{
		get_files_list(tips, "$game_saves$", SAVE_EXTENSION);
	}
}; //CCC_ALifeSave

class CCC_ALifeLoadFrom : public IConsole_Command
{
public:
	CCC_ALifeLoadFrom(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR args)
	{
		string_path saved_game;
		strncpy_s(saved_game, sizeof(saved_game), args, _MAX_PATH - 1);

		if (!ai().get_alife())
		{
			Log("! ALife simulator has not been started yet");
			return;
		}

		if (!xr_strlen(saved_game))
		{
			Log("! Specify file name!");
			return;
		}

		if (!CSavedGameWrapper::saved_game_exist(saved_game))
		{
			Msg("! Cannot find saved game %s", saved_game);
			return;
		}

		if (!CSavedGameWrapper::valid_saved_game(saved_game))
		{
			Msg("! Cannot load saved game %s, version mismatch or saved game is corrupted", saved_game);
			return;
		}

		if (!valid_saved_game_name(saved_game))
		{
			Msg("! Cannot load saved game %s, invalid file name", saved_game);
			return;
		}

		/*     moved to level_network_messages.cpp
		CSavedGameWrapper			wrapper(args);
		if (wrapper.level_id() == ai().level_graph().level_id()) {
		if (Device.Paused())
		Device.Pause		(FALSE, TRUE, TRUE, "CCC_ALifeLoadFrom");

		Level().remove_objects	();

		game_sv_Single			*game = smart_cast<game_sv_Single*>(Level().Server->game);
		R_ASSERT				(game);
		game->restart_simulator	(saved_game);

		return;
		}
		*/

		if (MainMenu()->IsActive())
			MainMenu()->Activate(false);

		Console->Execute("stat_memory");

		if (Device.Paused())
			Device.Pause(FALSE, TRUE, TRUE, "CCC_ALifeLoadFrom");

		NET_Packet net_packet;
		net_packet.w_begin(M_LOAD_GAME);
		net_packet.w_stringZ(saved_game);
		Level().Send(net_packet, net_flags(TRUE));
	}

	virtual void fill_tips(vecTips& tips, u32 mode)
	{
		get_files_list(tips, "$game_saves$", SAVE_EXTENSION);
	}
}; //CCC_ALifeLoadFrom

class CCC_LoadLastSave : public IConsole_Command
{
public:
	CCC_LoadLastSave(LPCSTR N) : IConsole_Command(N)
	{
		bEmptyArgsHandled = true;
	}

	virtual void Execute(LPCSTR args)
	{
		string_path saved_game = "";
		if (args)
		{
			strncpy_s(saved_game, sizeof(saved_game), args, _MAX_PATH - 1);
		}

		if (saved_game && *saved_game)
		{
			xr_strcpy(g_last_saved_game, saved_game);
			return;
		}

		if (!*g_last_saved_game)
		{
			Msg("! cannot load last saved game since it hasn't been specified");
			return;
		}

		if (!CSavedGameWrapper::saved_game_exist(g_last_saved_game))
		{
			Msg("! Cannot find saved game %s", g_last_saved_game);
			return;
		}

		if (!CSavedGameWrapper::valid_saved_game(g_last_saved_game))
		{
			Msg("! Cannot load saved game %s, version mismatch or saved game is corrupted", g_last_saved_game);
			return;
		}

		if (!valid_saved_game_name(g_last_saved_game))
		{
			Msg("! Cannot load saved game %s, invalid file name", g_last_saved_game);
			return;
		}

		LPSTR command;
		if (ai().get_alife())
		{
			STRCONCAT(command, "load ", g_last_saved_game);
			Console->Execute(command);
			return;
		}

		STRCONCAT(command, "start server(", g_last_saved_game, "/single/alife/load)");
		Console->Execute(command);
	}

	virtual void Save(IWriter* F)
	{
		if (!*g_last_saved_game)
			return;

		F->w_printf("%s %s\r\n", cName, g_last_saved_game);
	}
};

class CCC_FlushLog : public IConsole_Command
{
public:
	CCC_FlushLog(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR /**args/**/)
	{
		FlushLog();
		Msg("* Log file has been saved successfully!");
	}
};

class CCC_ClearLog : public IConsole_Command
{
public:
	CCC_ClearLog(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR)
	{
		LogFile->clear_not_free();
		FlushLog();
		Msg("* Log file has been cleaned successfully!");
	}
};

class CCC_FloatBlock : public CCC_Float
{
public:
	CCC_FloatBlock(LPCSTR N, float* V, float _min = 0, float _max = 1) :
		CCC_Float(N, V, _min, _max)
	{
	};

	virtual void Execute(LPCSTR args)
	{
#ifdef _DEBUG
		CCC_Float::Execute(args);
#else
		if (!g_pGameLevel || GameID() == eGameIDSingle)
			CCC_Float::Execute(args);
		else
		{
			Msg("! Command disabled for this type of game");
		}
#endif
	}
};

class CCC_Net_CL_InputUpdateRate : public CCC_Integer
{
protected:
	int* value_blin;
public:
	CCC_Net_CL_InputUpdateRate(LPCSTR N, int* V, int _min = 0, int _max = 999) :
		CCC_Integer(N, V, _min, _max),
		value_blin(V)
	{
	};

	virtual void Execute(LPCSTR args)
	{
		CCC_Integer::Execute(args);
		if ((*value_blin > 0) && g_pGameLevel)
		{
			g_dwInputUpdateDelta = 1000 / (*value_blin);
		};
	}
};

#ifdef DEBUG

class CCC_DrawGameGraphAll : public IConsole_Command
{
public:
	CCC_DrawGameGraphAll(LPCSTR N) : IConsole_Command(N)
	{
		bEmptyArgsHandled = true;
	}

	virtual void Execute(LPCSTR args)
	{
		if (!ai().get_level_graph())
			return;

		ai().level_graph().setup_current_level(-1);
	}
};

class CCC_DrawGameGraphCurrent : public IConsole_Command
{
public:
	CCC_DrawGameGraphCurrent(LPCSTR N) : IConsole_Command(N)
	{
		bEmptyArgsHandled = true;
	}

	virtual void Execute(LPCSTR args)
	{
		if (!ai().get_level_graph())
			return;

		ai().level_graph().setup_current_level(
			ai().level_graph().level_id()
			);
	}
};

class CCC_DrawGameGraphLevel : public IConsole_Command
{
public:
	CCC_DrawGameGraphLevel(LPCSTR N) : IConsole_Command(N)
	{
	}

	virtual void Execute(LPCSTR args)
	{
		if (!ai().get_level_graph())
			return;

		if (!*args)
		{
			ai().level_graph().setup_current_level(-1);
			return;
		}

		const GameGraph::SLevel	*level = ai().game_graph().header().level(args, true);
		if (!level)
		{
			Msg("! There is no level %s in the game graph", args);
			return;
		}

		ai().level_graph().setup_current_level(level->id());
	}
};

#if defined(USE_DEBUGGER) && !defined(USE_LUA_STUDIO)
class CCC_ScriptDbg : public IConsole_Command {
public:
	CCC_ScriptDbg(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args) {
		if (strstr(cName, "script_debug_break") == cName){
			CScriptDebugger* d = ai().script_engine().debugger();
			if (d){
				if (d->Active())
					d->initiateDebugBreak();
				else
					Msg("Script debugger not active.");
			}
			else
				Msg("Script debugger not present.");
		}
		else if (strstr(cName, "script_debug_stop") == cName){
			ai().script_engine().stopDebugger();
		}
		else if (strstr(cName, "script_debug_restart") == cName){
			ai().script_engine().restartDebugger();
		};
	};

	virtual void	Info(TInfo& I)
	{
		if (strstr(cName, "script_debug_break") == cName)
			xr_strcpy(I, "initiate script debugger [DebugBreak] command");

		else if (strstr(cName, "script_debug_stop") == cName)
			xr_strcpy(I, "stop script debugger activity");

		else if (strstr(cName, "script_debug_restart") == cName)
			xr_strcpy(I, "restarts script debugger or start if no script debugger presents");
	}
};
#endif // #if defined(USE_DEBUGGER) && !defined(USE_LUA_STUDIO)

#if defined(USE_DEBUGGER) && defined(USE_LUA_STUDIO)
class CCC_ScriptLuaStudioConnect : public IConsole_Command
{
public:
	CCC_ScriptLuaStudioConnect(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		ai().script_engine().try_connect_to_debugger();
	};
};

class CCC_ScriptLuaStudioDisconnect : public IConsole_Command
{
public:
	CCC_ScriptLuaStudioDisconnect(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR args)
	{
		ai().script_engine().disconnect_from_debugger();
	};
};
#endif // #if defined(USE_DEBUGGER) && defined(USE_LUA_STUDIO)

class CCC_DumpInfos : public IConsole_Command
{
public:
	CCC_DumpInfos(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void	Execute(LPCSTR args)
	{
		CActor* A = smart_cast<CActor*>(Level().CurrentEntity());
		if (A)
			A->DumpInfo();
	}
	virtual void	Info(TInfo& I)
	{
		xr_strcpy(I, "dumps all infoportions that actor have");
	}
};
class CCC_DumpTasks : public IConsole_Command
{
public:
	CCC_DumpTasks(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void	Execute(LPCSTR args)
	{
		CActor* A = smart_cast<CActor*>(Level().CurrentEntity());
		if (A)
			A->DumpTasks();
	}
	virtual void	Info(TInfo& I)
	{
		xr_strcpy(I, "dumps all tasks that actor have");
	}
};
#include "map_manager.h"
class CCC_DumpMap : public IConsole_Command
{
public:
	CCC_DumpMap(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void	Execute(LPCSTR args)
	{
		Level().MapManager().Dump();
	}
	virtual void	Info(TInfo& I)
	{
		xr_strcpy(I, "dumps all currentmap locations");
	}
};

#include "alife_graph_registry.h"
class CCC_DumpCreatures : public IConsole_Command
{
public:
	CCC_DumpCreatures(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void	Execute(LPCSTR args)
	{
		typedef CSafeMapIterator<ALife::_OBJECT_ID, CSE_ALifeDynamicObject>::_REGISTRY::const_iterator const_iterator;

		const_iterator I = ai().alife().graph().level().objects().begin();
		const_iterator E = ai().alife().graph().level().objects().end();
		for (; I != E; ++I)
		{
			CSE_ALifeCreatureAbstract *obj = smart_cast<CSE_ALifeCreatureAbstract *>(I->second);
			if (obj)
			{
				Msg("\"%s\",", obj->name_replace());
			}
		}
	}
	virtual void	Info(TInfo& I)
	{
		xr_strcpy(I, "dumps all creature names");
	}
};

class CCC_DebugFonts : public IConsole_Command
{
public:
	CCC_DebugFonts(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; }
	virtual void Execute(LPCSTR args)
	{
		xr_new<CUIDebugFonts>()->ShowDialog(true);
	}
};

class CCC_DebugNode : public IConsole_Command
{
public:
	CCC_DebugNode(LPCSTR N) : IConsole_Command(N) {};

	virtual void Execute(LPCSTR args)
	{
		string128 param1, param2;
		VERIFY(xr_strlen(args) < sizeof(string128));

		_GetItem(args, 0, param1, ' ');
		_GetItem(args, 1, param2, ' ');

		u32 value1;
		u32 value2;

		sscanf(param1, "%u", &value1);
		sscanf(param2, "%u", &value2);

		if ((value1 > 0) && (value2 > 0))
		{
			g_bDebugNode = TRUE;
			g_dwDebugNodeSource = value1;
			g_dwDebugNodeDest = value2;
		}
		else
		{
			g_bDebugNode = FALSE;
		}
	}
};

class CCC_ShowMonsterInfo : public IConsole_Command
{
public:
	CCC_ShowMonsterInfo(LPCSTR N) : IConsole_Command(N) {};

	virtual void Execute(LPCSTR args)
	{
		string128 param1, param2;
		VERIFY(xr_strlen(args) < sizeof(string128));

		_GetItem(args, 0, param1, ' ');
		_GetItem(args, 1, param2, ' ');

		CObject			*obj = Level().Objects.FindObjectByName(param1);
		CBaseMonster	*monster = smart_cast<CBaseMonster *>(obj);
		if (!monster)	return;

		u32				value2;

		sscanf(param2, "%u", &value2);
		monster->set_show_debug_info(u8(value2));
	}
};

void PH_DBG_SetTrackObject();
extern string64 s_dbg_trace_obj_name;
class CCC_DbgPhTrackObj : public CCC_String
{
public:
	CCC_DbgPhTrackObj(LPCSTR N) : CCC_String(N, s_dbg_trace_obj_name, sizeof(s_dbg_trace_obj_name)) {};
	virtual void Execute(LPCSTR args/**/)
	{
		CCC_String::Execute(args);
		if (!xr_strcmp(args, "none"))
		{
			ph_dbg_draw_mask1.set(ph_m1_DbgTrackObject, FALSE);
			return;
		}
		ph_dbg_draw_mask1.set(ph_m1_DbgTrackObject, TRUE);
		PH_DBG_SetTrackObject();
		//CObject* O= Level().Objects.FindObjectByName(args);
//if(O)
//{
//	PH_DBG_SetTrackObject(*(O->cName()));
//	ph_dbg_draw_mask1.set(ph_m1_DbgTrackObject,TRUE);
		//}
	}

	//virtual void	Info	(TInfo& I)
//{
//	xr_strcpy(I,"restart game fast");
	//}
};
#endif

class CCC_PHIterations : public CCC_Integer
{
public:
	CCC_PHIterations(LPCSTR N) :
		CCC_Integer(N, &phIterations, 15, 50)
	{
	};

	virtual void Execute(LPCSTR args)
	{
		CCC_Integer::Execute(args);
		// dWorldSetQuickStepNumIterations(NULL,phIterations);
		if (physics_world())
			physics_world()->StepNumIterations(phIterations);
	}
};

//#ifdef DEBUG
class CCC_PHGravity : public IConsole_Command
{
public:
	CCC_PHGravity(LPCSTR N) :
		IConsole_Command(N)
	{
	};

	virtual void Execute(LPCSTR args)
	{
		if (!physics_world())
			return;
#ifndef DEBUG
		if (g_pGameLevel && Level().game && GameID() != eGameIDSingle)
		{
			Msg("Command is not available in Multiplayer");
			return;
		}
#endif
		physics_world()->SetGravity(float(atof(args)));
	}

	virtual void Status(TStatus& S)
	{
		if (physics_world())
			xr_sprintf(S, "%3.5f", physics_world()->Gravity());
		else
			xr_sprintf(S, "%3.5f", default_world_gravity);
		while (xr_strlen(S) && ('0' == S[xr_strlen(S) - 1])) S[xr_strlen(S) - 1] = 0;
	}
};

//#endif // DEBUG

class CCC_PHFps : public IConsole_Command
{
public:
	CCC_PHFps(LPCSTR N) :
		IConsole_Command(N)
	{
	};

	virtual void Execute(LPCSTR args)
	{
		float step_count = (float)atof(args);
#ifndef		DEBUG
		clamp(step_count, 50.f, 200.f);
#endif
		//IPHWorld::SetStep(1.f/step_count);
		ph_console::ph_step_time = 1.f / step_count;
		//physics_world()->SetStep(1.f/step_count);
		if (physics_world())
			physics_world()->SetStep(ph_console::ph_step_time);
	}

	virtual void Status(TStatus& S)
	{
		xr_sprintf(S, "%3.5f", 1.f / ph_console::ph_step_time);
	}
};

#ifdef DEBUG
extern void print_help(lua_State *L);

struct CCC_LuaHelp : public IConsole_Command
{
	CCC_LuaHelp(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR args)
	{
		print_help(ai().script_engine().lua());
	}
};

struct CCC_ShowSmartCastStats : public IConsole_Command
{
	CCC_ShowSmartCastStats(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR args)
	{
		show_smart_cast_stats();
	}
};

struct CCC_ClearSmartCastStats : public IConsole_Command
{
	CCC_ClearSmartCastStats(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR args)
	{
		clear_smart_cast_stats();
	}
};
/*
struct CCC_NoClip : public CCC_Mask
{
public:
CCC_NoClip(LPCSTR N, Flags32* V, u32 M):CCC_Mask(N,V,M){};
virtual	void Execute(LPCSTR args)
{
CCC_Mask::Execute(args);
if (EQ(args,"on") || EQ(args,"1"))
{
if(g_pGameLevel && Level().CurrentViewEntity())
{
CActor* actor = smart_cast<CActor*>(Level().CurrentViewEntity());
actor->character_physics_support()->SetRemoved();
}
}
};
};
*/
#endif

//#ifndef MASTER_GOLD
#	include "game_graph.h"

struct CCC_JumpToLevel : public IConsole_Command
{
	CCC_JumpToLevel(LPCSTR N) : IConsole_Command(N)
	{
	};

	virtual void Execute(LPCSTR level)
	{
		if (!ai().get_alife())
		{
			Msg("! ALife simulator is needed to perform specified command!");
			return;
		}

		GameGraph::LEVEL_MAP::const_iterator I = ai().game_graph().header().levels().begin();
		GameGraph::LEVEL_MAP::const_iterator E = ai().game_graph().header().levels().end();
		for (; I != E; ++I)
			if (!xr_strcmp((*I).second.name(), level))
			{
				ai().alife().jump_to_level(level);
				return;
			}
		Msg("! There is no level \"%s\" in the game graph!", level);
	}

	virtual void Save(IWriter* F)
	{
	};

	virtual void fill_tips(vecTips& tips, u32 mode)
	{
		if (!ai().get_alife())
		{
			Msg("! ALife simulator is needed to perform specified command!");
			return;
		}

		GameGraph::LEVEL_MAP::const_iterator itb = ai().game_graph().header().levels().begin();
		GameGraph::LEVEL_MAP::const_iterator ite = ai().game_graph().header().levels().end();
		for (; itb != ite; ++itb)
		{
			tips.push_back((*itb).second.name());
		}
	}
};

//#ifndef MASTER_GOLD
class CCC_Script : public IConsole_Command
{
public:
	CCC_Script(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };

	virtual void Execute(LPCSTR args)
	{
		if (!xr_strlen(args))
		{
			Log("* Specify script name!");
		}
		else
		{
			// rescan pathes
			FS_Path* P = FS.get_path("$game_scripts$");
			P->m_Flags.set(FS_Path::flNeedRescan, TRUE);
			FS.rescan_pathes();
			// run script
			if (ai().script_engine().script_process(ScriptEngine::eScriptProcessorLevel))
				ai().script_engine().script_process(ScriptEngine::eScriptProcessorLevel)->add_script(args, false, true);
		}
	}

	virtual void Status(TStatus& S)
	{
		xr_strcpy(S, "<script_name> (Specify script name!)");
	}

	virtual void Save(IWriter* F)
	{
	}

	virtual void fill_tips(vecTips& tips, u32 mode)
	{
		get_files_list(tips, "$game_scripts$", ".script");
	}
};

class CCC_ScriptCommand : public IConsole_Command
{
public:
	CCC_ScriptCommand(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };

	virtual void Execute(LPCSTR args)
	{
		if (!xr_strlen(args))
			Log("* Specify string to run!");
		else
		{
			if (ai().script_engine().script_process(ScriptEngine::eScriptProcessorLevel))
			{
				ai().script_engine().script_process(ScriptEngine::eScriptProcessorLevel)->add_script(args, true, true);
				return;
			}

			string4096 S;
			shared_str m_script_name = "console command";
			xr_sprintf(S, "%s\n", args);
			int l_iErrorCode = luaL_loadbuffer(ai().script_engine().lua(), S, xr_strlen(S), "@console_command");
			if (!l_iErrorCode)
			{
				l_iErrorCode = lua_pcall(ai().script_engine().lua(), 0, 0, 0);
				if (l_iErrorCode)
				{
					ai().script_engine().print_output(ai().script_engine().lua(), *m_script_name, l_iErrorCode);
					ai().script_engine().on_error(ai().script_engine().lua());
					return;
				}
			}

			ai().script_engine().print_output(ai().script_engine().lua(), *m_script_name, l_iErrorCode);
		}
	} //void	Execute

	virtual void Status(TStatus& S)
	{
		xr_strcpy(S, "<script_name.function()> (Specify script and function name!)");
	}

	virtual void Save(IWriter* F)
	{
	}

	virtual void fill_tips(vecTips& tips, u32 mode)
	{
		if (mode == 1)
		{
			get_files_list(tips, "$game_scripts$", ".script");
			return;
		}

		IConsole_Command::fill_tips(tips, mode);
	}
};
class CCC_FreezeTime : public IConsole_Command
{
public:
	CCC_FreezeTime(LPCSTR N) : IConsole_Command(N)
	{
	}

	virtual void Execute(LPCSTR args)
	{
		float time_factor;
		bool isSet = false;

		if (EQ(args, "off") || EQ(args, "0")) {
			time_factor = 1.0f;
			isSet = true;
		}

		if (EQ(args, "on") || EQ(args, "1")) {
			time_factor = 0.0f;
			isSet = true;
		}

		if (!isSet) {
			Msg("requires parameter [on,off]");
			return;
		}

		Device.time_factor(time_factor);
	}

		virtual void Info(TInfo& I)
	{
		xr_strcpy(I, "[on,off] or [1,0]");
	}
};
class CCC_TimeFactor : public IConsole_Command
{
public:
	CCC_TimeFactor(LPCSTR N) : IConsole_Command(N)
	{
	}

	virtual void Execute(LPCSTR args)
	{
		float time_factor = (float)atof(args);
		clamp(time_factor, EPS, 1000.f);
		Device.time_factor(time_factor);
		if (!strstr(Core.Params, "-sound_constant_speed"))
			psSpeedOfSound = time_factor;
	}

	virtual void Status(TStatus& S)
	{
		xr_sprintf(S, sizeof(S), "%f", Device.time_factor());
	}

	virtual void Info(TInfo& I)
	{
		xr_strcpy(I, "[0.001 - 1000.0]");
	}

	virtual void fill_tips(vecTips& tips, u32 mode)
	{
		TStatus str;
		xr_sprintf(str, sizeof(str), "%3.3f  (current)  [0.001 - 1000.0]", Device.time_factor());
		tips.push_back(str);
		IConsole_Command::fill_tips(tips, mode);
	}
};

//#endif // MASTER_GOLD

#include "GamePersistent.h"

class CCC_MainMenu : public IConsole_Command
{
public:
	CCC_MainMenu(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR args)
	{
		bool bWhatToDo = TRUE;
		if (0 == xr_strlen(args))
		{
			bWhatToDo = !MainMenu()->IsActive();
		};

		if (EQ(args, "on") || EQ(args, "1"))
			bWhatToDo = TRUE;

		if (EQ(args, "off") || EQ(args, "0"))
			bWhatToDo = FALSE;

		if (Device.Paused() && bWhatToDo == TRUE)
			Device.Pause(FALSE, TRUE, TRUE, "li_pause_key");

		MainMenu()->Activate(bWhatToDo);
	}
};

class CCC_DiscordStatus : public CCC_Mask
{
public:
	CCC_DiscordStatus(LPCSTR N, Flags32* V, u32 M) :
		CCC_Mask(N, V, M){};

	virtual void Execute(LPCSTR args)
	{
		if (EQ(args, "on") || EQ(args, "1"))
		{
			value->set(mask, TRUE);
			discord_gameinfo.ex_update = true;
		}
		else if (EQ(args, "off") || EQ(args, "0"))
		{
			value->set(mask, FALSE);
			clearDiscordPresence();
		}
		else InvalidSyntax();
	}
};

struct CCC_StartTimeSingle : public IConsole_Command
{
	CCC_StartTimeSingle(LPCSTR N) : IConsole_Command(N)
	{
	};

	virtual void Execute(LPCSTR args)
	{
		u32 year = 1, month = 1, day = 1, hours = 0, mins = 0, secs = 0, milisecs = 0;
		sscanf(args, "%d.%d.%d %d:%d:%d.%d", &year, &month, &day, &hours, &mins, &secs, &milisecs);
		year = _max(year, 1);
		month = _max(month, 1);
		day = _max(day, 1);
		g_qwStartGameTime = generate_time(year, month, day, hours, mins, secs, milisecs);

		if (!g_pGameLevel)
			return;

		if (!Level().Server)
			return;

		if (!Level().Server->game)
			return;

		Level().SetGameTimeFactor(g_qwStartGameTime, g_fTimeFactor);
	}

	virtual void Status(TStatus& S)
	{
		u32 year = 1, month = 1, day = 1, hours = 0, mins = 0, secs = 0, milisecs = 0;
		split_time(g_qwStartGameTime, year, month, day, hours, mins, secs, milisecs);
		xr_sprintf(S, "%d.%d.%d %d:%d:%d.%d", year, month, day, hours, mins, secs, milisecs);
	}
};

struct CCC_TimeFactorSingle : public CCC_Float
{
	CCC_TimeFactorSingle(LPCSTR N, float* V, float _min = 0.f, float _max = 1.f) : CCC_Float(N, V, _min, _max)
	{
	};

	virtual void Execute(LPCSTR args)
	{
		CCC_Float::Execute(args);

		if (!g_pGameLevel)
			return;

		if (!Level().Server)
			return;

		if (!Level().Server->game)
			return;

		Level().SetGameTimeFactor(g_fTimeFactor);
	}
};

#ifdef DEBUG
class CCC_RadioGroupMask2;
class CCC_RadioMask :public CCC_Mask
{
	CCC_RadioGroupMask2		*group;
public:
	CCC_RadioMask(LPCSTR N, Flags32* V, u32 M) :
		CCC_Mask(N, V, M)
	{
		group = NULL;
	}
	void	SetGroup(CCC_RadioGroupMask2		*G)
	{
		group = G;
	}
	virtual	void	Execute(LPCSTR args);

	IC		void	Set(BOOL V)
	{
		value->set(mask, V);
	}
};

class CCC_RadioGroupMask2
{
	CCC_RadioMask *mask0;
	CCC_RadioMask *mask1;
public:
	CCC_RadioGroupMask2(CCC_RadioMask *m0, CCC_RadioMask *m1)
	{
		mask0 = m0; mask1 = m1;
		mask0->SetGroup(this);
		mask1->SetGroup(this);
	}
	void	Execute(CCC_RadioMask& m, LPCSTR args)
	{
		BOOL value = m.GetValue();
		if (value)
		{
			mask0->Set(!value); mask1->Set(!value);
		}
		m.Set(value);
	}
};

void	CCC_RadioMask::Execute(LPCSTR args)
{
	CCC_Mask::Execute(args);
	VERIFY2(group, "CCC_RadioMask: group not set");
	group->Execute(*this, args);
}

#define CMD_RADIOGROUPMASK2(p1,p2,p3,p4,p5,p6)		\
{\
    static CCC_RadioMask x##CCC_RadioMask1(p1, p2, p3);		Console->AddCommand(&x##CCC_RadioMask1); \
    static CCC_RadioMask x##CCC_RadioMask2(p4, p5, p6);		Console->AddCommand(&x##CCC_RadioMask2); \
    static CCC_RadioGroupMask2 x##CCC_RadioGroupMask2(&x##CCC_RadioMask1, &x##CCC_RadioMask2); \
}

struct CCC_DbgBullets : public CCC_Integer
{
	CCC_DbgBullets(LPCSTR N, int* V, int _min = 0, int _max = 999) : CCC_Integer(N, V, _min, _max) {};

	virtual void	Execute(LPCSTR args)
	{
		extern FvectorVec g_hit[];
		g_hit[0].clear();
		g_hit[1].clear();
		g_hit[2].clear();
		CCC_Integer::Execute(args);
	}
};

#include "attachable_item.h"
#include "attachment_owner.h"
#include "InventoryOwner.h"
#include "Inventory.h"
class CCC_TuneAttachableItem : public IConsole_Command
{
public:
	CCC_TuneAttachableItem(LPCSTR N) :IConsole_Command(N) {};
	virtual void	Execute(LPCSTR args)
	{
		if (CAttachableItem::m_dbgItem)
		{
			CAttachableItem::m_dbgItem = NULL;
			Msg("CCC_TuneAttachableItem switched to off");
			return;
		};

		CObject* obj = Level().CurrentViewEntity();	VERIFY(obj);
		shared_str ssss = args;

		CAttachmentOwner* owner = smart_cast<CAttachmentOwner*>(obj);
		CAttachableItem* itm = owner->attachedItem(ssss);
		if (itm)
		{
			CAttachableItem::m_dbgItem = itm;
		}
		else
		{
			CInventoryOwner* iowner = smart_cast<CInventoryOwner*>(obj);
			PIItem active_item = iowner->m_inventory->ActiveItem();
			if (active_item && active_item->object().cNameSect() == ssss)
				CAttachableItem::m_dbgItem = active_item->cast_attachable_item();
		}

		if (CAttachableItem::m_dbgItem)
			Msg("CCC_TuneAttachableItem switched to ON for [%s]", args);
		else
			Msg("CCC_TuneAttachableItem cannot find attached item [%s]", args);
	}

	virtual void	Info(TInfo& I)
	{
		xr_sprintf(I, "allows to change bind rotation and position offsets for attached item, <section_name> given as arguments");
	}
};

class CCC_Crash : public IConsole_Command
{
public:
	CCC_Crash(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR /**args/**/)
	{
		VERIFY3(false, "This is a test crash", "Do not post it as a bug");
		int						*pointer = 0;
		*pointer = 0;
	}
};

#ifdef DEBUG_MEMORY_MANAGER

class CCC_MemAllocShowStats : public IConsole_Command {
public:
	CCC_MemAllocShowStats(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR) {
		mem_alloc_show_stats();
	}
};

class CCC_MemAllocClearStats : public IConsole_Command {
public:
	CCC_MemAllocClearStats(LPCSTR N) : IConsole_Command(N)  { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR) {
		mem_alloc_clear_stats();
	}
};

#endif // DEBUG_MEMORY_MANAGER

class CCC_DumpModelBones : public IConsole_Command
{
public:
	CCC_DumpModelBones(LPCSTR N) : IConsole_Command(N)
	{
	}

	virtual void Execute(LPCSTR arguments)
	{
		if (!arguments || !*arguments)
		{
			Msg("! no arguments passed");
			return;
		}

		LPCSTR					name;

		if (0 == strext(arguments))
			STRCONCAT(name, arguments, ".ogf");
		else
			STRCONCAT(name, arguments);

		string_path				fn;

		if (!FS.exist(arguments) && !FS.exist(fn, "$level$", name) && !FS.exist(fn, "$game_meshes$", name))
		{
			Msg("! Cannot find visual \"%s\"", arguments);
			return;
		}

		IRenderVisual			*visual = Render->model_Create(arguments);
		IKinematics				*kinematics = smart_cast<IKinematics*>(visual);
		if (!kinematics)
		{
			Render->model_Delete(visual);
			Msg("! Invalid visual type \"%s\" (not a IKinematics)", arguments);
			return;
		}

		Msg("bones for model \"%s\"", arguments);
		for (u16 i = 0, n = kinematics->LL_BoneCount(); i < n; ++i)
			Msg("%s", *kinematics->LL_GetData(i).name);

		Render->model_Delete(visual);
	}
};

extern void show_animation_stats();

class CCC_ShowAnimationStats : public IConsole_Command
{
public:
	CCC_ShowAnimationStats(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };
	virtual void Execute(LPCSTR)
	{
		show_animation_stats();
	}
};

class CCC_InvUpgradesHierarchy : public IConsole_Command
{
public:
	CCC_InvUpgradesHierarchy(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args)
	{
		if (ai().get_alife())
		{
			ai().alife().inventory_upgrade_manager().log_hierarchy();
		}
	}
};

class CCC_InvUpgradesCurItem : public IConsole_Command
{
public:
	CCC_InvUpgradesCurItem(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args)
	{
		if (!g_pGameLevel)
		{
			return;
		}
		CUIGameSP* ui_game_sp = smart_cast<CUIGameSP*>(CurrentGameUI());
		if (!ui_game_sp)
		{
			return;
		}
		PIItem item = ui_game_sp->GetActorMenu().get_upgrade_item();
		if (item)
		{
			item->log_upgrades();
		}
		else
		{
			Msg("- Current item in ActorMenu is unknown!");
		}
	}
};

class CCC_InvDropAllItems : public IConsole_Command
{
public:
	CCC_InvDropAllItems(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args)
	{
		if (!g_pGameLevel)
		{
			return;
		}
		CUIGameSP* ui_game_sp = smart_cast<CUIGameSP*>(CurrentGameUI());
		if (!ui_game_sp)
		{
			return;
		}
		int d = 0;
		sscanf(args, "%d", &d);
		if (ui_game_sp->GetActorMenu().DropAllItemsFromRuck(d == 1))
		{
			Msg("- All items from ruck of Actor is dropping now.");
		}
		else
		{
			Msg("! ActorMenu is not in state `Inventory`");
		}
	}
}; // CCC_InvDropAllItems

#endif // DEBUG

class CCC_DumpObjects : public IConsole_Command
{
public:
	CCC_DumpObjects(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR)
	{
		Level().Objects.dump_all_objects();
	}
};

class CCC_GSCheckForUpdates : public IConsole_Command
{
public:
	CCC_GSCheckForUpdates(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = true; };

	virtual void Execute(LPCSTR arguments)
	{
		if (!MainMenu()) return;
		/*
		CGameSpy_Available GSA;
		shared_str result_string;
		if (!GSA.CheckAvailableServices(result_string))
		{
		Msg(*result_string);
		//			return;
		};
		CGameSpy_Patching GameSpyPatching;
		*/
		bool InformOfNoPatch = true;
		if (arguments && *arguments)
		{
			int bInfo = 1;
			sscanf(arguments, "%d", &bInfo);
			InformOfNoPatch = (bInfo != 0);
		}

		//		GameSpyPatching.CheckForPatch(InformOfNoPatch);

		//MainMenu()->GetGS()->GetGameSpyPatching()->CheckForPatch(InformOfNoPatch);
	}
};

class CCC_Net_SV_GuaranteedPacketMode : public CCC_Integer
{
protected:
	int* value_blin;
public:
	CCC_Net_SV_GuaranteedPacketMode(LPCSTR N, int* V, int _min = 0, int _max = 2) :
		CCC_Integer(N, V, _min, _max),
		value_blin(V)
	{
	};

	virtual void Execute(LPCSTR args)
	{
		CCC_Integer::Execute(args);
	}
};
#ifdef	DEBUG
void DBG_CashedClear();
class CCC_DBGDrawCashedClear : public IConsole_Command
{
public:
	CCC_DBGDrawCashedClear(LPCSTR N) :IConsole_Command(N) { bEmptyArgsHandled = true; }
private:
	virtual void	Execute(LPCSTR args)
	{
		DBG_CashedClear();
	}
};

#endif

class CCC_DbgVar : public IConsole_Command
{
public:
	CCC_DbgVar(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = false; };

	virtual void Execute(LPCSTR arguments)
	{
		if (!arguments || !*arguments)
		{
			return;
		}

		if (_GetItemCount(arguments, ' ') == 1)
		{
			ai_dbg::show_var(arguments);
		}
		else
		{
			char name[1024];
			float f;
			sscanf(arguments, "%s %f", name, &f);
			ai_dbg::set_var(name, f);
		}
	}
};

void CCC_RegisterCommands()
{
	//Not needed for a singleplayer-only mod
	//g_OptConCom.Init();

	CMD1(CCC_MemStats, "stat_memory");
#ifdef DEBUG
	CMD1(CCC_MemCheckpoint, "stat_memory_checkpoint");
#endif //#ifdef DEBUG
	// game

	psActorFlags.set(AF_CROUCH_TOGGLE, FALSE);
	psActorFlags.set(AF_WALK_TOGGLE, FALSE);
	psActorFlags.set(AF_SPRINT_TOGGLE, TRUE);
	psActorFlags.set(AF_LOOKOUT_TOGGLE, FALSE);
	psActorFlags.set(AF_FREELOOK_TOGGLE, FALSE);
	psActorFlags.set(AF_SIMPLE_PDA, TRUE);
	psActorFlags.set(AF_3D_PDA, TRUE);

	CMD3(CCC_Mask, "g_crouch_toggle", &psActorFlags, AF_CROUCH_TOGGLE);
	CMD3(CCC_Mask, "g_walk_toggle", &psActorFlags, AF_WALK_TOGGLE);
	CMD3(CCC_Mask, "g_sprint_toggle", &psActorFlags, AF_SPRINT_TOGGLE);
	CMD3(CCC_Mask, "g_lookout_toggle", &psActorFlags, AF_LOOKOUT_TOGGLE);
	CMD3(CCC_Mask, "g_freelook_toggle", &psActorFlags, AF_FREELOOK_TOGGLE);
	CMD3(CCC_Mask, "g_3d_pda", &psActorFlags, AF_3D_PDA);
	CMD3(CCC_Mask, "g_simple_pda", &psActorFlags, AF_SIMPLE_PDA);
	CMD1(CCC_GameDifficulty, "g_game_difficulty");

	CMD3(CCC_Mask, "g_backrun", &psActorFlags, AF_RUN_BACKWARD);

	// alife
#ifdef DEBUG
	CMD1(CCC_ALifePath, "al_path");		// build path

#endif // DEBUG

	CMD1(CCC_ALifeSave, "save"); // save game
	CMD1(CCC_ALifeLoadFrom, "load"); // load game from ...
	CMD1(CCC_LoadLastSave, "load_last_save"); // load last saved game from ...

	CMD1(CCC_FlushLog, "flush"); // flush log
	CMD1(CCC_ClearLog, "clear_log");

#ifndef MASTER_GOLD
	CMD1(CCC_ALifeTimeFactor, "al_time_factor");		// set time factor
	CMD1(CCC_ALifeSwitchDistance, "al_switch_distance");		// set switch distance
	CMD1(CCC_ALifeSwitchFactor, "al_switch_factor");		// set switch factor
#endif // #ifndef MASTER_GOLD

	CMD1(CCC_ALifeProcessTime, "al_process_time"); // set process time
	CMD1(CCC_ALifeObjectsPerUpdate, "al_objects_per_update"); // set process time

	CMD3(CCC_Mask, "hud_weapon", &psHUD_Flags, HUD_WEAPON);
	CMD3(CCC_Mask, "hud_info", &psHUD_Flags, HUD_INFO);
	CMD3(CCC_Mask, "hud_draw", &psHUD_Flags, HUD_DRAW);

	// hud
	psHUD_Flags.set(HUD_CROSSHAIR, true);
	psHUD_Flags.set(HUD_WEAPON, true);
	psHUD_Flags.set(HUD_DRAW, true);
	psHUD_Flags.set(HUD_INFO, true);

	CMD3(CCC_Mask, "hud_crosshair", &psHUD_Flags, HUD_CROSSHAIR);
	CMD3(CCC_Mask, "hud_crosshair_dist", &psHUD_Flags, HUD_CROSSHAIR_DIST);

	//#ifdef DEBUG
	CMD4(CCC_Float, "hud_fov", &psHUD_FOV_def, 0.1f, 1.0f);
	CMD4(CCC_Float, "fov", &g_fov, 5.0f, 180.0f);
	CMD4(CCC_Float, "viewport_near", &Device.ViewportNear, 0.0f, 1.0f);
	//#endif // DEBUG

	// Demo
	//#ifndef MASTER_GOLD
	CMD1(CCC_DemoPlay, "demo_play");
	CMD1(CCC_DemoRecord, "demo_record");
	CMD1(CCC_DemoRecordBlockedInput, "demo_record_blocked_input");
	CMD1(CCC_DemoRecordStop, "demo_record_stop");
	CMD1(CCC_DemoRecordReturnCtrlInputs, "demo_record_return_ctrl_inputs");
	CMD1(CCC_DemoRecordSetPos, "demo_set_cam_position");
	CMD1(CCC_DemoRecordSetDir, "demo_set_cam_direction");
	//#endif // #ifndef MASTER_GOLD

#ifndef MASTER_GOLD
	// ai
	CMD3(CCC_Mask, "mt_ai_vision", &g_mt_config, mtAiVision);
	CMD3(CCC_Mask, "mt_level_path", &g_mt_config, mtLevelPath);
	CMD3(CCC_Mask, "mt_detail_path", &g_mt_config, mtDetailPath);
	CMD3(CCC_Mask, "mt_object_handler", &g_mt_config, mtObjectHandler);
	CMD3(CCC_Mask, "mt_sound_player", &g_mt_config, mtSoundPlayer);
	CMD3(CCC_Mask, "mt_bullets", &g_mt_config, mtBullets);
	CMD3(CCC_Mask, "mt_script_gc", &g_mt_config, mtLUA_GC);
	CMD3(CCC_Mask, "mt_level_sounds", &g_mt_config, mtLevelSounds);
	CMD3(CCC_Mask, "mt_alife", &g_mt_config, mtALife);
	CMD3(CCC_Mask, "mt_map", &g_mt_config, mtMap);
#endif // MASTER_GOLD

#ifndef MASTER_GOLD
	CMD3(CCC_Mask, "ai_obstacles_avoiding", &psAI_Flags, aiObstaclesAvoiding);
	CMD3(CCC_Mask, "ai_obstacles_avoiding_static", &psAI_Flags, aiObstaclesAvoidingStatic);
	CMD3(CCC_Mask, "ai_use_smart_covers", &psAI_Flags, aiUseSmartCovers);
	CMD3(CCC_Mask, "ai_use_smart_covers_animation_slots", &psAI_Flags, (u32)aiUseSmartCoversAnimationSlot);
	CMD4(CCC_Float, "ai_smart_factor", &g_smart_cover_factor, 0.f, 1000000.f);
	CMD3(CCC_Mask, "ai_dbg_lua", &psAI_Flags, aiLua);
#endif // MASTER_GOLD

    // Moved lua_gcstep outside of DEBUG to allow for easier experimentation.
	CMD4(CCC_Integer, "lua_gcstep", &psLUA_GCSTEP, 1, 1000);
	CMD4(CCC_Integer, "lua_debug", &lua_debug, 0, 1);

#ifdef DEBUG
	CMD3(CCC_Mask, "ai_debug", &psAI_Flags, aiDebug);
	CMD3(CCC_Mask, "ai_dbg_brain", &psAI_Flags, aiBrain);
	CMD3(CCC_Mask, "ai_dbg_motion", &psAI_Flags, aiMotion);
	CMD3(CCC_Mask, "ai_dbg_frustum", &psAI_Flags, aiFrustum);
	CMD3(CCC_Mask, "ai_dbg_funcs", &psAI_Flags, aiFuncs);
	CMD3(CCC_Mask, "ai_dbg_alife", &psAI_Flags, aiALife);
	CMD3(CCC_Mask, "ai_dbg_goap", &psAI_Flags, aiGOAP);
	CMD3(CCC_Mask, "ai_dbg_goap_script", &psAI_Flags, aiGOAPScript);
	CMD3(CCC_Mask, "ai_dbg_goap_object", &psAI_Flags, aiGOAPObject);
	CMD3(CCC_Mask, "ai_dbg_cover", &psAI_Flags, aiCover);
	CMD3(CCC_Mask, "ai_dbg_anim", &psAI_Flags, aiAnimation);
	CMD3(CCC_Mask, "ai_dbg_vision", &psAI_Flags, aiVision);
	CMD3(CCC_Mask, "ai_dbg_monster", &psAI_Flags, aiMonsterDebug);
	CMD3(CCC_Mask, "ai_dbg_stalker", &psAI_Flags, aiStalker);
	CMD3(CCC_Mask, "ai_stats", &psAI_Flags, aiStats);
	CMD3(CCC_Mask, "ai_dbg_destroy", &psAI_Flags, aiDestroy);
	CMD3(CCC_Mask, "ai_dbg_serialize", &psAI_Flags, aiSerialize);
	CMD3(CCC_Mask, "ai_dbg_dialogs", &psAI_Flags, aiDialogs);
	CMD3(CCC_Mask, "ai_dbg_infoportion", &psAI_Flags, aiInfoPortion);

	CMD3(CCC_Mask, "ai_draw_game_graph", &psAI_Flags, aiDrawGameGraph);
	CMD3(CCC_Mask, "ai_draw_game_graph_stalkers", &psAI_Flags, aiDrawGameGraphStalkers);
	CMD3(CCC_Mask, "ai_draw_game_graph_objects", &psAI_Flags, aiDrawGameGraphObjects);
	CMD3(CCC_Mask, "ai_draw_game_graph_real_pos", &psAI_Flags, aiDrawGameGraphRealPos);

	CMD3(CCC_Mask, "ai_nil_object_access", &psAI_Flags, aiNilObjectAccess);

	CMD3(CCC_Mask, "ai_draw_visibility_rays", &psAI_Flags, aiDrawVisibilityRays);
	CMD3(CCC_Mask, "ai_animation_stats", &psAI_Flags, aiAnimationStats);

	/////////////////////////////////////////////HIT ANIMATION////////////////////////////////////////////////////
	//float						power_factor				= 2.f;
	//float						rotational_power_factor		= 3.f;
	//float						side_sensitivity_threshold	= 0.2f;
	//float						anim_channel_factor			= 3.f;

	CMD4(CCC_Float, "hit_anims_power", &ghit_anims_params.power_factor, 0.0f, 100.0f);
	CMD4(CCC_Float, "hit_anims_rotational_power", &ghit_anims_params.rotational_power_factor, 0.0f, 100.0f);
	CMD4(CCC_Float, "hit_anims_side_sensitivity_threshold", &ghit_anims_params.side_sensitivity_threshold, 0.0f, 10.0f);
	CMD4(CCC_Float, "hit_anims_channel_factor", &ghit_anims_params.anim_channel_factor, 0.0f, 100.0f);
	//float	block_blend					= 0.1f;
	//float	reduce_blend				= 0.5f;
	//float	reduce_power_factor			= 0.5f;
	CMD4(CCC_Float, "hit_anims_block_blend", &ghit_anims_params.block_blend, 0.f, 1.f);
	CMD4(CCC_Float, "hit_anims_reduce_blend", &ghit_anims_params.reduce_blend, 0.f, 1.f);
	CMD4(CCC_Float, "hit_anims_reduce_blend_factor", &ghit_anims_params.reduce_power_factor, 0.0f, 1.0f);
	CMD4(CCC_Integer, "hit_anims_tune", &tune_hit_anims, 0, 1);
	/////////////////////////////////////////////HIT ANIMATION END////////////////////////////////////////////////////

#ifdef DEBUG_MEMORY_MANAGER
	CMD3(CCC_Mask, "debug_on_frame_gather_stats", &psAI_Flags, aiDebugOnFrameAllocs);
	CMD4(CCC_Float, "debug_on_frame_gather_stats_frequency", &debug_on_frame_gather_stats_frequency, 0.f, 1.f);
	CMD1(CCC_MemAllocShowStats, "debug_on_frame_show_stats");
	CMD1(CCC_MemAllocClearStats, "debug_on_frame_clear_stats");
#endif // DEBUG_MEMORY_MANAGER

	CMD1(CCC_DumpModelBones, "debug_dump_model_bones");

	CMD1(CCC_DrawGameGraphAll, "ai_draw_game_graph_all");
	CMD1(CCC_DrawGameGraphCurrent, "ai_draw_game_graph_current_level");
	CMD1(CCC_DrawGameGraphLevel, "ai_draw_game_graph_level");

	CMD4(CCC_Integer, "ai_dbg_inactive_time", &g_AI_inactive_time, 0, 1000000);

	CMD1(CCC_DebugNode, "ai_dbg_node");
#if defined(USE_DEBUGGER) && !defined(USE_LUA_STUDIO)
	CMD1(CCC_ScriptDbg, "script_debug_break");
	CMD1(CCC_ScriptDbg, "script_debug_stop");
	CMD1(CCC_ScriptDbg, "script_debug_restart");
#endif // #if defined(USE_DEBUGGER) && !defined(USE_LUA_STUDIO)

#if defined(USE_DEBUGGER) && defined(USE_LUA_STUDIO)
	CMD1(CCC_ScriptLuaStudioConnect, "lua_studio_connect");
	CMD1(CCC_ScriptLuaStudioDisconnect, "lua_studio_disconnect");
#endif // #if defined(USE_DEBUGGER) && defined(USE_LUA_STUDIO)

	CMD1(CCC_ShowMonsterInfo, "ai_monster_info");
	CMD1(CCC_DebugFonts, "debug_fonts");
	CMD1(CCC_TuneAttachableItem, "dbg_adjust_attachable_item");

	CMD1(CCC_ShowAnimationStats, "ai_show_animation_stats");
#endif // DEBUG

#ifndef MASTER_GOLD
	CMD3(CCC_Mask, "ai_ignore_actor", &psAI_Flags, aiIgnoreActor);
#endif // MASTER_GOLD

	// Physics
	CMD1(CCC_PHFps, "ph_frequency");
	CMD1(CCC_PHIterations, "ph_iterations");

#ifdef DEBUG
	CMD1(CCC_PHGravity, "ph_gravity");
	CMD4(CCC_FloatBlock, "ph_timefactor", &phTimefactor, 0.000001f, 1000.f);
	CMD4(CCC_FloatBlock, "ph_break_common_factor", &ph_console::phBreakCommonFactor, 0.f, 1000000000.f);
	CMD4(CCC_FloatBlock, "ph_rigid_break_weapon_factor", &ph_console::phRigidBreakWeaponFactor, 0.f, 1000000000.f);
	CMD4(CCC_Integer, "ph_tri_clear_disable_count", &ph_console::ph_tri_clear_disable_count, 0, 255);
	CMD4(CCC_FloatBlock, "ph_tri_query_ex_aabb_rate", &ph_console::ph_tri_query_ex_aabb_rate, 1.01f, 3.f);
	CMD3(CCC_Mask, "g_no_clip", &psActorFlags, AF_NO_CLIP);
	CMD1(CCC_JumpToLevel, "jump_to_level");
	CMD3(CCC_Mask, "g_god", &psActorFlags, AF_GODMODE);
	CMD3(CCC_Mask, "g_unlimitedammo", &psActorFlags, AF_UNLIMITEDAMMO);
	CMD1(CCC_Script, "run_script");
	CMD1(CCC_ScriptCommand, "run_string");
	CMD1(CCC_TimeFactor, "time_factor");
#endif // DEBUG

	/* AVO: changing restriction to -dbg key instead of DEBUG */
	//#ifndef MASTER_GOLD
#ifdef MASTER_GOLD
	if (0 != strstr(Core.Params, "-dbg"))
	{
		CMD1(CCC_JumpToLevel, "jump_to_level");
		CMD3(CCC_Mask, "g_god", &psActorFlags, AF_GODMODE);
		CMD3(CCC_Mask, "g_unlimitedammo", &psActorFlags, AF_UNLIMITEDAMMO);
		CMD1(CCC_Script, "run_script");
		CMD1(CCC_ScriptCommand, "run_string");
		//CMD3(CCC_Mask, "g_no_clip", &psActorFlags, AF_NO_CLIP);
		CMD1(CCC_PHGravity, "ph_gravity");
		CMD3(CCC_Mask, "log_missing_ini", &FS.m_Flags, FS.flPrintLTX);
		CMD4(CCC_Float, "g_end_modif", &g_end_modif, 0.f, 10.f);
	}
#endif // MASTER_GOLD
	//#endif // MASTER_GOLD
	/* AVO: end */

	CMD1(CCC_TimeFactor, "time_factor");
	CMD1(CCC_FreezeTime, "freeze_time");

	CMD3(CCC_Mask, "g_firepos", &psActorFlags, AF_FIREPOS);
	CMD3(CCC_Mask, "g_firepos_zoom", &psActorFlags, AF_FIREPOS_ZOOM);
	CMD3(CCC_Mask, "g_firedir_third_person", &psActorFlags, AF_FIREDIR_THIRD_PERSON);
	CMD3(CCC_Mask, "g_aimpos", &psActorFlags, AF_AIMPOS);
	CMD3(CCC_Mask, "g_aimpos_zoom", &psActorFlags, AF_AIMPOS_ZOOM);
	CMD4(CCC_Integer, "g_nearwall", &g_nearwall, 0, 2);
	CMD4(CCC_Integer, "g_nearwall_trace", &g_nearwall_trace, 0, 1);

	CMD3(CCC_Mask, "g_crosshair_show_always", &psCrosshair_Flags, CROSSHAIR_SHOW_ALWAYS);
	CMD3(CCC_Mask, "g_crosshair_independent", &psCrosshair_Flags, CROSSHAIR_INDEPENDENT);
	
	CrosshairCameraNearCommands(g_crosshair_camera_near, "camera_near");
	CrosshairCameraFarCommands(g_crosshair_camera_far, "camera_far");
	CrosshairNearCommands(g_crosshair_weapon_near, "weapon_near");
	CrosshairFarCommands(g_crosshair_weapon_far, "weapon_far");
	CrosshairNearCommands(g_crosshair_device_near, "device_near");
	CrosshairFarCommands(g_crosshair_device_far, "device_far");

	CMD4(CCC_Float, "g_recon_show_speed", &recon_show_speed, 0.f, 20.f);
	CMD4(CCC_Float, "g_recon_hide_speed", &recon_hide_speed, 0.f, 20.f);
	CMD4(CCC_Float, "g_recon_mindist", &recon_mindist, 0.f, 300.f);
	CMD4(CCC_Float, "g_recon_maxdist", &recon_mindist, 0.f, 300.f);
	CMD4(CCC_Float, "g_recon_minspeed", &recon_mindist, .1f, 20.f);
	CMD4(CCC_Float, "g_recon_maxspeed", &recon_mindist, .1f, 20.f);

	CMD3(CCC_Mask, "g_use_tracers", &psActorFlags, AF_USE_TRACERS);
	CMD3(CCC_Mask, "g_autopickup", &psActorFlags, AF_AUTOPICKUP);
	CMD3(CCC_Mask, "g_dynamic_music", &psActorFlags, AF_DYNAMIC_MUSIC);
	psActorFlags.set(AF_IMPORTANT_SAVE, TRUE);
	CMD3(CCC_Mask, "g_important_save", &psActorFlags, AF_IMPORTANT_SAVE);

#ifdef DEBUG
	CMD1(CCC_LuaHelp, "lua_help");
	CMD1(CCC_ShowSmartCastStats, "show_smart_cast_stats");
	CMD1(CCC_ClearSmartCastStats, "clear_smart_cast_stats");

	CMD3(CCC_Mask, "dbg_draw_actor_alive", &dbg_net_Draw_Flags, dbg_draw_actor_alive);
	CMD3(CCC_Mask, "dbg_draw_actor_dead", &dbg_net_Draw_Flags, dbg_draw_actor_dead);
	CMD3(CCC_Mask, "dbg_draw_customzone", &dbg_net_Draw_Flags, dbg_draw_customzone);
	CMD3(CCC_Mask, "dbg_draw_teamzone", &dbg_net_Draw_Flags, dbg_draw_teamzone);
	CMD3(CCC_Mask, "dbg_draw_invitem", &dbg_net_Draw_Flags, dbg_draw_invitem);
	CMD3(CCC_Mask, "dbg_draw_actor_phys", &dbg_net_Draw_Flags, dbg_draw_actor_phys);
	CMD3(CCC_Mask, "dbg_draw_customdetector", &dbg_net_Draw_Flags, dbg_draw_customdetector);
	CMD3(CCC_Mask, "dbg_destroy", &dbg_net_Draw_Flags, dbg_destroy);
	CMD3(CCC_Mask, "dbg_draw_autopickupbox", &dbg_net_Draw_Flags, dbg_draw_autopickupbox);
	CMD3(CCC_Mask, "dbg_draw_rp", &dbg_net_Draw_Flags, dbg_draw_rp);
	CMD3(CCC_Mask, "dbg_draw_climbable", &dbg_net_Draw_Flags, dbg_draw_climbable);
	CMD3(CCC_Mask, "dbg_draw_skeleton", &dbg_net_Draw_Flags, dbg_draw_skeleton);

	CMD3(CCC_Mask, "dbg_draw_ph_contacts", &ph_dbg_draw_mask, phDbgDrawContacts);
	CMD3(CCC_Mask, "dbg_draw_ph_enabled_aabbs", &ph_dbg_draw_mask, phDbgDrawEnabledAABBS);
	CMD3(CCC_Mask, "dbg_draw_ph_intersected_tries", &ph_dbg_draw_mask, phDBgDrawIntersectedTries);
	CMD3(CCC_Mask, "dbg_draw_ph_saved_tries", &ph_dbg_draw_mask, phDbgDrawSavedTries);
	CMD3(CCC_Mask, "dbg_draw_ph_tri_trace", &ph_dbg_draw_mask, phDbgDrawTriTrace);
	CMD3(CCC_Mask, "dbg_draw_ph_positive_tries", &ph_dbg_draw_mask, phDBgDrawPositiveTries);
	CMD3(CCC_Mask, "dbg_draw_ph_negative_tries", &ph_dbg_draw_mask, phDBgDrawNegativeTries);
	CMD3(CCC_Mask, "dbg_draw_ph_tri_test_aabb", &ph_dbg_draw_mask, phDbgDrawTriTestAABB);
	CMD3(CCC_Mask, "dbg_draw_ph_tries_changes_sign", &ph_dbg_draw_mask, phDBgDrawTriesChangesSign);
	CMD3(CCC_Mask, "dbg_draw_ph_tri_point", &ph_dbg_draw_mask, phDbgDrawTriPoint);
	CMD3(CCC_Mask, "dbg_draw_ph_explosion_position", &ph_dbg_draw_mask, phDbgDrawExplosionPos);
	CMD3(CCC_Mask, "dbg_draw_ph_statistics", &ph_dbg_draw_mask, phDbgDrawObjectStatistics);
	CMD3(CCC_Mask, "dbg_draw_ph_mass_centres", &ph_dbg_draw_mask, phDbgDrawMassCenters);
	CMD3(CCC_Mask, "dbg_draw_ph_death_boxes", &ph_dbg_draw_mask, phDbgDrawDeathActivationBox);
	CMD3(CCC_Mask, "dbg_draw_ph_hit_app_pos", &ph_dbg_draw_mask, phHitApplicationPoints);
	CMD3(CCC_Mask, "dbg_draw_ph_cashed_tries_stats", &ph_dbg_draw_mask, phDbgDrawCashedTriesStat);
	CMD3(CCC_Mask, "dbg_draw_ph_car_dynamics", &ph_dbg_draw_mask, phDbgDrawCarDynamics);
	CMD3(CCC_Mask, "dbg_draw_ph_car_plots", &ph_dbg_draw_mask, phDbgDrawCarPlots);
	CMD3(CCC_Mask, "dbg_ph_ladder", &ph_dbg_draw_mask, phDbgLadder);
	CMD3(CCC_Mask, "dbg_draw_ph_explosions", &ph_dbg_draw_mask, phDbgDrawExplosions);
	CMD3(CCC_Mask, "dbg_draw_car_plots_all_trans", &ph_dbg_draw_mask, phDbgDrawCarAllTrnsm);
	CMD3(CCC_Mask, "dbg_draw_ph_zbuffer_disable", &ph_dbg_draw_mask, phDbgDrawZDisable);
	CMD3(CCC_Mask, "dbg_ph_obj_collision_damage", &ph_dbg_draw_mask, phDbgDispObjCollisionDammage);
	CMD_RADIOGROUPMASK2("dbg_ph_ai_always_phmove", &ph_dbg_draw_mask, phDbgAlwaysUseAiPhMove, "dbg_ph_ai_never_phmove", &ph_dbg_draw_mask, phDbgNeverUseAiPhMove);
	CMD3(CCC_Mask, "dbg_ph_ik", &ph_dbg_draw_mask, phDbgIK);
	CMD3(CCC_Mask, "dbg_ph_ik_off", &ph_dbg_draw_mask1, phDbgIKOff);
	CMD3(CCC_Mask, "dbg_draw_ph_ik_goal", &ph_dbg_draw_mask, phDbgDrawIKGoal);
	CMD3(CCC_Mask, "dbg_ph_ik_limits", &ph_dbg_draw_mask, phDbgIKLimits);
	CMD3(CCC_Mask, "dbg_ph_character_control", &ph_dbg_draw_mask, phDbgCharacterControl);
	CMD3(CCC_Mask, "dbg_draw_ph_ray_motions", &ph_dbg_draw_mask, phDbgDrawRayMotions);
	CMD4(CCC_Float, "dbg_ph_vel_collid_damage_to_display", &dbg_vel_collid_damage_to_display, 0.f, 1000.f);
	CMD4(CCC_DbgBullets, "dbg_draw_bullet_hit", &g_bDrawBulletHit, 0, 1);
	CMD4(CCC_Integer, "dbg_draw_fb_crosshair", &g_bDrawFirstBulletCrosshair, 0, 1);
	CMD1(CCC_DbgPhTrackObj, "dbg_track_obj");
	CMD3(CCC_Mask, "dbg_ph_actor_restriction", &ph_dbg_draw_mask1, ph_m1_DbgActorRestriction);
	CMD3(CCC_Mask, "dbg_draw_ph_hit_anims", &ph_dbg_draw_mask1, phDbgHitAnims);
	CMD3(CCC_Mask, "dbg_draw_ph_ik_limits", &ph_dbg_draw_mask1, phDbgDrawIKLimits);
	CMD3(CCC_Mask, "dbg_draw_ph_ik_predict", &ph_dbg_draw_mask1, phDbgDrawIKPredict);
	CMD3(CCC_Mask, "dbg_draw_ph_ik_collision", &ph_dbg_draw_mask1, phDbgDrawIKCollision);
	CMD3(CCC_Mask, "dbg_draw_ph_ik_shift_object", &ph_dbg_draw_mask1, phDbgDrawIKSHiftObject);
	CMD3(CCC_Mask, "dbg_draw_ph_ik_blending", &ph_dbg_draw_mask1, phDbgDrawIKBlending);
	CMD1(CCC_DBGDrawCashedClear, "dbg_ph_cashed_clear");
	extern BOOL dbg_draw_character_bones;
	extern BOOL dbg_draw_character_physics;
	extern BOOL dbg_draw_character_binds;
	extern BOOL dbg_draw_character_physics_pones;
	extern BOOL ik_cam_shift;
	CMD4(CCC_Integer, "dbg_draw_character_bones", &dbg_draw_character_bones, FALSE, TRUE);
	CMD4(CCC_Integer, "dbg_draw_character_physics", &dbg_draw_character_physics, FALSE, TRUE);
	CMD4(CCC_Integer, "dbg_draw_character_binds", &dbg_draw_character_binds, FALSE, TRUE);
	CMD4(CCC_Integer, "dbg_draw_character_physics_pones", &dbg_draw_character_physics_pones, FALSE, TRUE);

	CMD4(CCC_Integer, "ik_cam_shift", &ik_cam_shift, FALSE, TRUE);

	extern	float ik_cam_shift_tolerance;
	CMD4(CCC_Float, "ik_cam_shift_tolerance", &ik_cam_shift_tolerance, 0.f, 2.f);
	float ik_cam_shift_speed;
	CMD4(CCC_Float, "ik_cam_shift_speed", &ik_cam_shift_speed, 0.f, 1.f);
	extern	BOOL dbg_draw_doors;
	CMD4(CCC_Integer, "dbg_draw_doors", &dbg_draw_doors, FALSE, TRUE);

	/*
	extern int ik_allign_free_foot;
	extern int ik_local_blending;
	extern int ik_blend_free_foot;
	extern int ik_collide_blend;
	CMD4(CCC_Integer,	"ik_allign_free_foot"			,&ik_allign_free_foot,	0,	1);
	CMD4(CCC_Integer,	"ik_local_blending"				,&ik_local_blending,	0,	1);
	CMD4(CCC_Integer,	"ik_blend_free_foot"			,&ik_blend_free_foot,	0,	1);
	CMD4(CCC_Integer,	"ik_collide_blend"				,&ik_collide_blend,	0,	1);
	*/
	extern 	BOOL dbg_draw_ragdoll_spawn;
	CMD4(CCC_Integer, "dbg_draw_ragdoll_spawn", &dbg_draw_ragdoll_spawn, FALSE, TRUE);
	extern BOOL debug_step_info;
	extern BOOL debug_step_info_load;
	CMD4(CCC_Integer, "debug_step_info", &debug_step_info, FALSE, TRUE);
	CMD4(CCC_Integer, "debug_step_info_load", &debug_step_info_load, FALSE, TRUE);
	extern BOOL debug_character_material_load;
	CMD4(CCC_Integer, "debug_character_material_load", &debug_character_material_load, FALSE, TRUE);
	extern XRPHYSICS_API BOOL dbg_draw_camera_collision;
	CMD4(CCC_Integer, "dbg_draw_camera_collision", &dbg_draw_camera_collision, FALSE, TRUE);
	extern XRPHYSICS_API float	camera_collision_character_skin_depth;
	extern XRPHYSICS_API float	camera_collision_character_shift_z;
	CMD4(CCC_FloatBlock, "camera_collision_character_shift_z", &camera_collision_character_shift_z, 0.f, 1.f);
	CMD4(CCC_FloatBlock, "camera_collision_character_skin_depth", &camera_collision_character_skin_depth, 0.f, 1.f);
	extern BOOL	dbg_draw_animation_movement_controller;
	CMD4(CCC_Integer, "dbg_draw_animation_movement_controller", &dbg_draw_animation_movement_controller, FALSE, TRUE);

	/*
	enum
	{
	dbg_track_obj_blends_bp_0			= 1<< 0,
	dbg_track_obj_blends_bp_1			= 1<< 1,
	dbg_track_obj_blends_bp_2			= 1<< 2,
	dbg_track_obj_blends_bp_3			= 1<< 3,
	dbg_track_obj_blends_motion_name	= 1<< 4,
	dbg_track_obj_blends_time			= 1<< 5,
	dbg_track_obj_blends_ammount		= 1<< 6,
	dbg_track_obj_blends_mix_params		= 1<< 7,
	dbg_track_obj_blends_flags			= 1<< 8,
	dbg_track_obj_blends_state			= 1<< 9,
	dbg_track_obj_blends_dump			= 1<< 10
	};
	*/
	extern Flags32	dbg_track_obj_flags;
	CMD3(CCC_Mask, "dbg_track_obj_blends_bp_0", &dbg_track_obj_flags, dbg_track_obj_blends_bp_0);
	CMD3(CCC_Mask, "dbg_track_obj_blends_bp_1", &dbg_track_obj_flags, dbg_track_obj_blends_bp_1);
	CMD3(CCC_Mask, "dbg_track_obj_blends_bp_2", &dbg_track_obj_flags, dbg_track_obj_blends_bp_2);
	CMD3(CCC_Mask, "dbg_track_obj_blends_bp_3", &dbg_track_obj_flags, dbg_track_obj_blends_bp_3);
	CMD3(CCC_Mask, "dbg_track_obj_blends_motion_name", &dbg_track_obj_flags, dbg_track_obj_blends_motion_name);
	CMD3(CCC_Mask, "dbg_track_obj_blends_time", &dbg_track_obj_flags, dbg_track_obj_blends_time);

	CMD3(CCC_Mask, "dbg_track_obj_blends_ammount", &dbg_track_obj_flags, dbg_track_obj_blends_ammount);
	CMD3(CCC_Mask, "dbg_track_obj_blends_mix_params", &dbg_track_obj_flags, dbg_track_obj_blends_mix_params);
	CMD3(CCC_Mask, "dbg_track_obj_blends_flags", &dbg_track_obj_flags, dbg_track_obj_blends_flags);
	CMD3(CCC_Mask, "dbg_track_obj_blends_state", &dbg_track_obj_flags, dbg_track_obj_blends_state);
	CMD3(CCC_Mask, "dbg_track_obj_blends_dump", &dbg_track_obj_flags, dbg_track_obj_blends_dump);

	CMD1(CCC_DbgVar, "dbg_var");

	extern float	dbg_text_height_scale;
	CMD4(CCC_FloatBlock, "dbg_text_height_scale", &dbg_text_height_scale, 0.2f, 5.f);
#endif

#ifdef DEBUG
	CMD1(CCC_DumpInfos, "dump_infos");
	CMD1(CCC_DumpTasks, "dump_tasks");
	CMD1(CCC_DumpMap, "dump_map");
	CMD1(CCC_DumpCreatures, "dump_creatures");

#endif
	// demonized: string_table_error_msg cvar to display missing translations
	CMD4(CCC_Integer, "string_table_error_msg", &CStringTable::m_bWriteErrorsToLog, 0, 1);

	CMD3(CCC_Mask, "cl_dynamiccrosshair", &psHUD_Flags, HUD_CROSSHAIR_DYNAMIC);
	CMD1(CCC_MainMenu, "main_menu");

#ifndef MASTER_GOLD
	CMD1(CCC_StartTimeSingle, "start_time_single");
	CMD4(CCC_TimeFactorSingle, "time_factor_single", &g_fTimeFactor, 0.f, 1000.0f);
#endif // MASTER_GOLD

	g_uCommonFlags.zero();
	g_uCommonFlags.set(flAiUseTorchDynamicLights, TRUE);

	CMD3(CCC_Mask, "ai_use_torch_dynamic_lights", &g_uCommonFlags, flAiUseTorchDynamicLights);

#ifndef MASTER_GOLD
	CMD4(CCC_Vector3, "psp_cam_offset", &CCameraLook2::m_cam_offset, Fvector().set(-1000, -1000, -1000), Fvector().set(1000, 1000, 1000));
#endif // MASTER_GOLD

	CMD1(CCC_GSCheckForUpdates, "check_for_updates");
#ifdef DEBUG
	CMD1(CCC_Crash, "crash");
	CMD1(CCC_DumpObjects, "dump_all_objects");
	CMD3(CCC_String, "stalker_death_anim", dbg_stalker_death_anim, 32);
	CMD4(CCC_Integer, "death_anim_debug", &death_anim_debug, FALSE, TRUE);
	CMD4(CCC_Integer, "death_anim_velocity", &b_death_anim_velocity, FALSE, TRUE);
	CMD4(CCC_Integer, "dbg_imotion_draw_velocity", &dbg_imotion_draw_velocity, FALSE, TRUE);
	CMD4(CCC_Integer, "dbg_imotion_collide_debug", &dbg_imotion_collide_debug, FALSE, TRUE);

	CMD4(CCC_Integer, "dbg_imotion_draw_skeleton", &dbg_imotion_draw_skeleton, FALSE, TRUE);
	CMD4(CCC_Float, "dbg_imotion_draw_velocity_scale", &dbg_imotion_draw_velocity_scale, 0.0001f, 100.0f);

	CMD4(CCC_Integer, "show_wnd_rect_all", &g_show_wnd_rect2, 0, 1);
	CMD4(CCC_Integer, "dbg_show_ani_info", &g_ShowAnimationInfo, 0, 1);
	CMD4(CCC_Integer, "dbg_dump_physics_step", &ph_console::g_bDebugDumpPhysicsStep, 0, 1);
	CMD1(CCC_InvUpgradesHierarchy, "inv_upgrades_hierarchy");
	CMD1(CCC_InvUpgradesCurItem, "inv_upgrades_cur_item");
	CMD4(CCC_Integer, "inv_upgrades_log", &g_upgrades_log, 0, 1);
	CMD1(CCC_InvDropAllItems, "inv_drop_all_items");

	extern BOOL dbg_moving_bones_snd_player;
	CMD4(CCC_Integer, "dbg_bones_snd_player", &dbg_moving_bones_snd_player, FALSE, TRUE);
#endif
	CMD4(CCC_Float, "con_sensitive", &g_console_sensitive, 0.01f, 1.0f);

	psActorFlags.set(AF_AIM_TOGGLE, FALSE);
	CMD3(CCC_Mask, "wpn_aim_toggle", &psActorFlags, AF_AIM_TOGGLE);
	CMD4(CCC_Float, "wpn_degradation", &f_weapon_deterioration, 0.1f, 2.0f);
	CMD4(CCC_Float, "power_loss_bias", &f_power_loss_bias, 0.0f, 1.0f);
	CMD4(CCC_Float, "power_loss_factor", &f_power_loss_factor, 0.1f, 3.0f);

	//	CMD4(CCC_Integer,	"hud_old_style",			&g_old_style_ui_hud, 0, 1);

#ifdef DEBUG
	CMD4(CCC_Float, "ai_smart_cover_animation_speed_factor", &g_smart_cover_animation_speed_factor, .1f, 10.f);
	CMD4(CCC_Float, "air_resistance_epsilon", &air_resistance_epsilon, .0f, 1.f);
#endif // #ifdef DEBUG

	CMD4(CCC_Integer, "g_sleep_time", &psActorSleepTime, 1, 24);

	CMD4(CCC_Integer, "ai_use_old_vision", &g_ai_use_old_vision, 0, 1);

	CMD4(CCC_Integer, "ai_die_in_anomaly", &g_ai_die_in_anomaly, 0, 1); //Alundaio

	CMD4(CCC_Integer, "pseudogiant_can_damage_objects_on_stomp", &pseudogiantCanDamageObjects, 0, 1);

	CMD4(CCC_Integer, "telekinetic_objects_include_corpses", &g_telekinetic_objects_include_corpses, 0, 1); // Tosox

	CMD4(CCC_Integer, "allow_weapon_control_inertion_factor", &g_allow_weapon_control_inertion_factor, 0, 1); // momopate
	CMD4(CCC_Integer, "allow_outfit_control_inertion_factor", &g_allow_outfit_control_inertion_factor, 0, 1);
	CMD4(CCC_Integer, "render_short_tracers", &g_render_short_tracers, 0, 1);
	CMD4(CCC_Integer, "fix_avelocity_spread", &g_fix_avelocity_spread, 0, 1);
	CMD4(CCC_Integer, "apply_pdm_to_ads", &g_apply_pdm_to_ads, 0, 1);
	CMD4(CCC_Integer, "smooth_ads_transition", &g_smooth_ads_transition, 0, 1);
	CMD4(CCC_Integer, "allow_silencer_hide_tracer", &g_allow_silencer_hide_tracer, 0, 1);

	CMD4(CCC_Float, "ai_aim_predict_time", &g_aim_predict_time, 0.f, 10.f);

	CMD4(CCC_Float, "head_bob_factor", &g_head_bob_factor, 0.f, 2.f);

	CMD3(CCC_Mask, "weapon_sway", &psDeviceFlags2, rsAimSway);

	CMD3(CCC_Mask, "blend_move_anims", &psDeviceFlags2, rsBlendMoveAnims);

	CMD4(CCC_Integer, "mt_update_weapon_sounds", &mt_UpdateWeaponSounds, 0, 1);

	CMD4(CCC_Integer, "spawn_antifreeze", &spawn_antifreeze, 0, 1);
	CMD4(CCC_Integer, "spawn_antifreeze_debug", &spawn_antifreeze_debug, 0, 1);

	CMD4(CCC_Float, "ik_calc_dist", &IK_CALC_DIST, 50, 150);
	CMD4(CCC_Float, "ik_always_calc_dist", &IK_ALWAYS_CALC_DIST, 10, 50);
	CMD4(CCC_Integer, "r__optimize_calculate_bones", &r_optimize_calculate_bones, 0, 1);

	CMD4(CCC_Integer, "g_progressive_stamina_cost", &progressiveStaminaCost, 0, 1);
	CMD4(CCC_Integer, "g_npcs_look_at_actor", &NPCsLookAtActor, 0, 1);
	CMD4(CCC_Float, "g_npcs_look_at_actor_min_distance", &NPCsLookAtActorMinDistance, 1.f, 8.f);

	// demonized: Restores fun physics bugs like lift
	CMD4(CCC_Integer, "fun_allowed", &fun_allowed, 0, 1);

#ifdef DEBUG
	//extern BOOL g_use_new_ballistics;
	//CMD4(CCC_Integer,	"use_new_ballistics",	&g_use_new_ballistics, 0, 1);
	extern float g_bullet_time_factor;
	CMD4(CCC_Float, "g_bullet_time_factor", &g_bullet_time_factor, 0.f, 10.f);
#endif

#ifdef DEBUG
	extern BOOL g_ai_dbg_sight;
	CMD4(CCC_Integer, "ai_dbg_sight", &g_ai_dbg_sight, 0, 1);

#endif // #ifdef DEBUG
	//Alundaio: Scoped outside DEBUG
	extern BOOL g_ai_aim_use_smooth_aim;
	CMD4(CCC_Integer, "ai_aim_use_smooth_aim", &g_ai_aim_use_smooth_aim, 0, 1);

	extern float g_ai_aim_min_speed;
	CMD4(CCC_Float, "ai_aim_min_speed", &g_ai_aim_min_speed, 0.f, 10.f*PI);

	extern float g_ai_aim_min_angle;
	CMD4(CCC_Float, "ai_aim_min_angle", &g_ai_aim_min_angle, 0.f, 10.f*PI);

	extern float g_ai_aim_max_angle;
	CMD4(CCC_Float, "ai_aim_max_angle", &g_ai_aim_max_angle, 0.f, 10.f*PI);

#ifdef DEBUG
	extern BOOL g_debug_doors;
	CMD4(CCC_Integer, "ai_debug_doors", &g_debug_doors, 0, 1);
#endif // #ifdef DEBUG

	*g_last_saved_game = 0;

	CMD3(CCC_String, "slot_0", g_quick_use_slots[0], 32);
	CMD3(CCC_String, "slot_1", g_quick_use_slots[1], 32);
	CMD3(CCC_String, "slot_2", g_quick_use_slots[2], 32);
	CMD3(CCC_String, "slot_3", g_quick_use_slots[3], 32);

	psDeviceFlags2.set(rsKeypress, TRUE);
	CMD3(CCC_Mask, "keypress_on_start", &psDeviceFlags2, rsKeypress);

	//Discord
	psDeviceFlags2.set(rsDiscord, TRUE);
	CMD3(CCC_DiscordStatus, "discord_status", &psDeviceFlags2, rsDiscord);
	CMD4(CCC_Float, "discord_update_rate", &discord_update_rate, .5f, 5.f);

	psActorFlags.set(rsCODPickup, TRUE);
	CMD3(CCC_Mask, "cl_cod_pickup_mode", &psDeviceFlags2, rsCODPickup);
	psActorFlags.set(AF_MULTI_ITEM_PICKUP, TRUE);
	CMD3(CCC_Mask, "g_multi_item_pickup", &psActorFlags, AF_MULTI_ITEM_PICKUP);

	CMD3(CCC_Token, "g_dead_body_collision", &g_dead_body_collision, dead_body_collision_tokens);
	CMD3(CCC_Mask, "g_feel_grenade", &psDeviceFlags2, rsFeelGrenade);
	CMD3(CCC_Mask, "g_always_active", &psDeviceFlags2, rsAlwaysActive);

	// demonized: use_english_text_for_missing_translations
	CMD4(CCC_Integer, "use_english_text_for_missing_translations", &use_english_text_for_missing_translations, 0, 1);

	//First Person Death
	CMD4(CCC_Integer, "first_person_death", &firstPersonDeath, 0, 1);
	CMD1(CCC_FPDDirectionOffset, "first_person_death_direction_offset");
	CMD1(CCC_FPDPositionOffset, "first_person_death_position_offset");
	CMD4(CCC_Integer, "first_person_death_position_smoothing", &firstPersonDeathPositionSmoothing, 1, 30);
	CMD4(CCC_Integer, "first_person_death_direction_smoothing", &firstPersonDeathDirectionSmoothing, 1, 60);
	CMD4(CCC_Float, "first_person_death_near_plane_offset", &viewportNearOffset, -.1f, .5f);

	// PDA commands
	CMD4(CCC_Integer, "pda_map_zoom_in_to_mouse", &pda_map_zoom_in_to_mouse, 0, 1);
	CMD4(CCC_Integer, "pda_map_zoom_out_to_mouse", &pda_map_zoom_out_to_mouse, 0, 1);

	// Mouse Wheel
	CMD4(CCC_Integer, "mouse_wheel_change_weapon", &mouseWheelChangeWeapon, 0, 1);
	CMD4(CCC_Integer, "mouse_wheel_invert_change_weapon", &mouseWheelInvertChangeWeapons, 0, 1);
	CMD4(CCC_Integer, "mouse_wheel_invert_zoom", &mouseWheelInvertZoom, 0, 1);

	//Toggle crash saving
	CMD4(CCC_Integer, "crash_save", &crash_saving::enabled, 0, 1);
	CMD4(CCC_Integer, "crash_save_count", &crash_saving::saveCountMax, 0, 20);

	// Monster Stuck Fix
	CMD4(CCC_Integer, "monster_stuck_fix", &monsterStuckFix, 0, 1);

	// Timestamps in log
	CMD4(CCC_Integer, "log_timestamps", &logTimestamps, 0, 1);

	// Freelook
	CMD4(CCC_Float, "freelook_cam_limit", &f_Freelook_cam_limit, 0.f, PI);

	// Input buffer sizes
	CMD4(CCC_Integer, "mouse_buffer_size", &MOUSEBUFFERSIZE, 64, 2048);
	CMD4(CCC_Integer, "keyboard_buffer_size", &KEYBOARDBUFFERSIZE, 64, 512);

	// Print warnings when using bone_position and bone_direction functions and encounter invalid bones
	CMD4(CCC_Integer, "print_bone_warnings", &print_bone_warnings, 0, 1);

	// Print DLTX warnings when "override section which doesn't exist"
	CMD4(CCC_Integer, "print_dltx_warnings", &print_dltx_warnings, 0, 1);

	// Ignore "no renderer type set for hanging-lamp" error
	CMD4(CCC_Integer, "hanging_lamp_ignore_match_configuration", &alifeObjectHangingLampIgnoreMatchConfiguration, 0, 1);

	// Poltergeists spawn corpses on death
	CMD4(CCC_Integer, "poltergeist_spawn_corpse_on_death", &poltergeist_spawn_corpse_on_death, 0, 1);

	// New zoom delta algorithm
	CMD4(CCC_Integer, "new_zoom_delta_algorithm", &useNewZoomDeltaAlgorithm, 0, 1);

	if (strstr(Core.Params, "-dbgdev"))
		CMD4(CCC_Float, "g_streff", &streff, -10.f, 10.f);
	//No need for server commands in a singleplayer-only mod
	//register_mp_console_commands();
    
    zoomFlags.set(NEW_ZOOM, FALSE);
    zoomFlags.set(SDS_ZOOM, TRUE);
    zoomFlags.set(SDS_SPEED, TRUE);
    zoomFlags.set(SDS, TRUE);

    CMD3(CCC_Mask, "new_zoom_enable", &zoomFlags, NEW_ZOOM);
    CMD3(CCC_Mask, "sds_zoom_enable", &zoomFlags, SDS_ZOOM);
    CMD3(CCC_Mask, "sds_speed_enable", &zoomFlags, SDS_SPEED);
    CMD3(CCC_Mask, "sds_enable", &zoomFlags, SDS);

    CMD4(CCC_Float, "zoom_step_count", &n_zoom_step_count, 1.0f, 10.0f);

	// UBGL/Aim mode switch separation
	// When switching to UBGL the weapon will remember what mode you switched from and will put you back in that mode. 
	// For example: You were aiming down with a canted sight when you switched to UBGL. When you switch back it will put you back into Canted sight aim and not the scope.
	CMD4(CCC_Integer, "use_separate_ubgl_keybind", &useSeparateUBGLKeybind, 0, 1);
	CMD4(CCC_Integer, "aimmode_remember", &g_aimmode_remember, 0, 1);

	// Allows freelook during reload animations
	CMD4(CCC_Integer, "freelook_while_reloading", &g_freelook_while_reloading, 0, 1);
	// Indoor weapon sounds
	CMD4(CCC_Float, "g_gunsnd_indoor", &g_gunsnd_indoor, 0.0f, 1.0f);
	CMD4(CCC_Float, "g_gunsnd_indoor_volume", &g_gunsnd_indoor_volume, 0.0f, 5.0f);

	// Draw pickup item names
	CMD4(CCC_Integer, "g_draw_pickup_item_names", &drawPickupItemNames, 0, 1);

	// Wallmark distances
	CMD4(CCC_Float, "g_wallmark_range_static", &wallmark_range_static, 0.f, 1000.f);
	CMD4(CCC_Float, "g_wallmark_range_skeleton", &wallmark_range_skeleton, 0.f, 1000.f);
}

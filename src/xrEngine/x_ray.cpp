//-----------------------------------------------------------------------------
// File: x_ray.cpp
//
// Programmers:
// Oles - Oles Shishkovtsov
// AlexMX - Alexander Maksimchuk
//-----------------------------------------------------------------------------
#include "stdafx.h"
#include "igame_level.h"
#include "igame_persistent.h"

#include "dedicated_server_only.h"
#include "no_single.h"
#include "../xrNetServer/NET_AuthCheck.h"

#include "xr_input.h"
#include "xr_ioconsole.h"
#include "x_ray.h"
#include "std_classes.h"
#include "GameFont.h"
#include "resource.h"
#include "LightAnimLibrary.h"
#include "../xrcdb/ispatial.h"
#include "Text_Console.h"
#include <process.h>
#include <locale.h>

#include <unicode\unistr.h>
#include <unicode\ucnv.h>
#include <discord\discord.h>
#include "../xrCore/profiler.h"

#include "xrSash.h"

//#include "securom_api.h"


//---------------------------------------------------------------------
#define XRAY_MONOLITH_VERSION "X-Ray Monolith v1.5.3"
ENGINE_API CInifile* pGameIni = NULL;
BOOL g_bIntroFinished = FALSE;
extern void Intro(void* fn);
extern void Intro_DSHOW(void* fn);
extern int PASCAL IntroDSHOW_wnd(HINSTANCE hInstC, HINSTANCE hInstP, LPSTR lpCmdLine, int nCmdShow);
//int max_load_stage = 0;

// computing build id
XRCORE_API LPCSTR build_date;
XRCORE_API u32 build_id;

#ifdef MASTER_GOLD
# define NO_MULTI_INSTANCES
#endif // #ifdef MASTER_GOLD


//Discord
discord::Core* discord_core{};
discord::Activity discordPresence{};
static int64_t StartTime;
bool use_discord = true;
#pragma comment(lib, "discord_game_sdk.lib")
rpc_info discord_gameinfo;
rpc_strings discord_strings;
float discord_update_rate = .5f;

//UTF-8 (ICU)
#pragma comment(lib, "icuuc.lib")
//#pragma comment(lib, "sicuuc.lib")
//#pragma comment(lib, "sicudt.lib")

//Reshade
#pragma comment(lib, "reshadecompat.lib")
bool use_reshade = false;
extern bool init_reshade();
extern void unregister_reshade();

//ImGui
#pragma comment(lib, "imgui.lib")

static LPSTR month_id[12] =
{
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static int days_in_month[12] =
{
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static int start_day = 31; // 31
static int start_month = 1; // January
static int start_year = 1999; // 1999

// binary hash, mainly for copy-protection

#ifndef DEDICATED_SERVER

#include <ctype.h>

#define DEFAULT_MODULE_HASH "3CAABCFCFF6F3A810019C6A72180F166"
static char szEngineHash[33] = DEFAULT_MODULE_HASH;

#endif // DEDICATED_SERVER

void compute_build_id()
{
	build_date = __DATE__;

	int days;
	int months = 0;
	int years;
	string16 month;
	string256 buffer;
	xr_strcpy(buffer, __DATE__);
	sscanf(buffer, "%s %d %d", month, &days, &years);

	for (int i = 0; i < 12; i++)
	{
		if (_stricmp(month_id[i], month))
			continue;

		months = i;
		break;
	}

	build_id = (years - start_year) * 365 + days - start_day;

	for (int i = 0; i < months; ++i)
		build_id += days_in_month[i];

	for (int i = 0; i < start_month - 1; ++i)
		build_id -= days_in_month[i];
}

//---------------------------------------------------------------------
// 2446363
// umbt@ukr.net
//////////////////////////////////////////////////////////////////////////
struct _SoundProcessor : public pureFrame
{
	virtual void _BCL OnFrame()
	{
		//Msg ("------------- sound: %d [%3.2f,%3.2f,%3.2f]",u32(Device.dwFrame),VPUSH(Device.vCameraPosition));
		Device.Statistic->Sound.Begin();
		::Sound->update(Device.vCameraPosition, Device.vCameraDirection, Device.vCameraTop);
		Device.Statistic->Sound.End();
	}
} SoundProcessor;

//////////////////////////////////////////////////////////////////////////
// global variables
ENGINE_API CApplication* pApp = NULL;
static HWND logoWindow = NULL;

int doLauncher();
void doBenchmark(LPCSTR name);
ENGINE_API bool g_bBenchmark = false;
string512 g_sBenchmarkName;


ENGINE_API string512 g_sLaunchOnExit_params;
ENGINE_API string512 g_sLaunchOnExit_app;
ENGINE_API string_path g_sLaunchWorkingFolder;
// -------------------------------------------
// startup point
void InitEngine()
{
	Engine.Initialize();
	while (!g_bIntroFinished) Sleep(100);
	Device.Initialize();
}

struct path_excluder_predicate
{
	explicit path_excluder_predicate(xr_auth_strings_t const* ignore) :
		m_ignore(ignore)
	{
	}

	bool xr_stdcall is_allow_include(LPCSTR path)
	{
		if (!m_ignore)
			return true;

		return allow_to_include_path(*m_ignore, path);
	}

	xr_auth_strings_t const* m_ignore;
};

extern float g_fTimeFactor;

PROTECT_API void InitSettings()
{
	string_path fname;
	FS.update_path(fname, "$game_config$", "system.ltx");
#ifdef DEBUG
    Msg("Updated path to system.ltx is %s", fname);
#endif // #ifdef DEBUG
	pSettings = xr_new<CInifile>(fname, TRUE);
	CHECK_OR_EXIT(0 != pSettings->section_count(),
	              make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));

	xr_auth_strings_t tmp_ignore_pathes;
	xr_auth_strings_t tmp_check_pathes;
	fill_auth_check_params(tmp_ignore_pathes, tmp_check_pathes);

	path_excluder_predicate tmp_excluder(&tmp_ignore_pathes);
	CInifile::allow_include_func_t tmp_functor;
	tmp_functor.bind(&tmp_excluder, &path_excluder_predicate::is_allow_include);
	pSettingsAuth = xr_new<CInifile>(
		fname,
		TRUE,
		TRUE,
		FALSE,
		0,
		tmp_functor
	);

	FS.update_path(fname, "$game_config$", "game.ltx");
	pGameIni = xr_new<CInifile>(fname, TRUE);
	CHECK_OR_EXIT(0 != pGameIni->section_count(),
	              make_string("Cannot find file %s.\nReinstalling application may fix this problem.", fname));

	g_fTimeFactor = pSettings->r_float("alife", "time_factor");
}

PROTECT_API void InitConsole()
{
	////SECUROM_MARKER_SECURITY_ON(5)

#ifdef DEDICATED_SERVER
    {
        Console = xr_new<CTextConsole>();
    }
#else
	// else
	{
		Console = xr_new<CConsole>();
	}
#endif
	Console->Initialize();

	xr_strcpy(Console->ConfigFile, "user.ltx");
	if (strstr(Core.Params, "-ltx "))
	{
		string64 c_name;
		sscanf(strstr(Core.Params, "-ltx ") + 5, "%[^ ] ", c_name);
		xr_strcpy(Console->ConfigFile, c_name);
	}

	////SECUROM_MARKER_SECURITY_OFF(5)
}

PROTECT_API void InitInput()
{
	BOOL bCaptureInput = FALSE; // !strstr(Core.Params, "-i");

	pInput = xr_new<CInput>(bCaptureInput);
}

void destroyInput()
{
	xr_delete(pInput);
}

PROTECT_API void InitSound1()
{
	CSound_manager_interface::_create(0);
}

PROTECT_API void InitSound2()
{
	CSound_manager_interface::_create(1);
}

void destroySound()
{
	CSound_manager_interface::_destroy();
}

void destroySettings()
{
	auto s = const_cast<CInifile**>(&pSettings);
	xr_delete(*s);
	auto sa = const_cast<CInifile**>(&pSettingsAuth);
	xr_delete(*sa);
	xr_delete(pGameIni);
}

void destroyConsole()
{
	Console->Execute("cfg_save");
	Console->Destroy();
	xr_delete(Console);
}

void destroyEngine()
{
	Device.Destroy();
	Engine.Destroy();
}

void execUserScript()
{
	Console->Execute("default_controls");
	Console->ExecuteScript(Console->ConfigFile);
}

void slowdownthread(void*)
{
	PROF_EVENT();

	// Sleep (30*1000);
	for (;;)
	{
		if (Device.Statistic->fFPS < 30) Sleep(1);
		if (Device.mt_bMustExit) return;
		if (0 == pSettings) return;
		if (0 == Console) return;
		if (0 == pInput) return;
		if (0 == pApp) return;
	}
}

void CheckPrivilegySlowdown()
{
#ifdef DEBUG
    if (strstr(Core.Params, "-slowdown"))
    {
        thread_spawn(slowdownthread, "slowdown", 0, 0);
    }
    if (strstr(Core.Params, "-slowdown2x"))
    {
        thread_spawn(slowdownthread, "slowdown", 0, 0);
        thread_spawn(slowdownthread, "slowdown", 0, 0);
    }
#endif // DEBUG
}

LPCSTR xr_ToUTF8(LPCSTR input, int max_length)
{
	UErrorCode errorCode = U_ZERO_ERROR;
	UConverter *conv_from = ucnv_open("cp1251", &errorCode);
	R_ASSERT3(conv_from, "[Discord RPC] Error creating UConverter!\n", std::to_string(errorCode).c_str());

	std::vector<UChar> converted(strlen(input) * 2);
	int32_t conv_len = ucnv_toUChars(conv_from, &converted[0], converted.size(), input, strlen(input), &errorCode);
	if (errorCode != U_ZERO_ERROR)
	{
		Msg("[Discord RPC] Failed to convert string! (%s)", std::to_string(errorCode).c_str());
		return input;
	}

	converted.resize(conv_len);
	ucnv_close(conv_from);

	// needs to be static so the data buffer is still valid after this function returns
	static std::string g;
	g.clear();

	g.resize(converted.size() * 4);

	UConverter *conv_u8 = ucnv_open("UTF-8", &errorCode);
	int32_t u8_len = ucnv_fromUChars(conv_u8, &g[0], g.size(), &converted[0], converted.size(), &errorCode);
	if (errorCode != U_ZERO_ERROR)
	{
		Msg("[Discord RPC] Failed to convert string! (%s)", std::to_string(errorCode).c_str());
		return input;
	}

	g.resize(_min(u8_len, max_length));
	ucnv_close(conv_u8);

	return g.data();
}

//Discord Rich Presence - Rezy ------------------------------------------------

void DiscordLog(discord::LogLevel level, std::string message)
{
	Msg("[Discord RPC]: %s", message.c_str());
}

void updateDiscordPresence()
{
	PROF_EVENT();

	if (!use_discord)
		return;

	static char details_buffer[128];
	static char state_buffer[128];

	// Main Menu
	if (discord_gameinfo.mainmenu)
	{
		snprintf(state_buffer, 128, discord_strings.mainmenu);
		discordPresence.GetAssets().SetLargeImage("gamelogo");
		discordPresence.GetAssets().SetLargeText("");
		discordPresence.GetAssets().SetSmallImage("");
		discordPresence.GetAssets().SetSmallText("");
			
		// Pause Menu
		if (discord_gameinfo.ingame)
			snprintf(state_buffer, 128, discord_strings.paused);
		else
			discordPresence.SetDetails("");
	}	

	// Loading
	else if (discord_gameinfo.loadscreen)
	{
		snprintf(state_buffer, 128, discord_strings.loading);
		discordPresence.SetDetails("");
		discordPresence.GetAssets().SetLargeImage("gamelogo");
		discordPresence.GetAssets().SetLargeText("");
		discordPresence.GetAssets().SetSmallImage("");
		discordPresence.GetAssets().SetSmallText("");
		discord_gameinfo.ex_update = true;
	}

	// In Game
	else if (discord_gameinfo.ingame)
	{
		// Time + Level Name
		char levelname_time[128];
		if (discord_gameinfo.level_name && discord_gameinfo.currenttime)
		{
			snprintf(levelname_time, 128, "%s | %s", discord_gameinfo.level_name, discord_gameinfo.currenttime);
			discordPresence.GetAssets().SetLargeText(levelname_time);
		}
		else if (discord_gameinfo.level_name)
		{
			snprintf(levelname_time, 128, discord_gameinfo.level_name);
			discordPresence.GetAssets().SetLargeText(levelname_time);
		}
		else
			discord_gameinfo.ex_update = true;

		//Faction, Rank, Rep
		if (discord_gameinfo.faction && discord_gameinfo.faction_name)
		{
			discordPresence.GetAssets().SetSmallImage(discord_gameinfo.faction);
			char rank_faction_rep[128];
			if (discord_gameinfo.rank_name && discord_gameinfo.reputation)
				snprintf(rank_faction_rep, 128, "%s | %s", discord_gameinfo.rank_name, discord_gameinfo.reputation);
			else
				snprintf(rank_faction_rep, 128, discord_gameinfo.faction_name);
			discordPresence.GetAssets().SetSmallText(rank_faction_rep);
		}

		// GameMode + Active Task
		if (discord_gameinfo.gamemode)
		{
			if (discord_gameinfo.task_name && 0 != xr_strcmp(discord_gameinfo.task_name, ""))
				snprintf(details_buffer, 128, "%s | %s", discord_gameinfo.gamemode, discord_gameinfo.task_name);
			else
				snprintf(details_buffer, 128, discord_gameinfo.gamemode);
			discordPresence.SetDetails(details_buffer);
		}

		// God Mode
		if (discord_gameinfo.godmode)
			snprintf(state_buffer, 128, discord_strings.godmode);

		// Health
		else if (discord_gameinfo.health)
		{
			// Iron Man
			if (discord_gameinfo.ironman && discord_gameinfo.lives_left)
			{
				if (discord_gameinfo.lives_left == 0 || discord_gameinfo.lives_left > 1)
					snprintf(state_buffer, 128, "%s: %i | %i %s", discord_strings.health, discord_gameinfo.health,
					        discord_gameinfo.lives_left, discord_strings.livesleft);
				else
					snprintf(state_buffer, 128, "%s: %i | %i %s", discord_strings.health, discord_gameinfo.health,
					        discord_gameinfo.lives_left, discord_strings.livesleftsingle);
			}

			// Azazel
			else if (discord_gameinfo.possessed_lives)
			{
				if (discord_gameinfo.possessed_lives == 0 || discord_gameinfo.possessed_lives > 1)
					snprintf(state_buffer, 128, "%s: %i | %i %s", discord_strings.health, discord_gameinfo.health,
					        discord_gameinfo.possessed_lives, discord_strings.livespossessed);
				else
					snprintf(state_buffer, 128, "%s: %i | %i %s", discord_strings.health, discord_gameinfo.health,
					        discord_gameinfo.possessed_lives, discord_strings.livespossessedsingle);
			}

			// No Iron Man or Azazel
			else
				snprintf(state_buffer, 128, "%s: %i", discord_strings.health, discord_gameinfo.health);

			discordPresence.SetState(state_buffer);
		}
		else
		{
			// Iron Man
			if (discord_gameinfo.ironman && discord_gameinfo.lives_left)
			{
				int real_lives = discord_gameinfo.lives_left - 1;
				if (real_lives == 0 || real_lives > 1)
					snprintf(state_buffer, 128, "%s | %i %s", discord_strings.dead, real_lives, discord_strings.livesleft);
				else
					snprintf(state_buffer, 128, "%s | %i %s", discord_strings.dead, real_lives,
						discord_strings.livesleftsingle);
			}


			// Azazel
			else if (discord_gameinfo.possessed_lives)
			{
				if (discord_gameinfo.possessed_lives == 0 || discord_gameinfo.possessed_lives > 1)
					snprintf(state_buffer, 128, "%s | %i %s", discord_strings.dead, discord_gameinfo.possessed_lives,
						discord_strings.livespossessed);
				else
					snprintf(state_buffer, 128, "%s | %i %s", discord_strings.dead, discord_gameinfo.possessed_lives,
						discord_strings.livespossessedsingle);
			}

			// No Iron Man or Azazel
			else
				snprintf(state_buffer, 128, "%s", discord_strings.dead);

			discordPresence.SetState(state_buffer);
		}

		// Level Icon
		if (discord_gameinfo.level && discord_gameinfo.level_icon_index)
		{
			char icon_buffer[32];
			snprintf(icon_buffer, 32, "%s_%i", discord_gameinfo.level, discord_gameinfo.level_icon_index);
			discordPresence.GetAssets().SetLargeImage(icon_buffer);
		}
	}

	discordPresence.SetState(state_buffer);
	discord_core->ActivityManager().UpdateActivity(discordPresence, [](discord::Result result) {});
}

void Init_Discord()
{
	auto result = discord::Core::Create(477910171964801060, DiscordCreateFlags_NoRequireDiscord, &discord_core);
	
	if (result != discord::Result::Ok)
	{
		Msg("[Discord RPC] Failed to create Discord RPC");
		use_discord = false;
		return;
	}

	discord_core->SetLogHook(discord::LogLevel::Error, DiscordLog);
	Msg("[Discord RPC] Created successfully!");

	//Set up basic RPC
	StartTime = time(0);
	discordPresence.SetType(discord::ActivityType::Playing);
	discordPresence.GetTimestamps().SetStart(StartTime);
	discordPresence.GetAssets().SetLargeImage("gamelogo");
	discord_core->ActivityManager().UpdateActivity(discordPresence, [](discord::Result result) {});
}

void clearDiscordPresence()
{
	if (discord_core)
		discord_core->ActivityManager().ClearActivity([](discord::Result result) {});
}

void Startup()
{
	InitSound1();
	execUserScript();
	InitSound2();

	// ...command line for auto start
	{
		LPCSTR pStartup = strstr(Core.Params, "-start ");
		if (pStartup) Console->Execute(pStartup + 1);
	}
	{
		LPCSTR pStartup = strstr(Core.Params, "-load ");
		if (pStartup) Console->Execute(pStartup + 1);
	}

	// Initialize APP
	ShowWindow(Device.m_hWnd, SW_SHOWNORMAL);
	Device.Create();

	LALib.OnCreate();
	pApp = xr_new<CApplication>();
	g_pGamePersistent = (IGame_Persistent*)NEW_INSTANCE(CLSID_GAME_PERSISTANT);
	g_SpatialSpace = xr_new<ISpatial_DB>();
	g_SpatialSpacePhysic = xr_new<ISpatial_DB>();

	// Destroy LOGO
	DestroyWindow(logoWindow);
	logoWindow = NULL;

	//Discord Rich Presence - Rezy
	Init_Discord();

	//Reshade
	use_reshade = init_reshade();
	if (use_reshade)
		Msg("[ReShade]: Loaded compatibility addon");
	else
		Msg("[ReShade]: ReShade not installed or version too old - didn't load compatibility addon");

	// Main cycle
	Msg("* [x-ray]: Starting Main Loop");
	Memory.mem_usage();

	Device.Run();

	// Discord
	clearDiscordPresence();

	//Reshade
	if (use_reshade)
		unregister_reshade();

	// Destroy APP
	xr_delete(g_SpatialSpacePhysic);
	xr_delete(g_SpatialSpace);
	DEL_INSTANCE(g_pGamePersistent);

	xr_delete(pApp);
	pApp = NULL;

	Engine.Event.Dump();

	// Destroying
	//. destroySound();
	destroyInput();

	if (!g_bBenchmark && !g_SASH.IsRunning())
		destroySettings();

	LALib.OnDestroy();

	if (!g_bBenchmark && !g_SASH.IsRunning())
		destroyConsole();
	else
		Console->Destroy();

	destroySound();

	destroyEngine();
}

static INT_PTR CALLBACK logDlgProc(HWND hw, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_DESTROY:
		break;
	case WM_CLOSE:
		DestroyWindow(hw);
		break;
	case WM_COMMAND:
		if (LOWORD(wp) == IDCANCEL)
			DestroyWindow(hw);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

// Always request high performance GPU
extern "C"
{
	// https://docs.nvidia.com/gameworks/content/technologies/desktop/optimus.htm
	_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001; // NVIDIA Optimus

	// https://gpuopen.com/amdpowerxpressrequesthighperformance/
	_declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001; // PowerXpress or Hybrid Graphics
}

/*
void test_rtc ()
{
CStatTimer tMc,tM,tC,tD;
u32 bytes=0;
tMc.FrameStart ();
tM.FrameStart ();
tC.FrameStart ();
tD.FrameStart ();
::Random.seed (0x12071980);
for (u32 test=0; test<10000; test++)
{
u32 in_size = ::Random.randI(1024,256*1024);
u32 out_size_max = rtc_csize (in_size);
u8* p_in = xr_alloc<u8> (in_size);
u8* p_in_tst = xr_alloc<u8> (in_size);
u8* p_out = xr_alloc<u8> (out_size_max);
for (u32 git=0; git<in_size; git++) p_in[git] = (u8)::Random.randI (8); // garbage
bytes += in_size;

tMc.Begin ();
memcpy (p_in_tst,p_in,in_size);
tMc.End ();

tM.Begin ();
CopyMemory(p_in_tst,p_in,in_size);
tM.End ();

tC.Begin ();
u32 out_size = rtc_compress (p_out,out_size_max,p_in,in_size);
tC.End ();

tD.Begin ();
u32 in_size_tst = rtc_decompress(p_in_tst,in_size,p_out,out_size);
tD.End ();

// sanity check
R_ASSERT (in_size == in_size_tst);
for (u32 tit=0; tit<in_size; tit++) R_ASSERT(p_in[tit] == p_in_tst[tit]); // garbage

xr_free (p_out);
xr_free (p_in_tst);
xr_free (p_in);
}
tMc.FrameEnd (); float rMc = 1000.f*(float(bytes)/tMc.result)/(1024.f*1024.f);
tM.FrameEnd (); float rM = 1000.f*(float(bytes)/tM.result)/(1024.f*1024.f);
tC.FrameEnd (); float rC = 1000.f*(float(bytes)/tC.result)/(1024.f*1024.f);
tD.FrameEnd (); float rD = 1000.f*(float(bytes)/tD.result)/(1024.f*1024.f);
Msg ("* memcpy: %5.2f M/s (%3.1f%%)",rMc,100.f*rMc/rMc);
Msg ("* mm-memcpy: %5.2f M/s (%3.1f%%)",rM,100.f*rM/rMc);
Msg ("* compression: %5.2f M/s (%3.1f%%)",rC,100.f*rC/rMc);
Msg ("* decompression: %5.2f M/s (%3.1f%%)",rD,100.f*rD/rMc);
}
*/
extern void testbed(void);

// video
/*
static HINSTANCE g_hInstance ;
static HINSTANCE g_hPrevInstance ;
static int g_nCmdShow ;
void __cdecl intro_dshow_x (void*)
{
IntroDSHOW_wnd (g_hInstance,g_hPrevInstance,"GameData\\Stalker_Intro.avi",g_nCmdShow);
g_bIntroFinished = TRUE ;
}
*/
#define dwStickyKeysStructSize sizeof( STICKYKEYS )
#define dwFilterKeysStructSize sizeof( FILTERKEYS )
#define dwToggleKeysStructSize sizeof( TOGGLEKEYS )

struct damn_keys_filter
{
	BOOL bScreenSaverState;

	// Sticky & Filter & Toggle keys

	STICKYKEYS StickyKeysStruct;
	FILTERKEYS FilterKeysStruct;
	TOGGLEKEYS ToggleKeysStruct;

	DWORD dwStickyKeysFlags;
	DWORD dwFilterKeysFlags;
	DWORD dwToggleKeysFlags;

	damn_keys_filter()
	{
		// Screen saver stuff

		bScreenSaverState = FALSE;

		// Saveing current state
		SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, (PVOID)&bScreenSaverState, 0);

		if (bScreenSaverState)
			// Disable screensaver
			SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, FALSE, NULL, 0);

		dwStickyKeysFlags = 0;
		dwFilterKeysFlags = 0;
		dwToggleKeysFlags = 0;


		ZeroMemory(&StickyKeysStruct, dwStickyKeysStructSize);
		ZeroMemory(&FilterKeysStruct, dwFilterKeysStructSize);
		ZeroMemory(&ToggleKeysStruct, dwToggleKeysStructSize);

		StickyKeysStruct.cbSize = dwStickyKeysStructSize;
		FilterKeysStruct.cbSize = dwFilterKeysStructSize;
		ToggleKeysStruct.cbSize = dwToggleKeysStructSize;

		// Saving current state
		SystemParametersInfo(SPI_GETSTICKYKEYS, dwStickyKeysStructSize, (PVOID)&StickyKeysStruct, 0);
		SystemParametersInfo(SPI_GETFILTERKEYS, dwFilterKeysStructSize, (PVOID)&FilterKeysStruct, 0);
		SystemParametersInfo(SPI_GETTOGGLEKEYS, dwToggleKeysStructSize, (PVOID)&ToggleKeysStruct, 0);

		if (StickyKeysStruct.dwFlags & SKF_AVAILABLE)
		{
			// Disable StickyKeys feature
			dwStickyKeysFlags = StickyKeysStruct.dwFlags;
			StickyKeysStruct.dwFlags = 0;
			SystemParametersInfo(SPI_SETSTICKYKEYS, dwStickyKeysStructSize, (PVOID)&StickyKeysStruct, 0);
		}

		if (FilterKeysStruct.dwFlags & FKF_AVAILABLE)
		{
			// Disable FilterKeys feature
			dwFilterKeysFlags = FilterKeysStruct.dwFlags;
			FilterKeysStruct.dwFlags = 0;
			SystemParametersInfo(SPI_SETFILTERKEYS, dwFilterKeysStructSize, (PVOID)&FilterKeysStruct, 0);
		}

		if (ToggleKeysStruct.dwFlags & TKF_AVAILABLE)
		{
			// Disable FilterKeys feature
			dwToggleKeysFlags = ToggleKeysStruct.dwFlags;
			ToggleKeysStruct.dwFlags = 0;
			SystemParametersInfo(SPI_SETTOGGLEKEYS, dwToggleKeysStructSize, (PVOID)&ToggleKeysStruct, 0);
		}
	}

	~damn_keys_filter()
	{
		if (bScreenSaverState)
			// Restoring screen saver
			SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, TRUE, NULL, 0);

		if (dwStickyKeysFlags)
		{
			// Restore StickyKeys feature
			StickyKeysStruct.dwFlags = dwStickyKeysFlags;
			SystemParametersInfo(SPI_SETSTICKYKEYS, dwStickyKeysStructSize, (PVOID)&StickyKeysStruct, 0);
		}

		if (dwFilterKeysFlags)
		{
			// Restore FilterKeys feature
			FilterKeysStruct.dwFlags = dwFilterKeysFlags;
			SystemParametersInfo(SPI_SETFILTERKEYS, dwFilterKeysStructSize, (PVOID)&FilterKeysStruct, 0);
		}

		if (dwToggleKeysFlags)
		{
			// Restore FilterKeys feature
			ToggleKeysStruct.dwFlags = dwToggleKeysFlags;
			SystemParametersInfo(SPI_SETTOGGLEKEYS, dwToggleKeysStructSize, (PVOID)&ToggleKeysStruct, 0);
		}
	}
};

#undef dwStickyKeysStructSize
#undef dwFilterKeysStructSize
#undef dwToggleKeysStructSize

#include "xr_ioc_cmd.h"

//typedef void DUMMY_STUFF (const void*,const u32&,void*);
//XRCORE_API DUMMY_STUFF *g_temporary_stuff;

//#define TRIVIAL_ENCRYPTOR_DECODER
//#include "trivial_encryptor.h"

//#define RUSSIAN_BUILD

#if 0
void foo()
{
    typedef std::map<int, int> TEST_MAP;
    TEST_MAP temp;
    temp.insert(std::make_pair(0, 0));
    TEST_MAP::const_iterator I = temp.upper_bound(2);
    if (I == temp.end())
        OutputDebugString("end() returned\r\n");
    else
        OutputDebugString("last element returned\r\n");

    typedef void* pvoid;

    LPCSTR path = "d:\\network\\stalker_net2";
    FILE* f = fopen(path, "rb");
    int file_handle = _fileno(f);
    u32 buffer_size = _filelength(file_handle);
    pvoid buffer = xr_malloc(buffer_size);
    size_t result = fread(buffer, buffer_size, 1, f);
    R_ASSERT3(!buffer_size || (result && (buffer_size >= result)), "Cannot read from file", path);
    fclose(f);

    u32 compressed_buffer_size = rtc_csize(buffer_size);
    pvoid compressed_buffer = xr_malloc(compressed_buffer_size);
    u32 compressed_size = rtc_compress(compressed_buffer, compressed_buffer_size, buffer, buffer_size);

    LPCSTR compressed_path = "d:\\network\\stalker_net2.rtc";
    FILE* f1 = fopen(compressed_path, "wb");
    fwrite(compressed_buffer, compressed_size, 1, f1);
    fclose(f1);
}
#endif // 0

ENGINE_API bool g_dedicated_server = false;

#ifndef DEDICATED_SERVER

#endif // DEDICATED_SERVER

int APIENTRY WinMain_impl(HINSTANCE hInstance,
                          HINSTANCE hPrevInstance,
                          char* lpCmdLine,
                          int nCmdShow)
{
#ifdef DEDICATED_SERVER
    Debug._initialize(true);
#else // DEDICATED_SERVER
	Debug._initialize(false);
#endif // DEDICATED_SERVER

	if (!IsDebuggerPresent())
	{
		HMODULE const kernel32 = LoadLibrary("kernel32.dll");
		R_ASSERT(kernel32);

		typedef BOOL (__stdcall*HeapSetInformation_type)(HANDLE, HEAP_INFORMATION_CLASS, PVOID, SIZE_T);
		HeapSetInformation_type const heap_set_information =
			(HeapSetInformation_type)GetProcAddress(kernel32, "HeapSetInformation");
		if (heap_set_information)
		{
			ULONG HeapFragValue = 2;
#ifdef DEBUG
            BOOL const result =
#endif // #ifdef DEBUG
			heap_set_information(
				GetProcessHeap(),
				HeapCompatibilityInformation,
				&HeapFragValue,
				sizeof(HeapFragValue)
			);
#ifdef DEBUG
			VERIFY2(result, "can't set process heap low fragmentation");
#endif
		}
	}

	// foo();
#ifndef DEDICATED_SERVER

	// Check for another instance
#ifdef NO_MULTI_INSTANCES
#define STALKER_PRESENCE_MUTEX "Local\\STALKER-COP"

	HANDLE hCheckPresenceMutex = INVALID_HANDLE_VALUE;
	hCheckPresenceMutex = OpenMutex(READ_CONTROL, FALSE, STALKER_PRESENCE_MUTEX);
	if (hCheckPresenceMutex == NULL)
	{
		// New mutex
		hCheckPresenceMutex = CreateMutex(NULL, FALSE, STALKER_PRESENCE_MUTEX);
		if (hCheckPresenceMutex == NULL)
			// Shit happens
			return 2;
	}
	else
	{
		// Already running
		CloseHandle(hCheckPresenceMutex);
		return 1;
	}
#endif
#else // DEDICATED_SERVER
    g_dedicated_server = true;
#endif // DEDICATED_SERVER

	// Title window
	logoWindow = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_STARTUP), 0, logDlgProc);

	HWND logoPicture = GetDlgItem(logoWindow, IDC_STATIC_LOGO);
	RECT logoRect;
	GetWindowRect(logoPicture, &logoRect);

	SetWindowPos(
		logoWindow,
#ifndef DEBUG
		HWND_TOPMOST,
#else
        HWND_NOTOPMOST,
#endif // NDEBUG
		0,
		0,
		logoRect.right - logoRect.left,
		logoRect.bottom - logoRect.top,
		SWP_NOMOVE | SWP_SHOWWINDOW // | SWP_NOSIZE
	);

	UpdateWindow(logoWindow);

	// AVI
	g_bIntroFinished = TRUE;

	g_sLaunchOnExit_app[0] = NULL;
	g_sLaunchOnExit_params[0] = NULL;

	LPCSTR fsgame_ltx_name = "-fsltx ";
	string_path fsgame = "";
	//MessageBox(0, lpCmdLine, "my cmd string", MB_OK);
	if (strstr(lpCmdLine, fsgame_ltx_name))
	{
		int sz = xr_strlen(fsgame_ltx_name);
		sscanf(strstr(lpCmdLine, fsgame_ltx_name) + sz, "%[^ ] ", fsgame);
		//MessageBox(0, fsgame, "using fsltx", MB_OK);
	}

	// g_temporary_stuff = &trivial_encryptor::decode;

	compute_build_id();
	Core._initialize("xray", NULL, TRUE, fsgame[0] ? fsgame : NULL);

	InitSettings();
	Msg(XRAY_MONOLITH_VERSION);

	{
		FS_FileSet fset;
		FS.file_list(fset, "$game_data$", FS_ListFiles, "*");

		// list all files in gamedata folder
		u32 count = 0;
		for (FS_FileSet::iterator it = fset.begin(); it != fset.end(); it++)
		{
			// skip virtual files from .db? archives, only interested in loose files
			if ((*it).attrib != 0) continue;
			Msg("gamedata: '%s'", (*it).name.c_str());

			const u32 cutoff = 100;
			if (++count >= cutoff)
			{
				u32 total = fset.size();
				if (total > cutoff)
				{
					Msg("gamedata: ... %d more ...", total - cutoff);
				}
				break;
			}
		}
	}

	// Adjust player & computer name for Asian
	if (pSettings->line_exist("string_table", "no_native_input"))
	{
		xr_strcpy(Core.UserName, sizeof(Core.UserName), "Player");
		xr_strcpy(Core.CompName, sizeof(Core.CompName), "Computer");
	}

#ifndef DEDICATED_SERVER
	{
		damn_keys_filter filter;
		(void)filter;
#endif // DEDICATED_SERVER

		FPU::m24r();
		InitEngine();

		InitInput();

		InitConsole();

		Engine.External.CreateRendererList();

		LPCSTR benchName = "-batch_benchmark ";
		if (strstr(lpCmdLine, benchName))
		{
			int sz = xr_strlen(benchName);
			string64 b_name;
			sscanf(strstr(Core.Params, benchName) + sz, "%[^ ] ", b_name);
			doBenchmark(b_name);
			return 0;
		}

		extern bool ignore_verify;
		ignore_verify = !strstr(Core.Params, "-dbgdev");

		Msg("command line %s", Core.Params);
		LPCSTR sashName = "-openautomate ";
		if (strstr(lpCmdLine, sashName))
		{
			int sz = xr_strlen(sashName);
			string512 sash_arg;
			sscanf(strstr(Core.Params, sashName) + sz, "%[^ ] ", sash_arg);
			//doBenchmark (sash_arg);
			g_SASH.Init(sash_arg);
			g_SASH.MainLoop();
			return 0;
		}

		if (strstr(lpCmdLine, "-launcher"))
		{
			int l_res = doLauncher();
			if (l_res != 0)
				return 0;
		};

#ifndef DEDICATED_SERVER
		if (strstr(Core.Params, "-r2a"))
			Console->Execute("renderer renderer_r2a");
		else if (strstr(Core.Params, "-r2"))
			Console->Execute("renderer renderer_r2");
		else
		{
			CCC_LoadCFG_custom* pTmp = xr_new<CCC_LoadCFG_custom>("renderer ");
			pTmp->Execute(Console->ConfigFile);
			xr_delete(pTmp);
		}
#else
        Console->Execute("renderer renderer_r1");
#endif
		//. InitInput ( );
		Engine.External.Initialize();
		Console->Execute("stat_memory");

		Startup();
		Core._destroy();

		// check for need to execute something external
		if (/*xr_strlen(g_sLaunchOnExit_params) && */xr_strlen(g_sLaunchOnExit_app))
		{
			//CreateProcess need to return results to next two structures
			STARTUPINFO si;
			PROCESS_INFORMATION pi;
			ZeroMemory(&si, sizeof(si));
			si.cb = sizeof(si);
			ZeroMemory(&pi, sizeof(pi));
			//We use CreateProcess to setup working folder
			char const* temp_wf = (xr_strlen(g_sLaunchWorkingFolder) > 0) ? g_sLaunchWorkingFolder : NULL;
			CreateProcess(g_sLaunchOnExit_app, g_sLaunchOnExit_params, NULL, NULL, FALSE, 0, NULL,
			              temp_wf, &si, &pi);
		}
#ifndef DEDICATED_SERVER
#ifdef NO_MULTI_INSTANCES
		// Delete application presence mutex
		CloseHandle(hCheckPresenceMutex);
#endif
	}
	// here damn_keys_filter class instanse will be destroyed
#endif // DEDICATED_SERVER

	return 0;
}

int stack_overflow_exception_filter(int exception_code)
{
	if (exception_code == EXCEPTION_STACK_OVERFLOW)
	{
		// Do not call _resetstkoflw here, because
		// at this point, the stack is not yet unwound.
		// Instead, signal that the handler (the __except block)
		// is to be executed.
		return EXCEPTION_EXECUTE_HANDLER;
	}
	else
		return EXCEPTION_CONTINUE_SEARCH;
}

#include <boost/crc.hpp>

extern BOOL DllMainOpenAL32(HANDLE module, DWORD reason, LPVOID reserved);
extern BOOL DllMainXrCore(HANDLE hinstDLL, DWORD ul_reason_for_call, LPVOID lpvReserved);
extern BOOL DllMainXrPhysics(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved);

//extern BOOL DllMainXrGame(HANDLE hModule, u32 ul_reason_for_call, LPVOID lpReserved);
//
//extern BOOL DllMainXrRenderR1(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved);
//extern BOOL DllMainXrRenderR2(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved);
//extern BOOL DllMainXrRenderR3(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved);
//extern BOOL DllMainXrRenderR4(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved);

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     char* lpCmdLine,
                     int nCmdShow)
{
	DllMainOpenAL32(NULL, DLL_PROCESS_ATTACH, NULL);
	DllMainXrCore(NULL, DLL_PROCESS_ATTACH, NULL);
	DllMainXrPhysics(NULL, DLL_PROCESS_ATTACH, NULL);

	DllMainXrCore(NULL, DLL_THREAD_ATTACH, NULL);

	__try
	{
		WinMain_impl(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
	}
	__except (stack_overflow_exception_filter(GetExceptionCode()))
	{
		_resetstkoflw();
		FATAL("stack overflow");
	}

	DllMainXrPhysics(NULL, DLL_PROCESS_DETACH, NULL);
	DllMainXrCore(NULL, DLL_PROCESS_DETACH, NULL);
	DllMainOpenAL32(NULL, DLL_PROCESS_DETACH, NULL);

	return (0);
}

LPCSTR _GetFontTexName(LPCSTR section)
{
	static char* tex_names[] = {"texture800", "texture", "texture1600", "texture2160"};
	int def_idx = 1; //default 1024x768
	int idx = def_idx;

#if 0
    u32 w = Device.dwWidth;

    if (w <= 800) idx = 0;
    else if (w <= 1280)idx = 1;
    else idx = 2;
#else
	u32 h = Device.dwHeight;

	if (h <= 600) idx = 0;
	else if (h < 1024) idx = 1;
	else if (h < 1440) idx = 2;
	else idx = 3;
#endif

	while (idx >= 0)
	{
		if (pSettings->line_exist(section, tex_names[idx]))
			return pSettings->r_string(section, tex_names[idx]);
		--idx;
	}
	return pSettings->r_string(section, tex_names[def_idx]);
}

void _InitializeFont(CGameFont*& F, LPCSTR section, u32 flags)
{
	LPCSTR font_tex_name = _GetFontTexName(section);
	R_ASSERT(font_tex_name);

	LPCSTR sh_name = pSettings->r_string(section, "shader");
	if (!F)
	{
		F = xr_new<CGameFont>(sh_name, font_tex_name, flags);
	}
	else
		F->Initialize(sh_name, font_tex_name);

	if (pSettings->line_exist(section, "size"))
	{
		float sz = pSettings->r_float(section, "size");
		if (flags & CGameFont::fsDeviceIndependent) F->SetHeightI(sz);
		else F->SetHeight(sz);
	}
	if (pSettings->line_exist(section, "interval"))
		F->SetInterval(pSettings->r_fvector2(section, "interval"));
}

CApplication::CApplication()
{
	ll_dwReference = 0;

	max_load_stage = 0;

	// events
	eQuit = Engine.Event.Handler_Attach("KERNEL:quit", this);
	eStart = Engine.Event.Handler_Attach("KERNEL:start", this);
	eStartLoad = Engine.Event.Handler_Attach("KERNEL:load", this);
	eDisconnect = Engine.Event.Handler_Attach("KERNEL:disconnect", this);
	eConsole = Engine.Event.Handler_Attach("KERNEL:console", this);
	eStartMPDemo = Engine.Event.Handler_Attach("KERNEL:start_mp_demo", this);

	// levels
	Level_Current = u32(-1);
	Level_Scan();

	// Font
	pFontSystem = NULL;

	// Register us
	Device.seqFrame.Add(this, REG_PRIORITY_HIGH + 1000);

	if (psDeviceFlags.test(mtSound)) Device.seqFrameMT.Add(&SoundProcessor);
	else Device.seqFrame.Add(&SoundProcessor);

	Console->Show();

	// App Title
	// app_title[ 0 ] = '\0';
	ls_header[0] = '\0';
	ls_tip_number[0] = '\0';
	ls_tip[0] = '\0';
}

CApplication::~CApplication()
{
	Console->Hide();

	// font
	xr_delete(pFontSystem);

	Device.seqFrameMT.Remove(&SoundProcessor);
	Device.seqFrame.Remove(&SoundProcessor);
	Device.seqFrame.Remove(this);


	// events
	Engine.Event.Handler_Detach(eConsole, this);
	Engine.Event.Handler_Detach(eDisconnect, this);
	Engine.Event.Handler_Detach(eStartLoad, this);
	Engine.Event.Handler_Detach(eStart, this);
	Engine.Event.Handler_Detach(eQuit, this);
	Engine.Event.Handler_Detach(eStartMPDemo, this);
}

extern CRenderDevice Device;

void CApplication::OnEvent(EVENT E, u64 P1, u64 P2)
{
	if (E == eQuit)
	{
		g_SASH.EndBenchmark();

		PostQuitMessage(0);

		for (u32 i = 0; i < Levels.size(); i++)
		{
			xr_free(Levels[i].folder);
			xr_free(Levels[i].name);
		}
	}
	else if (E == eStart)
	{
		LPSTR op_server = LPSTR(P1);
		LPSTR op_client = LPSTR(P2);
		Level_Current = u32(-1);
		R_ASSERT(0 == g_pGameLevel);
		R_ASSERT(0 != g_pGamePersistent);

#ifdef NO_SINGLE
        Console->Execute("main_menu on");
        if ((op_server == NULL) ||
                (!xr_strlen(op_server)) ||
                (
                    (strstr(op_server, "/dm") || strstr(op_server, "/deathmatch") ||
                     strstr(op_server, "/tdm") || strstr(op_server, "/teamdeathmatch") ||
                     strstr(op_server, "/ah") || strstr(op_server, "/artefacthunt") ||
                     strstr(op_server, "/cta") || strstr(op_server, "/capturetheartefact")
                    ) &&
                    !strstr(op_server, "/alife")
                )
           )
#endif // #ifdef NO_SINGLE
		{
			Console->Execute("main_menu off");
			Console->Hide();
			//! this line is commented by Dima
			//! because I don't see any reason to reset device here
			//! Device.Reset (false);
			//-----------------------------------------------------------
			g_pGamePersistent->PreStart(op_server);
			//-----------------------------------------------------------
			g_pGameLevel = (IGame_Level*)NEW_INSTANCE(CLSID_GAME_LEVEL);
			pApp->LoadBegin();
			g_pGamePersistent->Start(op_server);
			g_pGameLevel->net_Start(op_server, op_client);
			pApp->LoadEnd();
		}
		xr_free(op_server);
		xr_free(op_client);
	}
	else if (E == eDisconnect)
	{
		ls_header[0] = '\0';
		ls_tip_number[0] = '\0';
		ls_tip[0] = '\0';

		if (g_pGameLevel)
		{
			Console->Hide();
			g_pGameLevel->net_Stop();
			DEL_INSTANCE(g_pGameLevel);
			Console->Show();

			if ((FALSE == Engine.Event.Peek("KERNEL:quit")) && (FALSE == Engine.Event.Peek("KERNEL:start")))
			{
				Console->Execute("main_menu off");
				Console->Execute("main_menu on");
			}
		}
		R_ASSERT(0 != g_pGamePersistent);
		g_pGamePersistent->Disconnect();
	}
	else if (E == eConsole)
	{
		LPSTR command = (LPSTR)P1;
		Console->ExecuteCommand(command, false);
		xr_free(command);
	}
	else if (E == eStartMPDemo)
	{
		LPSTR demo_file = LPSTR(P1);

		R_ASSERT(0 == g_pGameLevel);
		R_ASSERT(0 != g_pGamePersistent);

		Console->Execute("main_menu off");
		Console->Hide();
		Device.Reset(false);

		g_pGameLevel = (IGame_Level*)NEW_INSTANCE(CLSID_GAME_LEVEL);
		shared_str server_options = g_pGameLevel->OpenDemoFile(demo_file);

		//-----------------------------------------------------------
		g_pGamePersistent->PreStart(server_options.c_str());
		//-----------------------------------------------------------

		pApp->LoadBegin();
		g_pGamePersistent->Start(""); //server_options.c_str()); - no prefetch !
		g_pGameLevel->net_StartPlayDemo();
		pApp->LoadEnd();

		xr_free(demo_file);
	}
}

static CTimer phase_timer;
extern ENGINE_API BOOL g_appLoaded = FALSE;
//AVO: used by SPAWN_ANTIFREEZE (by alpet)
extern ENGINE_API BOOL g_bootComplete = FALSE;
//-AVO

void CApplication::LoadBegin()
{
	ll_dwReference++;
	if (1 == ll_dwReference)
	{
		g_appLoaded = FALSE;

		//AVO:
		g_bootComplete = FALSE;
		//-AVO

#ifndef DEDICATED_SERVER
		_InitializeFont(pFontSystem, "ui_font_letterica18_russian", 0);

		m_pRender->LoadBegin();
#endif
		phase_timer.Start();
		load_stage = 0;
	}
}

void CApplication::LoadEnd()
{
	ll_dwReference--;
	if (0 == ll_dwReference)
	{
		Msg("* phase time: %d ms", phase_timer.GetElapsed_ms());
		Msg("* phase cmem: %lld K", Memory.mem_usage() / 1024);
		Console->Execute("stat_memory");
		g_appLoaded = TRUE;
		// DUMP_PHASE;
	}
}

void CApplication::destroy_loading_shaders()
{
	m_pRender->destroy_loading_shaders();

	//AVO:
	g_bootComplete = TRUE;
	//-AVO

	//hLevelLogo.destroy ();
	//sh_progress.destroy ();
	//. ::Sound->mute (false);
}

//u32 calc_progress_color(u32, u32, int, int);

PROTECT_API void CApplication::LoadDraw()
{
	PROF_EVENT();

	if (g_appLoaded) return;
	Device.dwFrame += 1;


	if (!Device.Begin()) return;

	if (g_dedicated_server)
		Console->OnRender();
	else
		load_draw_internal();

	Device.End();
}

void CApplication::LoadTitleInt(LPCSTR str1, LPCSTR str2, LPCSTR str3)
{
	xr_strcpy(ls_header, str1);
	xr_strcpy(ls_tip_number, str2);
	xr_strcpy(ls_tip, str3);
	// LoadDraw ();
}

void CApplication::LoadStage()
{
	load_stage++;
	VERIFY(ll_dwReference);
	Msg("* phase time: %d ms", phase_timer.GetElapsed_ms());
	phase_timer.Start();
	Msg("* phase cmem: %lld K", Memory.mem_usage() / 1024);

	if (g_pGamePersistent->GameType() == 1 && !xr_strcmp(g_pGamePersistent->m_game_params.m_alife, "alife"))
		max_load_stage = 17;
	else
		max_load_stage = 14;
	LoadDraw();
}

void CApplication::LoadSwitch()
{
}

// Sequential
void CApplication::OnFrame()
{
	PROF_EVENT();

	Engine.Event.OnFrame();
	g_SpatialSpace->update();
	g_SpatialSpacePhysic->update();
	if (g_pGameLevel)
		g_pGameLevel->SoundEvent_Dispatch();
}

void CApplication::Level_Append(LPCSTR folder)
{
	PROF_EVENT();

	string_path N1, N2, N3, N4;
	strconcat(sizeof(N1), N1, folder, "level");
	strconcat(sizeof(N2), N2, folder, "level.ltx");
	strconcat(sizeof(N3), N3, folder, "level.geom");
	strconcat(sizeof(N4), N4, folder, "level.cform");
	if (
		FS.exist("$game_levels$", N1) &&
		FS.exist("$game_levels$", N2) &&
		FS.exist("$game_levels$", N3) &&
		FS.exist("$game_levels$", N4)
	)
	{
		sLevelInfo LI;
		LI.folder = xr_strdup(folder);
		LI.name = 0;
		Levels.push_back(LI);
	}
}

void CApplication::Level_Scan()
{
	PROF_EVENT();

	//SECUROM_MARKER_PERFORMANCE_ON(8)

	for (u32 i = 0; i < Levels.size(); i++)
	{
		xr_free(Levels[i].folder);
		xr_free(Levels[i].name);
	}
	Levels.clear();


	xr_vector<char*>* folder = FS.file_list_open("$game_levels$", FS_ListFolders | FS_RootOnly);
	//. R_ASSERT (folder&&folder->size());

	for (u32 i = 0; i < folder->size(); ++i)
		Level_Append((*folder)[i]);

	FS.file_list_close(folder);

	//SECUROM_MARKER_PERFORMANCE_OFF(8)
}

void gen_logo_name(string_path& dest, LPCSTR level_name, int num)
{
	PROF_EVENT();

	strconcat(sizeof(dest), dest, "intro\\intro_", level_name);

	u32 len = xr_strlen(dest);
	if (dest[len - 1] == '\\')
		dest[len - 1] = 0;

	string16 buff;
	xr_strcat(dest, sizeof(dest), "_");
	xr_strcat(dest, sizeof(dest), itoa(num + 1, buff, 10));
}

void CApplication::Level_Set(u32 L)
{
	PROF_EVENT();

	//SECUROM_MARKER_PERFORMANCE_ON(9)

	if (L >= Levels.size()) return;
	FS.get_path("$level$")->_set(Levels[L].folder);

	static string_path path;

	if (Level_Current != L)
	{
		path[0] = 0;

		Level_Current = L;

		int count = 0;
		while (true)
		{
			string_path temp2;
			gen_logo_name(path, Levels[L].folder, count);
			if (FS.exist(temp2, "$game_textures$", path, ".dds") || FS.exist(temp2, "$level$", path, ".dds"))
				count++;
			else
				break;
		}

		if (count)
		{
			int num = ::Random.randI(count);
			gen_logo_name(path, Levels[L].folder, num);
		}
	}

	if (path[0])
		m_pRender->setLevelLogo(path);

	//SECUROM_MARKER_PERFORMANCE_OFF(9)
}

int CApplication::Level_ID(LPCSTR name, LPCSTR ver, bool bSet)
{
	PROF_EVENT();

	int result = -1;

	////SECUROM_MARKER_SECURITY_ON(7)

	CLocatorAPI::archives_it it = FS.m_archives.begin();
	CLocatorAPI::archives_it it_e = FS.m_archives.end();
	bool arch_res = false;

	for (; it != it_e; ++it)
	{
		CLocatorAPI::archive& A = *it;
		if (A.hSrcFile == NULL)
		{
			LPCSTR ln = A.header->r_string("header", "level_name");
			LPCSTR lv = A.header->r_string("header", "level_ver");
			if (0 == stricmp(ln, name) && 0 == stricmp(lv, ver))
			{
				FS.LoadArchive(A);
				arch_res = true;
			}
		}
	}

	if (arch_res)
		Level_Scan();

	string256 buffer;
	strconcat(sizeof(buffer), buffer, name, "\\");
	for (u32 I = 0; I < Levels.size(); ++I)
	{
		if (0 == stricmp(buffer, Levels[I].folder))
		{
			result = int(I);
			break;
		}
	}

	if (bSet && result != -1)
		Level_Set(result);

	if (arch_res)
		g_pGamePersistent->OnAssetsChanged();

	////SECUROM_MARKER_SECURITY_OFF(7)

	return result;
}

CInifile* CApplication::GetArchiveHeader(LPCSTR name, LPCSTR ver)
{
	PROF_EVENT();

	CLocatorAPI::archives_it it = FS.m_archives.begin();
	CLocatorAPI::archives_it it_e = FS.m_archives.end();

	for (; it != it_e; ++it)
	{
		CLocatorAPI::archive& A = *it;

		LPCSTR ln = A.header->r_string("header", "level_name");
		LPCSTR lv = A.header->r_string("header", "level_ver");
		if (0 == stricmp(ln, name) && 0 == stricmp(lv, ver))
		{
			return A.header;
		}
	}
	return NULL;
}

void CApplication::LoadAllArchives()
{
	PROF_EVENT();

	if (FS.load_all_unloaded_archives())
	{
		Level_Scan();
		g_pGamePersistent->OnAssetsChanged();
	}
}

//launcher stuff----------------------------
extern "C" {
typedef int __cdecl LauncherFunc(int);
}

HMODULE hLauncher = NULL;
LauncherFunc* pLauncher = NULL;

void InitLauncher()
{
	if (hLauncher)
		return;
	hLauncher = LoadLibrary("xrLauncher.dll");
	if (0 == hLauncher)
		R_CHK(GetLastError());
	R_ASSERT2(hLauncher, "xrLauncher DLL raised exception during loading or there is no xrLauncher.dll at all");

	pLauncher = (LauncherFunc*)GetProcAddress(hLauncher, "RunXRLauncher");
	R_ASSERT2(pLauncher, "Cannot obtain RunXRLauncher function from xrLauncher.dll");
};

void FreeLauncher()
{
	if (hLauncher)
	{
		FreeLibrary(hLauncher);
		hLauncher = NULL;
		pLauncher = NULL;
	};
}

int doLauncher()
{
	/*
	execUserScript();
	InitLauncher();
	int res = pLauncher(0);
	FreeLauncher();
	if(res == 1) // do benchmark
	g_bBenchmark = true;

	if(g_bBenchmark){ //perform benchmark cycle
	doBenchmark();

	// InitLauncher ();
	// pLauncher (2); //show results
	// FreeLauncher ();

	Core._destroy ();
	return (1);

	};
	if(res==8){//Quit
	Core._destroy ();
	return (1);
	}
	*/
	return 0;
}

void doBenchmark(LPCSTR name)
{
	g_bBenchmark = true;
	string_path in_file;
	FS.update_path(in_file, "$app_data_root$", name);
	CInifile ini(in_file);
	int test_count = ini.line_count("benchmark");
	LPCSTR test_name, t;
	shared_str test_command;
	for (int i = 0; i < test_count; ++i)
	{
		ini.r_line("benchmark", i, &test_name, &t);
		xr_strcpy(g_sBenchmarkName, test_name);

		test_command = ini.r_string_wb("benchmark", test_name);
		u32 cmdSize = test_command.size() + 1;
		Core.Params = (char*)xr_realloc(Core.Params, cmdSize);
		xr_strcpy(Core.Params, cmdSize, test_command.c_str());
		xr_strlwr(Core.Params);

		InitInput();
		if (i)
		{
			//ZeroMemory(&HW,sizeof(CHW));
			// TODO: KILL HW here!
			// pApp->m_pRender->KillHW();
			InitEngine();
		}


		Engine.External.Initialize();

		xr_strcpy(Console->ConfigFile, "user.ltx");
		if (strstr(Core.Params, "-ltx "))
		{
			string64 c_name;
			sscanf(strstr(Core.Params, "-ltx ") + 5, "%[^ ] ", c_name);
			xr_strcpy(Console->ConfigFile, c_name);
		}

		Startup();
	}
}
#pragma optimize("g", off)
void CApplication::load_draw_internal()
{
	m_pRender->load_draw_internal(*this);
	/*
	if(!sh_progress){
	CHK_DX (HW.pDevice->Clear(0,0,D3DCLEAR_TARGET,D3DCOLOR_ARGB(0,0,0,0),1,0));
	return;
	}
	// Draw logo
	u32 Offset;
	u32 C = 0xffffffff;
	u32 _w = Device.dwWidth;
	u32 _h = Device.dwHeight;
	FVF::TL* pv = NULL;

	//progress
	float bw = 1024.0f;
	float bh = 768.0f;
	Fvector2 k; k.set(float(_w)/bw, float(_h)/bh);

	RCache.set_Shader (sh_progress);
	CTexture* T = RCache.get_ActiveTexture(0);
	Fvector2 tsz;
	tsz.set ((float)T->get_Width(),(float)T->get_Height());
	Frect back_text_coords;
	Frect back_coords;
	Fvector2 back_size;

	//progress background
	static float offs = -0.5f;

	back_size.set (1024,768);
	back_text_coords.lt.set (0,0);back_text_coords.rb.add(back_text_coords.lt,back_size);
	back_coords.lt.set (offs, offs); back_coords.rb.add(back_coords.lt,back_size);

	back_coords.lt.mul (k);back_coords.rb.mul(k);

	back_text_coords.lt.x/=tsz.x; back_text_coords.lt.y/=tsz.y; back_text_coords.rb.x/=tsz.x; back_text_coords.rb.y/=tsz.y;
	pv = (FVF::TL*) RCache.Vertex.Lock(4,ll_hGeom.stride(),Offset);
	pv->set (back_coords.lt.x, back_coords.rb.y, C,back_text_coords.lt.x, back_text_coords.rb.y); pv++;
	pv->set (back_coords.lt.x, back_coords.lt.y, C,back_text_coords.lt.x, back_text_coords.lt.y); pv++;
	pv->set (back_coords.rb.x, back_coords.rb.y, C,back_text_coords.rb.x, back_text_coords.rb.y); pv++;
	pv->set (back_coords.rb.x, back_coords.lt.y, C,back_text_coords.rb.x, back_text_coords.lt.y); pv++;
	RCache.Vertex.Unlock (4,ll_hGeom.stride());

	RCache.set_Geometry (ll_hGeom);
	RCache.Render (D3DPT_TRIANGLELIST,Offset,0,4,0,2);

	//progress bar
	back_size.set (268,37);
	back_text_coords.lt.set (0,768);back_text_coords.rb.add(back_text_coords.lt,back_size);
	back_coords.lt.set (379 ,726);back_coords.rb.add(back_coords.lt,back_size);

	back_coords.lt.mul (k);back_coords.rb.mul(k);

	back_text_coords.lt.x/=tsz.x; back_text_coords.lt.y/=tsz.y; back_text_coords.rb.x/=tsz.x; back_text_coords.rb.y/=tsz.y;



	u32 v_cnt = 40;
	pv = (FVF::TL*)RCache.Vertex.Lock (2*(v_cnt+1),ll_hGeom2.stride(),Offset);
	FVF::TL* _pv = pv;
	float pos_delta = back_coords.width()/v_cnt;
	float tc_delta = back_text_coords.width()/v_cnt;
	u32 clr = C;

	for(u32 idx=0; idx<v_cnt+1; ++idx){
	clr = calc_progress_color(idx,v_cnt,load_stage,max_load_stage);
	pv->set (back_coords.lt.x+pos_delta*idx+offs, back_coords.rb.y+offs, 0+EPS_S, 1, clr, back_text_coords.lt.x+tc_delta*idx, back_text_coords.rb.y); pv++;
	pv->set (back_coords.lt.x+pos_delta*idx+offs, back_coords.lt.y+offs, 0+EPS_S, 1, clr, back_text_coords.lt.x+tc_delta*idx, back_text_coords.lt.y); pv++;
	}
	VERIFY (u32(pv-_pv)==2*(v_cnt+1));
	RCache.Vertex.Unlock (2*(v_cnt+1),ll_hGeom2.stride());

	RCache.set_Geometry (ll_hGeom2);
	RCache.Render (D3DPT_TRIANGLESTRIP, Offset, 2*v_cnt);


	// Draw title
	VERIFY (pFontSystem);
	pFontSystem->Clear ();
	pFontSystem->SetColor (color_rgba(157,140,120,255));
	pFontSystem->SetAligment (CGameFont::alCenter);
	pFontSystem->OutI (0.f,0.815f,app_title);
	pFontSystem->OnRender ();


	//draw level-specific screenshot
	if(hLevelLogo){
	Frect r;
	r.lt.set (257,369);
	r.lt.x += offs;
	r.lt.y += offs;
	r.rb.add (r.lt,Fvector2().set(512,256));
	r.lt.mul (k);
	r.rb.mul (k);
	pv = (FVF::TL*) RCache.Vertex.Lock(4,ll_hGeom.stride(),Offset);
	pv->set (r.lt.x, r.rb.y, C, 0, 1); pv++;
	pv->set (r.lt.x, r.lt.y, C, 0, 0); pv++;
	pv->set (r.rb.x, r.rb.y, C, 1, 1); pv++;
	pv->set (r.rb.x, r.lt.y, C, 1, 0); pv++;
	RCache.Vertex.Unlock (4,ll_hGeom.stride());

	RCache.set_Shader (hLevelLogo);
	RCache.set_Geometry (ll_hGeom);
	RCache.Render (D3DPT_TRIANGLELIST,Offset,0,4,0,2);
	}
	*/
}

/*
u32 calc_progress_color(u32 idx, u32 total, int stage, int max_stage)
{
if(idx>(total/2))
idx = total-idx;


float kk = (float(stage+1)/float(max_stage))*(total/2.0f);
float f = 1/(exp((float(idx)-kk)*0.5f)+1.0f);

return color_argb_f (f,1.0f,1.0f,1.0f);
}
*/

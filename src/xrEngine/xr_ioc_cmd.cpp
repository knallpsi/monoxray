#include "stdafx.h"
#include "igame_level.h"

//#include "xr_effgamma.h"
#include "x_ray.h"
#include "xr_ioconsole.h"
#include "xr_ioc_cmd.h"
//#include "fbasicvisual.h"
#include "cameramanager.h"
#include "environment.h"
#include "xr_input.h"
#include "CustomHUD.h"

#include "../Include/xrRender/RenderDeviceRender.h"

#include "xr_object.h"

xr_token* vid_quality_token = NULL;

u32 g_screenmode = 1;
xr_token screen_mode_tokens[] =
{
	{"fullscreen", 2},
	{"borderless", 1},
	{"windowed", 0},
	{0, 0}
};

xr_token vid_bpp_token[] =
{
	{"16", 16},
	{"32", 32},
	{0, 0}
};
//-----------------------------------------------------------------------

void IConsole_Command::add_to_LRU(shared_str const& arg)
{
	if (arg.size() == 0 || bEmptyArgsHandled)
	{
		return;
	}

	bool dup = (std::find(m_LRU.begin(), m_LRU.end(), arg) != m_LRU.end());
	if (!dup)
	{
		m_LRU.push_back(arg);
		if (m_LRU.size() > LRU_MAX_COUNT)
		{
			m_LRU.erase(m_LRU.begin());
		}
	}
}

void IConsole_Command::add_LRU_to_tips(vecTips& tips)
{
	vecLRU::reverse_iterator it_rb = m_LRU.rbegin();
	vecLRU::reverse_iterator it_re = m_LRU.rend();
	for (; it_rb != it_re; ++it_rb)
	{
		tips.push_back((*it_rb));
	}
}

// =======================================================

class CCC_Quit : public IConsole_Command
{
public:
	CCC_Quit(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };

	virtual void Execute(LPCSTR args)
	{
		// TerminateProcess(GetCurrentProcess(),0);
		Console->Hide();
		Engine.Event.Defer("KERNEL:disconnect");
		Engine.Event.Defer("KERNEL:quit");
	}
};

//-----------------------------------------------------------------------
#ifdef DEBUG_MEMORY_MANAGER
class CCC_MemStat : public IConsole_Command
{
public:
    CCC_MemStat(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
    virtual void Execute(LPCSTR args)
    {
        string_path fn;
        if (args&&args[0]) xr_sprintf(fn, sizeof(fn), "%s.dump", args);
        else strcpy_s_s(fn, sizeof(fn), "x:\\$memory$.dump");
        Memory.mem_statistic(fn);
        // g_pStringContainer->dump ();
        // g_pSharedMemoryContainer->dump ();
    }
};
#endif // DEBUG_MEMORY_MANAGER

#ifdef DEBUG_MEMORY_MANAGER
class CCC_DbgMemCheck : public IConsole_Command
{
public:
    CCC_DbgMemCheck(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
    virtual void Execute(LPCSTR args) { if (Memory.debug_mode) { Memory.dbg_check(); } else { Msg("~ Run with -mem_debug options."); } }
};
#endif // DEBUG_MEMORY_MANAGER

class CCC_DbgStrCheck : public IConsole_Command
{
public:
	CCC_DbgStrCheck(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) { g_pStringContainer->verify(); }
};

class CCC_DbgStrDump : public IConsole_Command
{
public:
	CCC_DbgStrDump(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
	virtual void Execute(LPCSTR args) { g_pStringContainer->dump(); }
};

//-----------------------------------------------------------------------
class CCC_MotionsStat : public IConsole_Command
{
public:
	CCC_MotionsStat(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };

	virtual void Execute(LPCSTR args)
	{
		//g_pMotionsContainer->dump();
		// TODO: move this console commant into renderer
		VERIFY(0);
	}
};

class CCC_TexturesStat : public IConsole_Command
{
public:
	CCC_TexturesStat(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };

	virtual void Execute(LPCSTR args)
	{
		Device.DumpResourcesMemoryUsage();
		//Device.Resources->_DumpMemoryUsage();
		// TODO: move this console commant into renderer
		//VERIFY(0);
	}
};

//-----------------------------------------------------------------------
class CCC_E_Dump : public IConsole_Command
{
public:
	CCC_E_Dump(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };

	virtual void Execute(LPCSTR args)
	{
		Engine.Event.Dump();
	}
};

class CCC_E_Signal : public IConsole_Command
{
public:
	CCC_E_Signal(LPCSTR N) : IConsole_Command(N)
	{
	};

	virtual void Execute(LPCSTR args)
	{
		char Event[128], Param[128];
		Event[0] = 0;
		Param[0] = 0;
		sscanf(args, "%[^,],%s", Event, Param);
		Engine.Event.Signal(Event, (u64)Param);
	}
};


//-----------------------------------------------------------------------
class CCC_Help : public IConsole_Command
{
public:
	CCC_Help(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };

	virtual void Execute(LPCSTR args)
	{
		Log("- --- Command listing: start ---");
		CConsole::vecCMD_IT it;
		for (it = Console->Commands.begin(); it != Console->Commands.end(); it++)
		{
			IConsole_Command& C = *(it->second);
			TStatus _S;
			C.Status(_S);
			TInfo _I;
			C.Info(_I);

			Msg("%-20s (%-10s) --- %s", C.Name(), _S, _I);
		}
		Log("Key: Ctrl + A         === Select all ");
		Log("Key: Ctrl + C         === Copy to clipboard ");
		Log("Key: Ctrl + V         === Paste from clipboard ");
		Log("Key: Ctrl + X         === Cut to clipboard ");
		Log("Key: Ctrl + Z         === Undo ");
		Log("Key: Ctrl + Insert    === Copy to clipboard ");
		Log("Key: Shift + Insert   === Paste from clipboard ");
		Log("Key: Shift + Delete   === Cut to clipboard ");
		Log("Key: Insert           === Toggle mode <Insert> ");
		Log("Key: Back / Delete          === Delete symbol left / right ");

		Log("Key: Up   / Down            === Prev / Next command in tips list ");
		Log("Key: Ctrl + Up / Ctrl + Down === Prev / Next executing command ");
		Log("Key: Left, Right, Home, End {+Shift/+Ctrl}       === Navigation in text ");
		Log("Key: PageUp / PageDown      === Scrolling history ");
		Log("Key: Tab  / Shift + Tab     === Next / Prev possible command from list");
		Log("Key: Enter  / NumEnter      === Execute current command ");

		Log("- --- Command listing: end ----");
	}
};


XRCORE_API void _dump_open_files(int mode);

class CCC_DumpOpenFiles : public IConsole_Command
{
public:
	CCC_DumpOpenFiles(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = FALSE; };

	virtual void Execute(LPCSTR args)
	{
		int _mode = atoi(args);
		_dump_open_files(_mode);
	}
};

//-----------------------------------------------------------------------
class CCC_SaveCFG : public IConsole_Command
{
public:
	CCC_SaveCFG(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };

	virtual void Execute(LPCSTR args)
	{
		string_path cfg_full_name;
		xr_strcpy(cfg_full_name, (xr_strlen(args) > 0) ? args : Console->ConfigFile);

		bool b_abs_name = xr_strlen(cfg_full_name) > 2 && cfg_full_name[1] == ':';

		if (!b_abs_name)
			FS.update_path(cfg_full_name, "$app_data_root$", cfg_full_name);

		if (strext(cfg_full_name))
			*strext(cfg_full_name) = 0;
		xr_strcat(cfg_full_name, ".ltx");

		BOOL b_allow = TRUE;
		if (FS.exist(cfg_full_name))
			b_allow = SetFileAttributes(cfg_full_name, FILE_ATTRIBUTE_NORMAL);

		if (b_allow)
		{
			IWriter* F = FS.w_open(cfg_full_name);
			CConsole::vecCMD_IT it;
			for (it = Console->Commands.begin(); it != Console->Commands.end(); it++)
				it->second->Save(F);
			FS.w_close(F);
			Msg("Config-file [%s] saved successfully", cfg_full_name);
		}
		else
			Msg("!Cannot store config file [%s]", cfg_full_name);
	}
};

CCC_LoadCFG::CCC_LoadCFG(LPCSTR N) : IConsole_Command(N)
{
};

void CCC_LoadCFG::Execute(LPCSTR args)
{
	Msg("Executing config-script \"%s\"...", args);
	string_path cfg_name;

	xr_strcpy(cfg_name, args);
	if (strext(cfg_name)) *strext(cfg_name) = 0;
	xr_strcat(cfg_name, ".ltx");

	string_path cfg_full_name;

	FS.update_path(cfg_full_name, "$app_data_root$", cfg_name);

	if (NULL == FS.exist(cfg_full_name))
		FS.update_path(cfg_full_name, "$fs_root$", cfg_name);

	if (NULL == FS.exist(cfg_full_name))
		xr_strcpy(cfg_full_name, cfg_name);

	IReader* F = FS.r_open(cfg_full_name);

	string1024 str;
	if (F != NULL)
	{
		while (!F->eof())
		{
			F->r_string(str, sizeof(str));
			if (allow(str))
				Console->Execute(str);
		}
		FS.r_close(F);
		Msg("[%s] successfully loaded.", cfg_full_name);
	}
	else
	{
		Msg("! Cannot open script file [%s]", cfg_full_name);
	}
}

CCC_LoadCFG_custom::CCC_LoadCFG_custom(LPCSTR cmd)
	: CCC_LoadCFG(cmd)
{
	xr_strcpy(m_cmd, cmd);
};

bool CCC_LoadCFG_custom::allow(LPCSTR cmd)
{
	return (cmd == strstr(cmd, m_cmd));
};

//-----------------------------------------------------------------------
class CCC_Start : public IConsole_Command
{
	void parse(LPSTR dest, LPCSTR args, LPCSTR name)
	{
		dest[0] = 0;
		if (strstr(args, name))
			sscanf(strstr(args, name) + xr_strlen(name), "(%[^)])", dest);
	}

	void protect_Name_strlwr(LPSTR str)
	{
		string4096 out;
		xr_strcpy(out, sizeof(out), str);
		strlwr(str);

		LPCSTR name_str = "name=";
		LPCSTR name1 = strstr(str, name_str);
		if (!name1 || !xr_strlen(name1))
		{
			return;
		}
		int begin_p = xr_strlen(str) - xr_strlen(name1) + xr_strlen(name_str);
		if (begin_p < 1)
		{
			return;
		}

		LPCSTR name2 = strchr(name1, '/');
		int end_p = xr_strlen(str) - ((name2) ? xr_strlen(name2) : 0);
		if (begin_p >= end_p)
		{
			return;
		}
		for (int i = begin_p; i < end_p; ++i)
		{
			str[i] = out[i];
		}
	}

public:
	CCC_Start(LPCSTR N) : IConsole_Command(N) { bLowerCaseArgs = false; };

	virtual void Execute(LPCSTR args)
	{
		/* if (g_pGameLevel) {
		 Log ("! Please disconnect/unload first");
		 return;
		 }
		 */
		string4096 op_server, op_client, op_demo;
		op_server[0] = 0;
		op_client[0] = 0;

		parse(op_server, args, "server"); // 1. server
		parse(op_client, args, "client"); // 2. client
		parse(op_demo, args, "demo"); // 3. demo

		strlwr(op_server);
		protect_Name_strlwr(op_client);

		if (!op_client[0] && strstr(op_server, "single"))
			xr_strcpy(op_client, "localhost");

		if ((0 == xr_strlen(op_client)) && (0 == xr_strlen(op_demo)))
		{
			Log("! Can't start game without client. Arguments: '%s'.", args);
			return;
		}
		if (g_pGameLevel)
			Engine.Event.Defer("KERNEL:disconnect");

		if (xr_strlen(op_demo))
		{
			Engine.Event.Defer("KERNEL:start_mp_demo", u64(xr_strdup(op_demo)), 0);
		}
		else
		{
			Engine.Event.Defer("KERNEL:start", u64(xr_strlen(op_server) ? xr_strdup(op_server) : 0),
			                   u64(xr_strdup(op_client)));
		}
	}
};

class CCC_Disconnect : public IConsole_Command
{
public:
	CCC_Disconnect(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };

	virtual void Execute(LPCSTR args)
	{
		Engine.Event.Defer("KERNEL:disconnect");
	}
};

//-----------------------------------------------------------------------
class CCC_PART_Export : public IConsole_Command
{
public:
	CCC_PART_Export(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };

	virtual void Execute(LPCSTR args)
	{
		Msg("Exporting particles...");
		Render->ExportParticles();
	}
};

class CCC_PART_Import : public IConsole_Command
{
public:
	CCC_PART_Import(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };

	virtual void Execute(LPCSTR args)
	{
		Msg("Importing particles...");
		Render->ImportParticles();
	}
};

//-----------------------------------------------------------------------
class CCC_VID_Reset : public IConsole_Command
{
public:
	CCC_VID_Reset(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };

	virtual void Execute(LPCSTR args)
	{
		if (Device.b_is_Ready)
		{
			Device.Reset();
		}
	}
};

class CCC_VidMode : public CCC_Token
{
	u32 _dummy;
public:
	CCC_VidMode(LPCSTR N) : CCC_Token(N, &_dummy, NULL) { bEmptyArgsHandled = FALSE; };

	virtual void Execute(LPCSTR args)
	{
		u32 _w, _h;
		int cnt = sscanf(args, "%dx%d", &_w, &_h);
		if (cnt == 2)
		{
			psCurrentVidMode[0] = _w;
			psCurrentVidMode[1] = _h;
		}
		else
		{
			Msg("! Wrong video mode [%s]", args);
			return;
		}
	}

	virtual void Status(TStatus& S)
	{
		xr_sprintf(S, sizeof(S), "%dx%d", psCurrentVidMode[0], psCurrentVidMode[1]);
	}

	virtual xr_token* GetToken() { return vid_mode_token; }

	virtual void Info(TInfo& I)
	{
		xr_strcpy(I, sizeof(I), "change screen resolution WxH");
	}

	virtual void fill_tips(vecTips& tips, u32 mode)
	{
		TStatus str, cur;
		Status(cur);

		bool res = false;
		xr_token* tok = GetToken();
		while (tok->name && !res)
		{
			if (!xr_strcmp(tok->name, cur))
			{
				xr_sprintf(str, sizeof(str), "%s (current)", tok->name);
				tips.push_back(str);
				res = true;
			}
			tok++;
		}
		if (!res)
		{
			tips.push_back("--- (current)");
		}
		tok = GetToken();
		while (tok->name)
		{
			tips.push_back(tok->name);
			tok++;
		}
	}
};

extern void GetMonitorResolution(u32& horizontal, u32& vertical);

class CCC_Screenmode : public CCC_Token
{
public:
	CCC_Screenmode(LPCSTR N) : CCC_Token(N, &g_screenmode, screen_mode_tokens){};

	virtual void Execute(LPCSTR args)
	{
		u32 prev_mode = g_screenmode;
		CCC_Token::Execute(args);

		if ((prev_mode != g_screenmode))
		{
			// TODO: If you enable the debug layer for DX11 and switch between fullscreen and windowed a few times,
			// you'll occasionally see the following error in the output:
			// DXGI ERROR: IDXGISwapChain::Present: The application has not called ResizeBuffers or re-created the SwapChain after a fullscreen or windowed transition. Flip model swapchains (DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL and DXGI_SWAP_EFFECT_FLIP_DISCARD) are required to do so. [ MISCELLANEOUS ERROR #117: ]
			// This shouldn't happen if we're calling reset on FS/Windowed transitions, I tried logging where Present()
			// is called as well as where ResizeBuffers and SetFullscreenState are called, and the debug output looks like this:
			//
			// Present()
			// SetFullscreenState()
			// [ERROR MESSAGE FROM ABOVE]
			// ResizeBuffers()
			// Present()
			//
			// Which makes no sense, it's impossible for us to have called Present between SetFullscreenState and ResizeBuffers
			// so DX11 must be doing it automatically
			//
			// These threads might be relevant for someone looking into this:
			// https://www.gamedev.net/forums/topic/652626-resizebuffers-bug/
			// https://www.gamedev.net/forums/topic/667426-dxgi-altenter-behaves-inconsistently/
			// https://www.gamedev.net/forums/topic/687484-correct-way-of-creating-the-swapchain-fullscreenwindowedvsync-about-refresh-rate/
			//
			// but the fixes make no sense and contradicts MSDN, and this isn't a major priority since ResizeBuffers is called
			// immediately after (before our Present call) so it works, just so stupid

			bool windowed_to_fullscreen = ((prev_mode == 0) || (prev_mode == 1)) && (g_screenmode == 2);
			bool fullscreen_to_windowed = (prev_mode == 2) && ((g_screenmode == 0) || (g_screenmode == 1));
			bool reset_required		    = windowed_to_fullscreen || fullscreen_to_windowed;
			if (Device.b_is_Ready && reset_required) {
				Device.Reset();
			}

			if (g_screenmode == 0 || g_screenmode == 1)
			{
				u32 w, h;
				GetMonitorResolution(w, h);
				SetWindowLongPtr(Device.m_hWnd, GWL_STYLE, WS_VISIBLE | WS_POPUP);
				SetWindowPos(Device.m_hWnd, HWND_TOP, 0, 0, w, h, SWP_FRAMECHANGED);

				if (g_screenmode == 0)
					SetWindowLongPtr(Device.m_hWnd, GWL_STYLE, WS_VISIBLE | WS_OVERLAPPEDWINDOW);
			}
		}

		RECT winRect;
		GetClientRect(Device.m_hWnd, &winRect);
		MapWindowPoints(Device.m_hWnd, nullptr, reinterpret_cast<LPPOINT>(&winRect), 2);
		ClipCursor(&winRect);
	}
};
//-----------------------------------------------------------------------
class CCC_SND_Restart : public IConsole_Command
{
public:
	CCC_SND_Restart(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };

	virtual void Execute(LPCSTR args)
	{
		Sound->_restart();
	}
};

//-----------------------------------------------------------------------
float ps_gamma = 1.f, ps_brightness = 1.f, ps_contrast = 1.f;

class CCC_Gamma : public CCC_Float
{
public:
	CCC_Gamma(LPCSTR N, float* V) : CCC_Float(N, V, 0.5f, 1.5f)
	{
	}

	virtual void Execute(LPCSTR args)
	{
		CCC_Float::Execute(args);
		//Device.Gamma.Gamma (ps_gamma);
		Device.m_pRender->setGamma(ps_gamma);
		//Device.Gamma.Brightness (ps_brightness);
		Device.m_pRender->setBrightness(ps_brightness);
		//Device.Gamma.Contrast (ps_contrast);
		Device.m_pRender->setContrast(ps_contrast);
		//Device.Gamma.Update ();
		Device.m_pRender->updateGamma();
	}
};

extern void updateCurrentScope();
class CCC_ScopeFactor : public CCC_Float
{
public:
	CCC_ScopeFactor(LPCSTR N, float* V) : CCC_Float(N, V, 0.01f, 1.0f)
	{
	}

	virtual void Execute(LPCSTR args)
	{
		CCC_Float::Execute(args);
		updateCurrentScope();
	}
};

//-----------------------------------------------------------------------
/*
#ifdef DEBUG
extern INT g_bDR_LM_UsePointsBBox;
extern INT g_bDR_LM_4Steps;
extern INT g_iDR_LM_Step;
extern Fvector g_DR_LM_Min, g_DR_LM_Max;

class CCC_DR_ClearPoint : public IConsole_Command
{
public:
CCC_DR_ClearPoint(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
virtual void Execute(LPCSTR args) {
g_DR_LM_Min.x = 1000000.0f;
g_DR_LM_Min.z = 1000000.0f;

g_DR_LM_Max.x = -1000000.0f;
g_DR_LM_Max.z = -1000000.0f;

Msg("Local BBox (%f, %f) - (%f, %f)", g_DR_LM_Min.x, g_DR_LM_Min.z, g_DR_LM_Max.x, g_DR_LM_Max.z);
}
};

class CCC_DR_TakePoint : public IConsole_Command
{
public:
CCC_DR_TakePoint(LPCSTR N) : IConsole_Command(N) { bEmptyArgsHandled = TRUE; };
virtual void Execute(LPCSTR args) {
Fvector CamPos = Device.vCameraPosition;

if (g_DR_LM_Min.x > CamPos.x) g_DR_LM_Min.x = CamPos.x;
if (g_DR_LM_Min.z > CamPos.z) g_DR_LM_Min.z = CamPos.z;

if (g_DR_LM_Max.x < CamPos.x) g_DR_LM_Max.x = CamPos.x;
if (g_DR_LM_Max.z < CamPos.z) g_DR_LM_Max.z = CamPos.z;

Msg("Local BBox (%f, %f) - (%f, %f)", g_DR_LM_Min.x, g_DR_LM_Min.z, g_DR_LM_Max.x, g_DR_LM_Max.z);
}
};

class CCC_DR_UsePoints : public CCC_Integer
{
public:
CCC_DR_UsePoints(LPCSTR N, int* V, int _min=0, int _max=999) : CCC_Integer(N, V, _min, _max) {};
virtual void Save (IWriter *F) {};
};
#endif
*/

ENGINE_API BOOL r2_sun_static = TRUE;
ENGINE_API BOOL r2_advanced_pp = FALSE; // advanced post process and effects

u32 renderer_value = 3;

//void fill_render_mode_list();
//void free_render_mode_list();

class CCC_r2 : public CCC_Token
{
	typedef CCC_Token inherited;
public:
	CCC_r2(LPCSTR N) : inherited(N, &renderer_value, NULL) { renderer_value = 0; };

	virtual ~CCC_r2()
	{
		//free_render_mode_list();
	}

	virtual void Execute(LPCSTR args)
	{
		//fill_render_mode_list ();
		// vid_quality_token must be already created!
		tokens = vid_quality_token;

		inherited::Execute(args);
		// 0 - r1
		// 1..3 - r2
		// 4 - r3
		LPCSTR renderer_name = "";
		for (int i = 0; vid_quality_token[i].name; i++)
			if (i == renderer_value)
				renderer_name = vid_quality_token[i].name;

		bool isR2 = strcmp("renderer_r2a", renderer_name) == 0;
		isR2 |= strcmp("renderer_r2", renderer_name) == 0;
		isR2 |= strcmp("renderer_r2.5", renderer_name) == 0;
		psDeviceFlags.set(rsR2, isR2);
		psDeviceFlags.set(rsR3, strcmp("renderer_r3", renderer_name) == 0);
		psDeviceFlags.set(rsR4, strcmp("renderer_r4", renderer_name) == 0);

		r2_sun_static = strcmp("renderer_r1", renderer_name) == 0;
		r2_sun_static |= strcmp("renderer_r2a", renderer_name) == 0;

		r2_advanced_pp = strcmp("renderer_r2.5", renderer_name) == 0;
		r2_advanced_pp |= strcmp("renderer_r3", renderer_name) == 0;
		r2_advanced_pp |= strcmp("renderer_r4", renderer_name) == 0;
	}

	virtual void Status(TStatus& S)
	{
		tokens = vid_quality_token;

		if (tokens == nullptr)
			Engine.External.CreateRendererList();

		while (tokens->name)
		{
			if (tokens->id == (int)(*value))
			{
				xr_strcpy(S, tokens->name);
				return;
			}
			tokens++;
		}
		xr_strcpy(S, "?");
		return;
	}

	virtual void Save(IWriter* F)
	{
		//fill_render_mode_list ();
		tokens = vid_quality_token;
		if (!strstr(Core.Params, "-r2"))
		{
			inherited::Save(F);
		}
	}

	virtual xr_token* GetToken()
	{
		tokens = vid_quality_token;
		return inherited::GetToken();
	}
};
#ifndef DEDICATED_SERVER
class CCC_soundDevice : public CCC_Token
{
	typedef CCC_Token inherited;
public:
	CCC_soundDevice(LPCSTR N) : inherited(N, &snd_device_id, NULL)
	{
	};

	virtual ~CCC_soundDevice()
	{
	}

	virtual void Execute(LPCSTR args)
	{
		GetToken();
		if (!tokens) return;
		inherited::Execute(args);
	}

	virtual void Status(TStatus& S)
	{
		GetToken();
		if (!tokens) return;
		inherited::Status(S);
	}

	virtual xr_token* GetToken()
	{
		tokens = snd_devices_token;
		return inherited::GetToken();
	}

	virtual void Save(IWriter* F)
	{
		GetToken();
		if (!tokens) return;
		inherited::Save(F);
	}
};
#endif
//-----------------------------------------------------------------------
class CCC_ExclusiveMode : public IConsole_Command
{
private:
	typedef IConsole_Command inherited;

public:
	CCC_ExclusiveMode(LPCSTR N) :
		inherited(N)
	{
	}

	virtual void Execute(LPCSTR args)
	{
		bool value = false;
		if (!xr_strcmp(args, "on"))
			value = true;
		else if (!xr_strcmp(args, "off"))
			value = false;
		else if (!xr_strcmp(args, "true"))
			value = true;
		else if (!xr_strcmp(args, "false"))
			value = false;
		else if (!xr_strcmp(args, "1"))
			value = true;
		else if (!xr_strcmp(args, "0"))
			value = false;
		else InvalidSyntax();

		pInput->exclusive_mode(value);
	}

	virtual void Save(IWriter* F)
	{
		F->w_printf("input_exclusive_mode %s\r\n", pInput->get_exclusive_mode() ? "on" : "off");
	}
};

class ENGINE_API CCC_HideConsole : public IConsole_Command
{
public:
	CCC_HideConsole(LPCSTR N) : IConsole_Command(N)
	{
		bEmptyArgsHandled = true;
	}

	virtual void Execute(LPCSTR args)
	{
		Console->Hide();
	}

	virtual void Status(TStatus& S)
	{
		S[0] = 0;
	}

	virtual void Info(TInfo& I)
	{
		xr_sprintf(I, sizeof(I), "hide console");
	}
};

class CCC_SoundParamsSmoothing : public CCC_Integer
{
public:
	CCC_SoundParamsSmoothing(LPCSTR N, int* V, int _min = 0, int _max = 999) : CCC_Integer(N, V, _min, _max) {};

	virtual void Execute(LPCSTR args)
	{
		CCC_Integer::Execute(args);
		soundSmoothingParams::alpha = soundSmoothingParams::getAlpha();
	}
};

class CCC_Editor : public IConsole_Command
{
public:
	CCC_Editor(pcstr name) : IConsole_Command(name) { bEmptyArgsHandled = true; }
	void Execute(pcstr args) override
	{
		Device.imgui().Show(true);
	}
};

ENGINE_API float psHUD_FOV_def = 0.45f;
ENGINE_API float psHUD_FOV = psHUD_FOV_def;

//extern int psSkeletonUpdate;
extern int rsDVB_Size;
extern int rsDIB_Size;
extern int psNET_ClientUpdate;
extern int psNET_ClientPending;
extern int psNET_ServerUpdate;
extern int psNET_ServerPending;
extern int psNET_DedicatedSleep;
extern char psNET_Name[32];
extern Flags32 psEnvFlags;
//extern float r__dtex_range;

extern int g_ErrorLineCount;

ENGINE_API int ps_r__Supersample = 1;
ENGINE_API float ps_r2_sun_shafts_min = 0.f;
ENGINE_API float ps_r2_sun_shafts_value = 1.f;
ENGINE_API float hit_modifier = 1.0f;

extern float g_dispersion_base;
extern float g_dispersion_factor;
float g_AimLookFactor = 1.f;

int ps_framelimiter = 0;
extern u32 g_crosshair_color;
float g_freelook_z_offset;
float g_ironsights_factor = 1.25f;

// crookr fake scope params (sorry)
float scope_fog_interp = 0.15f;
float scope_fog_travel = 0.25f;
float scope_fog_attack = 0.66f;
float scope_fog_mattack = 0.25f;
//float scope_drift_amount = 1.f;
float scope_ca = 0.003f;
float scope_outerblur = 1.0f;
float scope_innerblur = 0.1f;
float scope_scrollpower = 0.66f;
float scope_brightness = 1.0f;
float scope_radius = 0.f;
float scope_fog_radius = 1.25f;
float scope_fog_sharp = 4.0f;
int scope_2dtexactive = 0.0;
Fvector3 ssfx_wetness_multiplier = Fvector3().set(1.0f, 0.3f, 0.0f);

void CCC_Register()
{
	// General
	CMD1(CCC_Help, "help");
	CMD1(CCC_Quit, "quit");
	CMD1(CCC_Start, "start");
	CMD1(CCC_Disconnect, "disconnect");
	CMD1(CCC_SaveCFG, "cfg_save");
	CMD1(CCC_LoadCFG, "cfg_load");

#ifdef DEBUG
    CMD1(CCC_MotionsStat, "stat_motions");
    CMD1(CCC_TexturesStat, "stat_textures");
#endif // DEBUG

#ifdef DEBUG_MEMORY_MANAGER
    CMD1(CCC_MemStat, "dbg_mem_dump");
    CMD1(CCC_DbgMemCheck, "dbg_mem_check");
#endif // DEBUG_MEMORY_MANAGER

#ifdef DEBUG
    CMD3(CCC_Mask, "mt_particles", &psDeviceFlags, mtParticles);

    CMD1(CCC_DbgStrCheck, "dbg_str_check");
    CMD1(CCC_DbgStrDump, "dbg_str_dump");

    CMD3(CCC_Mask, "mt_sound", &psDeviceFlags, mtSound);
    CMD3(CCC_Mask, "mt_physics", &psDeviceFlags, mtPhysics);
    CMD3(CCC_Mask, "mt_network", &psDeviceFlags, mtNetwork);

    // Events
    CMD1(CCC_E_Dump, "e_list");
    CMD1(CCC_E_Signal, "e_signal");

    CMD3(CCC_Mask, "rs_wireframe", &psDeviceFlags, rsWireframe);
    CMD3(CCC_Mask, "rs_clear_bb", &psDeviceFlags, rsClearBB);
    CMD3(CCC_Mask, "rs_occlusion", &psDeviceFlags, rsOcclusion);

    CMD3(CCC_Mask, "rs_detail", &psDeviceFlags, rsDetails);
    //CMD4(CCC_Float, "r__dtex_range", &r__dtex_range, 5, 175 );

    // CMD3(CCC_Mask, "rs_constant_fps", &psDeviceFlags, rsConstantFPS );
    CMD3(CCC_Mask, "rs_render_statics", &psDeviceFlags, rsDrawStatic);
    CMD3(CCC_Mask, "rs_render_dynamics", &psDeviceFlags, rsDrawDynamic);
#endif

	// bone damage modifier
	CMD4(CCC_Float, "g_hit_pwr_modif", &hit_modifier, .5f, 3.f);

	CMD4(CCC_Float, "g_dispersion_base", &g_dispersion_base, 0.0f, 5.0f);
	CMD4(CCC_Float, "g_dispersion_factor", &g_dispersion_factor, 0.1f, 10.0f);

	// Render device states
	CMD4(CCC_Integer, "r__supersample", &ps_r__Supersample, 1, 4);

	CMD4(CCC_Float, "r2_sunshafts_min", &ps_r2_sun_shafts_min, 0.0, 0.5);
	CMD4(CCC_Float, "r2_sunshafts_value", &ps_r2_sun_shafts_value, 0.5, 2.0);

	CMD3(CCC_Mask, "rs_v_sync", &psDeviceFlags, rsVSync);
	// CMD3(CCC_Mask, "rs_disable_objects_as_crows",&psDeviceFlags, rsDisableObjectsAsCrows );
	CMD1(CCC_Screenmode, "rs_screenmode");
	CMD3(CCC_Mask, "rs_stats", &psDeviceFlags, rsStatistic);
	CMD4(CCC_Float, "rs_vis_distance", &psVisDistance, 0.4f, 1.5f);

	CMD3(CCC_Mask, "rs_cam_pos", &psDeviceFlags, rsCameraPos);
#ifdef DEBUG
    CMD3(CCC_Mask, "rs_occ_draw", &psDeviceFlags, rsOcclusionDraw);
    CMD3(CCC_Mask, "rs_occ_stats", &psDeviceFlags, rsOcclusionStats);
    //CMD4(CCC_Integer, "rs_skeleton_update", &psSkeletonUpdate, 2, 128 );
#endif // DEBUG

	CMD2(CCC_Gamma, "rs_c_gamma", &ps_gamma);
	CMD2(CCC_Gamma, "rs_c_brightness", &ps_brightness);
	CMD2(CCC_Gamma, "rs_c_contrast", &ps_contrast);
	// CMD4(CCC_Integer, "rs_vb_size", &rsDVB_Size, 32, 4096);
	// CMD4(CCC_Integer, "rs_ib_size", &rsDIB_Size, 32, 4096);

	// Texture manager
	CMD4(CCC_Integer, "texture_lod", &psTextureLOD, 0, 4);
	//CMD4(CCC_Integer, "net_dedicated_sleep", &psNET_DedicatedSleep, 0, 64);

	// General video control
	CMD1(CCC_VidMode, "vid_mode");

#ifdef DEBUG
    CMD3(CCC_Token, "vid_bpp", &psCurrentBPP, vid_bpp_token);
#endif // DEBUG

	CMD1(CCC_PART_Export, "part_export");
	CMD1(CCC_PART_Import, "part_import");

	CMD1(CCC_VID_Reset, "vid_restart");

	// Sound
	CMD2(CCC_Float, "snd_volume_eff", &psSoundVEffects);
	CMD2(CCC_Float, "snd_volume_music", &psSoundVMusic);
	CMD1(CCC_SND_Restart, "snd_restart");
	CMD3(CCC_Mask, "snd_acceleration", &psSoundFlags, ss_Hardware);
	CMD3(CCC_Mask, "snd_efx", &psSoundFlags, ss_EAX);
	CMD4(CCC_Integer, "snd_targets", &psSoundTargets, 32, 1024);
	CMD4(CCC_Integer, "snd_cache_size", &psSoundCacheSizeMB, 8, 256);

	// Distance based delay power
	CMD4(CCC_Float, "snd_distance_based_delay_power", &soundSmoothingParams::distanceBasedDelayPower, 0.f, 2.f);
	CMD4(CCC_Float, "snd_distance_based_delay_min_distance", &soundSmoothingParams::distanceBasedDelayMinDistance, 15.f, 100.f);

	// Pitch variation power
	CMD4(CCC_Float, "snd_pitch_variation_power", &soundSmoothingParams::pitchVariationPower, 0.f, 4.f);

	// Doppler effect power
	CMD4(CCC_Float, "snd_doppler_power", &soundSmoothingParams::power, 0.f, 5.f);
	CMD4(CCC_SoundParamsSmoothing, "snd_doppler_smoothing", &soundSmoothingParams::steps, 1, 100);

#ifdef DEBUG
    CMD3(CCC_Mask, "snd_stats", &g_stats_flags, st_sound);
    CMD3(CCC_Mask, "snd_stats_min_dist", &g_stats_flags, st_sound_min_dist);
    CMD3(CCC_Mask, "snd_stats_max_dist", &g_stats_flags, st_sound_max_dist);
    CMD3(CCC_Mask, "snd_stats_ai_dist", &g_stats_flags, st_sound_ai_dist);
    CMD3(CCC_Mask, "snd_stats_info_name", &g_stats_flags, st_sound_info_name);
    CMD3(CCC_Mask, "snd_stats_info_object", &g_stats_flags, st_sound_info_object);

    CMD4(CCC_Integer, "error_line_count", &g_ErrorLineCount, 6, 1024);
#endif // DEBUG

	// Mouse
	CMD3(CCC_Mask, "mouse_invert", &psMouseInvert, 1);
	psMouseSens = 0.12f;
	CMD4(CCC_Float, "mouse_sens", &psMouseSens, 0.001f, 0.6f);

	// demonized: vertical mouse sens
	psMouseSensVerticalK = 1.0f;
	CMD4(CCC_Float, "mouse_sens_vertical", &psMouseSensVerticalK, 0.1f, 3.0f);

	// Camera
	CMD2(CCC_Float, "cam_inert", &psCamInert);
	CMD2(CCC_Float, "cam_slide_inert", &psCamSlideInert);

	CMD1(CCC_r2, "renderer");

#ifndef DEDICATED_SERVER
	CMD1(CCC_soundDevice, "snd_device");
#endif
	//psSoundRolloff = pSettings->r_float ("sound","rolloff"); clamp(psSoundRolloff, EPS_S, 2.f);
	psSoundOcclusionScale = pSettings->r_float("sound", "occlusion_scale");
	clamp(psSoundOcclusionScale, 0.1f, .5f);

	extern int g_Dump_Export_Obj;
	extern int g_Dump_Import_Obj;
	//CMD4(CCC_Integer, "net_dbg_dump_export_obj", &g_Dump_Export_Obj, 0, 1);
	//CMD4(CCC_Integer, "net_dbg_dump_import_obj", &g_Dump_Import_Obj, 0, 1);

#ifdef DEBUG
    CMD1(CCC_DumpOpenFiles, "dump_open_files");
#endif

	//CMD1(CCC_ExclusiveMode, "input_exclusive_mode");

	extern int g_svTextConsoleUpdateRate;
	//CMD4(CCC_Integer, "sv_console_update_rate", &g_svTextConsoleUpdateRate, 1, 100);

	extern int g_svDedicateServerUpdateReate;
	//CMD4(CCC_Integer, "sv_dedicated_server_update_rate", &g_svDedicateServerUpdateReate, 1, 1000);

	CMD1(CCC_HideConsole, "hide");

	CMD4(CCC_Integer, "r__framelimit", &ps_framelimiter, 0, 500);
	CMD3(CCC_Mask, "rs_refresh_60hz", &psDeviceFlags, rsRefresh60hz);
	CMD2(CCC_Color, "g_crosshair_color", &g_crosshair_color);
	CMD4(CCC_Float, "mouse_sens_aim", &g_AimLookFactor, 0.01f, 5.0f);

	if (strstr(Core.Params, "-dbgdev"))
		CMD4(CCC_Float, "g_freelook_z_offset_factor", &g_freelook_z_offset, -3.f, 3.f);

	CMD4(CCC_Float, "g_ironsights_zoom_factor", &g_ironsights_factor, 1.f, 2.f);
	CMD4(CCC_Vector3, "ssfx_wetness_multiplier", &ssfx_wetness_multiplier, Fvector3().set(0.1f, 0.1f, 0.0f), Fvector3().set(20.0f, 20.0f, 0.0f));

	// - CrookR
	CMD2(CCC_Float, "scope_blur_outer", &scope_outerblur);
	CMD2(CCC_Float, "scope_blur_inner", &scope_innerblur);
	CMD2(CCC_ScopeFactor, "scope_factor", &scope_scrollpower);
	CMD2(CCC_Float, "scope_brightness", &scope_brightness);

	CMD2(CCC_Float, "scope_fog_interp", &scope_fog_interp);
	CMD4(CCC_Float, "scope_fog_travel", &scope_fog_travel, 0.f, 5.f);
	CMD4(CCC_Float, "scope_fog_swayAim", &scope_fog_attack, -999.f, 999.f);
	CMD4(CCC_Float, "scope_fog_swayMove", &scope_fog_mattack, -999.f, 999.f);
	//CMD4(CCC_Float, "scope_drift_amount", &scope_drift_amount, -999.f, 999.f);

	CMD2(CCC_Float, "scope_ca", &scope_ca);
	CMD4(CCC_Float, "scope_radius", &scope_radius, 0, 2);
	CMD4(CCC_Float, "scope_fog_radius", &scope_fog_radius, 0, 1000);
	CMD4(CCC_Float, "scope_fog_sharp", &scope_fog_sharp, 0, 1000);
	CMD2(CCC_Integer, "scope_2dtexactive", &scope_2dtexactive);
	CMD1(CCC_Editor, "rs_editor");
#ifdef DEBUG
    extern BOOL debug_destroy;
    CMD4(CCC_Integer, "debug_destroy", &debug_destroy, FALSE, TRUE);
#endif
};

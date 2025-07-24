#include "stdafx.h"
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#pragma warning(disable: 4047)
HINSTANCE hInstance = (HINSTANCE)&__ImageBase;
#pragma warning(default: 4047)

//Credits for idea SSE ReShade Helper https://www.nexusmods.com/skyrimspecialedition/mods/78961

static reshade::api::effect_runtime* m_runtime = nullptr;
static reshade::api::command_list* m_cmdlist = nullptr;
static reshade::api::resource_view m_rtv;
static reshade::api::resource_view m_rtv_srgb;

static void on_reshade_begin_effects(reshade::api::effect_runtime* runtime, reshade::api::command_list* cmd_list, reshade::api::resource_view rtv, reshade::api::resource_view rtv_srgb)
{
	m_runtime = runtime;
	m_cmdlist = cmd_list;
	m_rtv = rtv;
	m_rtv_srgb = rtv_srgb;
}

void render_reshade_effects()
{
	if (m_runtime)
		m_runtime->render_effects(m_cmdlist, m_rtv, m_rtv_srgb);
}

bool init_reshade()
{
	if (!reshade::register_addon(hInstance))
		return false;

	reshade::register_event<reshade::addon_event::reshade_begin_effects>(on_reshade_begin_effects);

	return true;
}

void unregister_reshade()
{
	reshade::unregister_event<reshade::addon_event::reshade_begin_effects>(on_reshade_begin_effects);
	reshade::unregister_addon(hInstance);
	m_runtime = nullptr;
	m_cmdlist = nullptr;
	m_rtv.handle = 0;
	m_rtv_srgb.handle = 0;
}
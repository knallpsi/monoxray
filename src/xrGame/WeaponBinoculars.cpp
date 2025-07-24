#include "stdafx.h"
#include "WeaponBinoculars.h"

#include "xr_level_controller.h"

#include "level.h"
#include "ui\UIFrameWindow.h"
#include "WeaponBinocularsVision.h"
#include "NewZoomFlag.h"
#include "object_broker.h"
#include "inventory.h"

extern float n_zoom_step_count;
float czoom;

CWeaponBinoculars::CWeaponBinoculars()
{
	m_binoc_vision = NULL;
	m_bVision = false;
}

CWeaponBinoculars::~CWeaponBinoculars()
{
	xr_delete(m_binoc_vision);
}

void CWeaponBinoculars::Load(LPCSTR section)
{
	inherited::Load(section);

	// Sounds
	m_sounds.LoadSound(section, "snd_zoomin", "sndZoomIn", false, SOUND_TYPE_ITEM_USING);
	m_sounds.LoadSound(section, "snd_zoomout", "sndZoomOut", false, SOUND_TYPE_ITEM_USING);
	m_bVision = !!pSettings->r_bool(section, "vision_present");
}

bool CWeaponBinoculars::NeedBlendAnm()
{
	return false;
}

bool CWeaponBinoculars::MovingAnimAllowedNow()
{
	return true;
}

Fmatrix CWeaponBinoculars::RayTransform()
{
	Fmatrix matrix = CHudItem::RayTransform();
	matrix.i = Device.vCameraTop;
	matrix.j = Device.vCameraRight;
	matrix.k = Device.vCameraDirection;
	return matrix;
}

bool CWeaponBinoculars::Action(u16 cmd, u32 flags)
{
	switch (cmd)
	{
	case kWPN_FIRE:
		return inherited::Action(kWPN_ZOOM, flags);
	}

	return inherited::Action(cmd, flags);
}

void CWeaponBinoculars::OnZoomIn()
{
	if (H_Parent() && !IsZoomed())
	{
		m_sounds.StopSound("sndZoomOut");
		bool b_hud_mode = (Level().CurrentEntity() == H_Parent());
		m_sounds.PlaySound("sndZoomIn", H_Parent()->Position(), H_Parent(), b_hud_mode);
		if (m_bVision && !m_binoc_vision)
		{
			//.VERIFY			(!m_binoc_vision);
			m_binoc_vision = xr_new<CBinocularsVision>(cNameSect());
		}
	}
	inherited::OnZoomIn();
    SetZoomFactor(czoom);
}

void CWeaponBinoculars::OnZoomOut()
{
	if (H_Parent() && IsZoomed() && !IsRotatingToZoom())
	{
		m_sounds.StopSound("sndZoomIn");
		bool b_hud_mode = (Level().CurrentEntity() == H_Parent());
		m_sounds.PlaySound("sndZoomOut", H_Parent()->Position(), H_Parent(), b_hud_mode);
		VERIFY(m_binoc_vision);
		xr_delete(m_binoc_vision);
	}


	inherited::OnZoomOut();
}

BOOL CWeaponBinoculars::net_Spawn(CSE_Abstract* DC)
{
    czoom = CWeaponBinoculars::m_zoom_params.m_fScopeZoomFactor;
	return inherited::net_Spawn(DC);
}

void CWeaponBinoculars::net_Destroy()
{
	inherited::net_Destroy();
	xr_delete(m_binoc_vision);
}

void CWeaponBinoculars::UpdateCL()
{
	inherited::UpdateCL();
	//manage visible entities here...
	if (H_Parent() && IsZoomed() && !IsRotatingToZoom() && m_binoc_vision)
		m_binoc_vision->Update();
}

bool CWeaponBinoculars::render_item_ui_query()
{
	bool b_is_active_item = m_pInventory->ActiveItem() == this;
	return b_is_active_item && H_Parent() && IsZoomed() && ZoomTexture() && !IsRotatingToZoom() && m_binoc_vision;
}

void CWeaponBinoculars::render_item_ui()
{
	m_binoc_vision->Draw();
	inherited::render_item_ui();
}

// demonized: new zoom delta change to have same multiple between steps for same visual change with each step
BOOL useNewZoomDeltaAlgorithm = FALSE;
void newGetZoomDelta(const float scope_factor, float& delta, const float min_zoom_factor, float steps)
{
	delta = pow(scope_factor / min_zoom_factor, 1.0f / steps);
}

void GetZoomData(const float scope_factor, const float zoom_step_count, float min_zoom_setting, float& delta, float& min_zoom_factor)
{
	float def_fov = float(g_fov);
	float min_zoom_k = 0.3f;
	float delta_factor_total = def_fov - scope_factor;
	VERIFY(delta_factor_total > 0);
	min_zoom_factor = def_fov - delta_factor_total * min_zoom_k;
	if (min_zoom_factor > min_zoom_setting)
		min_zoom_factor = min_zoom_setting;
	float steps = zoom_step_count ? zoom_step_count : 3.0;
	delta = (min_zoom_factor - scope_factor) / steps;
	if (useNewZoomDeltaAlgorithm)
		newGetZoomDelta(scope_factor, delta, min_zoom_factor, steps);
}

void newGetZoomData(const float scope_factor, const float zoom_step_count, float& delta, float& min_zoom_factor, float c_zoom)
{
	GetZoomData(scope_factor, zoom_step_count, 200.0, delta, min_zoom_factor);
	float steps = zoom_step_count ? zoom_step_count : n_zoom_step_count;
	delta = (min_zoom_factor - scope_factor) / steps;
	if (useNewZoomDeltaAlgorithm)
		newGetZoomDelta(scope_factor, delta, min_zoom_factor, steps);
}

void CWeaponBinoculars::ZoomInc()
{
	float delta, min_zoom_factor;
	if (zoomFlags.test(NEW_ZOOM)) {
		newGetZoomData(m_zoom_params.m_fScopeZoomFactor, m_zoom_params.m_fZoomStepCount, delta, min_zoom_factor, czoom);
	} else {
		GetZoomData(m_zoom_params.m_fScopeZoomFactor, m_zoom_params.m_fZoomStepCount, m_zoom_params.m_fMinBaseZoomFactor, delta, min_zoom_factor);
	}

	float f = useNewZoomDeltaAlgorithm ? GetZoomFactor() * delta : GetZoomFactor() - delta;
	clamp(f, m_zoom_params.m_fScopeZoomFactor, min_zoom_factor);
	SetZoomFactor(f);
    czoom = f;
}

void CWeaponBinoculars::ZoomDec()
{
	float delta, min_zoom_factor;
	if (zoomFlags.test(NEW_ZOOM)) {
		newGetZoomData(m_zoom_params.m_fScopeZoomFactor, m_zoom_params.m_fZoomStepCount, delta, min_zoom_factor, czoom);
	} else {
		GetZoomData(m_zoom_params.m_fScopeZoomFactor, m_zoom_params.m_fZoomStepCount, m_zoom_params.m_fMinBaseZoomFactor, delta, min_zoom_factor);
	}

	float f = useNewZoomDeltaAlgorithm ? GetZoomFactor() / max(delta, 0.001f) : GetZoomFactor() + delta;
	clamp(f, m_zoom_params.m_fScopeZoomFactor, min_zoom_factor);
	SetZoomFactor(f);
    czoom = f;
}

void CWeaponBinoculars::save(NET_Packet& output_packet)
{
	inherited::save(output_packet);
	save_data(m_fRTZoomFactor, output_packet);
}

void CWeaponBinoculars::load(IReader& input_packet)
{
	inherited::load(input_packet);
	load_data(m_fRTZoomFactor, input_packet);
}

bool CWeaponBinoculars::GetBriefInfo(II_BriefInfo& info)
{
	info.clear();
	info.name._set(m_nameShort);
	info.icon._set(cNameSect());
	return true;
}

void CWeaponBinoculars::net_Relcase(CObject* object)
{
	CHudItem::net_Relcase(object);

	if (!m_binoc_vision)
		return;

	m_binoc_vision->remove_links(object);
}

bool CWeaponBinoculars::can_kill() const
{
	return (false);
}

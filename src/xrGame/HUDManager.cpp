#include "stdafx.h"
#include "HUDManager.h"
#include "hudtarget.h"
#include "actor.h"
#include "../xrEngine/igame_level.h"
#include "../xrEngine/xr_input.h"
#include "../xrEngine/gamemtllib.h"
#include "MainMenu.h"
#include "grenade.h"
#include "spectator.h"
#include "Car.h"
#include "UIGameCustom.h"
#include "UICursor.h"
#include "string_table.h"
#include "game_cl_base.h"
#ifdef	DEBUG
#include "phdebug.h"
#endif

#include "Flashlight.h"
#include "Inventory.h"
#include "map_manager.h"
#include "player_hud.h"
#include "script_attachment_manager.h"

extern CUIGameCustom* CurrentGameUI()
{
	return g_hud ? HUD().GetGameUI() : nullptr;
}

CFontManager::CFontManager()
{
	Device.seqDeviceReset.Add(this, REG_PRIORITY_HIGH);

	m_all_fonts.push_back(&pFontMedium); // used cpp
	m_all_fonts.push_back(&pFontDI); // used cpp
	m_all_fonts.push_back(&pFontArial14); // used xml
	m_all_fonts.push_back(&pFontGraffiti19Russian);
	m_all_fonts.push_back(&pFontGraffiti22Russian);
	m_all_fonts.push_back(&pFontLetterica16Russian);
	m_all_fonts.push_back(&pFontLetterica18Russian);
	m_all_fonts.push_back(&pFontGraffiti32Russian);
	m_all_fonts.push_back(&pFontGraffiti50Russian);
	m_all_fonts.push_back(&pFontLetterica25);
	m_all_fonts.push_back(&pFontStat);

	FONTS_VEC_IT it = m_all_fonts.begin();
	FONTS_VEC_IT it_e = m_all_fonts.end();
	for (; it != it_e; ++it)
		(**it) = NULL;

	InitializeFonts();
}

void CFontManager::InitializeFonts()
{
	InitializeFont(pFontMedium, "hud_font_medium");
	InitializeFont(pFontDI, "hud_font_di", CGameFont::fsGradient | CGameFont::fsDeviceIndependent);
	InitializeFont(pFontArial14, "ui_font_arial_14");
	InitializeFont(pFontGraffiti19Russian, "ui_font_graffiti19_russian");
	InitializeFont(pFontGraffiti22Russian, "ui_font_graffiti22_russian");
	InitializeFont(pFontLetterica16Russian, "ui_font_letterica16_russian");
	InitializeFont(pFontLetterica18Russian, "ui_font_letterica18_russian");
	InitializeFont(pFontGraffiti32Russian, "ui_font_graff_32");
	InitializeFont(pFontGraffiti50Russian, "ui_font_graff_50");
	InitializeFont(pFontLetterica25, "ui_font_letter_25");
	InitializeFont(pFontStat, "stat_font", CGameFont::fsDeviceIndependent);
	pFontStat->SetInterval(0.75f, 1.0f);
}

LPCSTR CFontManager::GetFontTexName(LPCSTR section)
{
	static char* tex_names[] = {"texture800", "texture", "texture1600", "texture2160"};
	int def_idx = 1; //default 1024x768
	int idx = def_idx;
#if 0
    u32 w = Device.dwWidth;

    if(w<=800)		idx = 0;
    else if(w<=1280)idx = 1;
    else 			idx = 2;
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

void CFontManager::InitializeFont(CGameFont*& F, LPCSTR section, u32 flags)
{
	LPCSTR font_tex_name = GetFontTexName(section);
	R_ASSERT(font_tex_name);
	LPCSTR sh_name = pSettings->r_string(section, "shader");
	if (!F)
		F = xr_new<CGameFont>(sh_name, font_tex_name, flags);
	else
		F->Initialize(sh_name, font_tex_name);

#ifdef DEBUG
    F->m_font_name = section;
#endif
}

CFontManager::~CFontManager()
{
	Device.seqDeviceReset.Remove(this);
	FONTS_VEC_IT it = m_all_fonts.begin();
	FONTS_VEC_IT it_e = m_all_fonts.end();
	for (; it != it_e; ++it)
		xr_delete(**it);
}

void CFontManager::Render()
{
	FONTS_VEC_IT it = m_all_fonts.begin();
	FONTS_VEC_IT it_e = m_all_fonts.end();
	for (; it != it_e; ++it)
		(**it)->OnRender();
}

void CFontManager::OnDeviceReset()
{
	InitializeFonts();
}

//--------------------------------------------------------------------
CHUDManager::CHUDManager() : pUIGame(NULL), m_pHUDTarget(xr_new<CHUDTarget>()), b_online(false)
{
}

//--------------------------------------------------------------------
CHUDManager::~CHUDManager()
{
	OnDisconnected();

	if (pUIGame)
		pUIGame->UnLoad();

	xr_delete(pUIGame);
	xr_delete(m_pHUDTarget);
}

//--------------------------------------------------------------------
void CHUDManager::OnFrame()
{
	if (!psHUD_Flags.is(HUD_DRAW_RT2))
		return;

	if (!b_online)
		return;

	if (pUIGame)
		pUIGame->OnFrame();

	PP.CameraPick();
	g_player_hud->OnFrame();
	DoPick(PP);
}

//--------------------------------------------------------------------
void CHUDManager::Render_First()
{
	if (!psHUD_Flags.is(HUD_WEAPON | HUD_WEAPON_RT | HUD_WEAPON_RT2 | HUD_DRAW_RT2))return;
	if (0 == pUIGame) return;
	CObject* O = g_pGameLevel->CurrentViewEntity();
	if (0 == O) return;
	CActor* A = smart_cast<CActor*>(O);
	if (!A) return;
	if (A && !A->HUDview()) return;

	// only shadow
	::Render->set_Invisible(TRUE);
	::Render->set_Object(O->H_Root());
	O->renderable_Render();
	::Render->set_Invisible(FALSE);
}

bool need_render_hud()
{
	CObject* O = g_pGameLevel ? g_pGameLevel->CurrentViewEntity() : NULL;
	if (0 == O)
		return false;

	CActor* A = smart_cast<CActor*>(O);
	if (A && (!A->HUDview() || !A->g_Alive()))
		return false;

	if (smart_cast<CCar*>(O) || smart_cast<CSpectator*>(O))
		return false;

	return true;
}

void CHUDManager::Render_Last()
{
	if (0 == pUIGame) return;
	if (g_actor) g_actor->RenderCamAttached();
	if (!psHUD_Flags.is(HUD_WEAPON | HUD_WEAPON_RT | HUD_WEAPON_RT2 | HUD_DRAW_RT2))return;
	if (!need_render_hud()) return;

	CObject* O = g_pGameLevel->CurrentViewEntity();
	// hud itself
	::Render->set_HUD(TRUE);
	::Render->set_Object(O->H_Root());
	O->OnHUDDraw(this);
	::Render->set_HUD(FALSE);
}

void CHUDManager::Render_R1_Attachment_UI()
{
	for (auto att : g_pGamePersistent->AttachmentUIsToRender)
		att->RenderUI();

	g_pGamePersistent->AttachmentUIsToRender.clear_not_free();
}


bool CHUDManager::RenderActiveItemUIQuery()
{
	if (!psHUD_Flags.is(HUD_DRAW_RT2))
		return false;

	if (!psHUD_Flags.is(HUD_WEAPON | HUD_WEAPON_RT | HUD_WEAPON_RT2))return false;

	if (!need_render_hud()) return false;

	return (g_player_hud && g_player_hud->render_item_ui_query());
}

bool CHUDManager::RenderCamAttachedUIQuery()
{
	if (!g_actor) return false;

	for (auto& pair : *g_actor->GetAttachments())
	{
		script_attachment* att = pair.second;
		if (att->GetType() == eSA_CamAttached)
			return true;
	}
	return false;
}

void CHUDManager::RenderActiveItemUI()
{
	if (!psHUD_Flags.is(HUD_DRAW_RT2))
		return;

	g_player_hud->render_item_ui();
}

void CHUDManager::RenderCamAttachedUI()
{
	for (auto& pair : *g_actor->GetAttachments())
	{
		script_attachment* att = pair.second;
		if (att->GetType() == eSA_CamAttached)
			att->RenderUI();
	}
}

extern ENGINE_API BOOL bShowPauseString;
//отрисовка элементов интерфейса
void CHUDManager::RenderUI()
{
	if (!psHUD_Flags.is(HUD_DRAW_RT2))
		return;

	if (!b_online) return;

	if (true /*|| psHUD_Flags.is(HUD_DRAW | HUD_DRAW_RT)*/)
	{
		HitMarker.Render();
		if (pUIGame)
			pUIGame->Render();

		UI().RenderFont();
	}

	m_pHUDTarget->Render();

	if (Device.Paused() && bShowPauseString)
	{
		CGameFont* pFont = UI().Font().pFontGraffiti50Russian;
		pFont->SetColor(0x80FF0000);
		LPCSTR _str = CStringTable().translate("st_game_paused").c_str();

		Fvector2 _pos;
		_pos.set(UI_BASE_WIDTH / 2.0f, UI_BASE_HEIGHT / 2.0f);
		UI().ClientToScreenScaled(_pos);
		pFont->SetAligment(CGameFont::alCenter);
		pFont->Out(_pos.x, _pos.y, _str);
		pFont->OnRender();
	}
}

void CHUDManager::OnEvent(EVENT E, u64 P1, u64 P2)
{
}

bool CHUDManager::FireposActive()
{
	// If we have an actor...
	CActor* pActor = smart_cast<CActor*>(Level().CurrentEntity());
	if (!pActor)
		return psActorFlags.test(AF_FIREPOS);

	// And a weapon...
	CWeapon* pWeapon = smart_cast<CWeapon*>(pActor->inventory().ActiveItem());
	if (!pWeapon)
		return psActorFlags.test(AF_FIREPOS);

	if (!pWeapon->GetFirepos())
		return false;

	// Firepos is active if a setting matches its respective zoom state
	float zFac = pWeapon->GetZRotatingFactor();
	return (psActorFlags.test(AF_FIREPOS) && zFac < 1.f)
		|| (psActorFlags.test(AF_FIREPOS_ZOOM) && zFac >= 1.f);
}

bool CHUDManager::AimposActive()
{
	// If we have an actor...
	CActor* pActor = smart_cast<CActor*>(Level().CurrentEntity());
	if (!pActor)
		return psActorFlags.test(AF_AIMPOS);

	// And a weapon...
	CWeapon* pWeapon = smart_cast<CWeapon*>(pActor->inventory().ActiveItem());
	if (!pWeapon)
		return psActorFlags.test(AF_AIMPOS);

	if (!pWeapon->GetAimpos())
		return false;

	// Firepos is active if a setting matches its respective zoom state
	float zFac = pWeapon->GetZRotatingFactor();
	return (psActorFlags.test(AF_AIMPOS) && zFac < 1.f)
		|| (psActorFlags.test(AF_AIMPOS_ZOOM) && zFac >= 1.f);
}

ICF static BOOL pick_trace_callback(collide::rq_result& result, LPVOID params)
{
	SPickParam* pp = (SPickParam*)params;
	//	collide::rq_result* RQ	= pp->RQ;
	++pp->pass;

	if (result.O)
	{
		pp->result = result;
		return FALSE;
	}
	else
	{
		//получить треугольник и узнать его материал
		CDB::TRI* T = Level().ObjectSpace.GetStaticTris() + result.element;

		SGameMtl* mtl = GMLib.GetMaterialByIdx(T->material);
		pp->power *= mtl->fVisTransparencyFactor;
		if (pp->power > 0.34f)
		{
			return TRUE;
		}
		//.		if (mtl->Flags.is(SGameMtl::flPassable)) 
		//.			return TRUE;
	}
	pp->result = result;
	return FALSE;
}

bool CHUDManager::DoPick(SPickParam& pp)
{
	VERIFY(!fis_zero(pp.defs.dir.square_magnitude()));

	pp.result.set(NULL, pp.defs.range, -1);
	pp.power = 1.0f;
	pp.pass = 0;

	collide::rq_results rqr;
	rqr.r_clear();
	return Level().ObjectSpace.RayQuery(
		rqr,
		pp.defs,
		pick_trace_callback,
		&pp,
		NULL,
		Level().CurrentEntity()
	);
}

void CHUDManager::SetCrosshairDisp(float dispf, float disps)
{
	m_pHUDTarget->SetDispersion(psHUD_Flags.test(HUD_CROSSHAIR_DYNAMIC) ? dispf : disps);
}

#ifdef DEBUG
void CHUDManager::SetFirstBulletCrosshairDisp(float fbdispf)
{
    m_pHUDTarget->GetHUDCrosshair().SetFirstBulletDispertion(fbdispf);
}
#endif

void CHUDManager::ShowCrosshair(bool show)
{
	m_pHUDTarget->ShowCrosshair(show);
}

void CHUDManager::HitMarked(int idx, float power, const Fvector& dir)
{
	HitMarker.Hit(dir);
	clamp(power, 0.0f, 1.0f);
	pInput->feedback(u16(iFloor(u16(-1) * power)), u16(iFloor(u16(-1) * power)), 0.5f);
}

bool CHUDManager::AddGrenade_ForMark(CGrenade* grn)
{
	return HitMarker.AddGrenade_ForMark(grn);
}

void CHUDManager::Update_GrenadeView(Fvector& pos_actor)
{
	HitMarker.Update_GrenadeView(pos_actor);
}

void CHUDManager::SetHitmarkType(LPCSTR tex_name)
{
	HitMarker.InitShader(tex_name);
}

void CHUDManager::SetGrenadeMarkType(LPCSTR tex_name)
{
	HitMarker.InitShader_Grenade(tex_name);
}

// ------------------------------------------------------------------------------------

#include "ui\UIMainInGameWnd.h"
extern CUIXml* pWpnScopeXml;
extern CUIXml* g_uiSpotXml;

void CHUDManager::Load()
{
	if (!pUIGame)
	{
		pUIGame = Game().createGameUI();
	}
	else
	{
		pUIGame->SetClGame(&Game());
	}
}

void CHUDManager::OnScreenResolutionChanged()
{
	pUIGame->HideShownDialogs();

	xr_delete(pWpnScopeXml);
	xr_delete(g_uiSpotXml);

	pUIGame->UnLoad();

	Level().MapManager().ReloadSpots();

	pUIGame->Load();

	pUIGame->OnConnected();

	::luabind::functor<bool> funct;
	if (ai().script_engine().functor("_G.CHUDManager_OnScreenResolutionChanged", funct))
		funct();
}

void CHUDManager::OnDisconnected()
{
	b_online = false;
	if (pUIGame)
		Device.seqFrame.Remove(pUIGame);
}

void CHUDManager::OnConnected()
{
	if (b_online) return;
	b_online = true;
	if (pUIGame)
		Device.seqFrame.Add(pUIGame, REG_PRIORITY_LOW - 1000);
}

void CHUDManager::net_Relcase(CObject* obj)
{
	if (PP.result.O == obj)
		PP.result.O = NULL;

	HitMarker.net_Relcase(obj);

	VERIFY(g_player_hud);
	g_player_hud->net_Relcase(obj);

#ifdef	DEBUG
    DBG_PH_NetRelcase( obj );
#endif
}

CDialogHolder* CurrentDialogHolder()
{
	if (MainMenu()->IsActive())
		return MainMenu();
	else
		return HUD().GetGameUI();
}

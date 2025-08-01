#include "stdafx.h"
#include "EliteDetector.h"
#include "player_hud.h"
#include "../Include/xrRender/UIRender.h"
#include "ui/UIXmlInit.h"
#include "ui/xrUIXmlParser.h"
#include "ui/UIStatic.h"
#include "ui/ArtefactDetectorUI.h"


CEliteDetector::CEliteDetector()
{
	m_artefacts.m_af_rank = 3;
}

CEliteDetector::~CEliteDetector()
{
}


void CEliteDetector::CreateUI()
{
	R_ASSERT(nullptr==m_ui);
	m_ui = xr_new<CUIArtefactDetectorElite>();
	ui().construct(this);
}

CUIArtefactDetectorElite& CEliteDetector::ui()
{
	return *((CUIArtefactDetectorElite*)m_ui);
}

void CEliteDetector::ResetUI()
{
	if (m_ui) ui().Clear();
}

void CEliteDetector::UpdateAf()
{
	ui().Clear();
	if (m_artefacts.m_ItemInfos.size() == 0) return;

	CAfList::ItemsMapIt it_b = m_artefacts.m_ItemInfos.begin();
	CAfList::ItemsMapIt it_e = m_artefacts.m_ItemInfos.end();
	CAfList::ItemsMapIt it = it_b;

	Fvector detector_pos = Position();
	for (; it_b != it_e; ++it_b)
	{
		CArtefact* pAf = it_b->first;
		if (pAf->H_Parent())
			continue;

		ui().RegisterItemToDraw(pAf->Position(), "af_sign");

		if (pAf->CanBeInvisible())
		{
			float d = detector_pos.distance_to(pAf->Position());
			if (d < m_fAfVisRadius)
				pAf->SwitchVisibility(true);
		}
	}
}

void CEliteDetector::render_item_3d_ui()
{
	ui().Draw();

	//	Restore cull mode
	inherited::render_item_3d_ui();
}

void fix_ws_wnd_size(CUIWindow* w, float kx)
{
	Fvector2 p = w->GetWndSize();
	p.x /= kx;
	w->SetWndSize(p);

	p = w->GetWndPos();
	p.x /= kx;
	w->SetWndPos(p);

	CUIWindow::WINDOW_LIST::iterator it = w->GetChildWndList().begin();
	CUIWindow::WINDOW_LIST::iterator it_e = w->GetChildWndList().end();

	for (; it != it_e; ++it)
	{
		CUIWindow* w2 = *it;
		fix_ws_wnd_size(w2, kx);
	}
}

void CUIArtefactDetectorElite::construct(CEliteDetector* p)
{
	m_parent = p;
	CUIXml uiXml;
	uiXml.Load(CONFIG_PATH, UI_PATH, "ui_detector_artefact.xml");

	CUIXmlInit xml_init;
	string512 buff;
	xr_strcpy(buff, p->ui_xml_tag());

	xml_init.InitWindow(uiXml, buff, 0, this);

	m_wrk_area = xr_new<CUIWindow>();

	xr_sprintf(buff, "%s:wrk_area", p->ui_xml_tag());

	xml_init.InitWindow(uiXml, buff, 0, m_wrk_area);
	m_wrk_area->SetAutoDelete(true);
	AttachChild(m_wrk_area);

	xr_sprintf(buff, "%s", p->ui_xml_tag());
	int num = uiXml.GetNodesNum(buff, 0, "palette");
	XML_NODE* pStoredRoot = uiXml.GetLocalRoot();
	uiXml.SetLocalRoot(uiXml.NavigateToNode(buff, 0));
	for (int idx = 0; idx < num; ++idx)
	{
		CUIStatic* S = xr_new<CUIStatic>();
		shared_str name = uiXml.ReadAttrib("palette", idx, "id");
		m_palette[name] = S;
		xml_init.InitStatic(uiXml, "palette", idx, S);
		S->SetAutoDelete(true);
		m_wrk_area->AttachChild(S);
		S->SetCustomDraw(true);
	}
	uiXml.SetLocalRoot(pStoredRoot);

	Fvector _map_attach_p = pSettings->r_fvector3(m_parent->cNameSect(), "ui_p");
	Fvector _map_attach_r = pSettings->r_fvector3(m_parent->cNameSect(), "ui_r");

	_map_attach_r.mul(PI / 180.f);
	m_map_attach_offset.setHPB(_map_attach_r.x, _map_attach_r.y, _map_attach_r.z);
	m_map_attach_offset.translate_over(_map_attach_p);
}

void CUIArtefactDetectorElite::update()
{
	inherited::update();
	CUIWindow::Update();
}

void CUIArtefactDetectorElite::Draw()
{
	Fmatrix LM;
	GetUILocatorMatrix(LM);

	UIRender->CacheSetXformWorld(LM);

	CUIWindow::Draw();

	//.	Frect r						= m_wrk_area->GetWndRect();
	Fvector2 wrk_sz = m_wrk_area->GetWndSize();
	Fvector2 rp;
	m_wrk_area->GetAbsolutePos(rp);

	Fmatrix M, Mc;
	float h, p;
	Device.vCameraDirection.getHP(h, p);
	Mc.setHPB(h, 0, 0);
	Mc.c.set(Device.vCameraPosition);
	M.invert(Mc);

	UI().ScreenFrustumLIT().CreateFromRect(Frect().set(rp.x, rp.y, wrk_sz.x, wrk_sz.y));

	xr_vector<SDrawOneItem>::const_iterator it = m_items_to_draw.begin();
	xr_vector<SDrawOneItem>::const_iterator it_e = m_items_to_draw.end();
	for (; it != it_e; ++it)
	{
		Fvector p = (*it).pos;
		Fvector pt3d;
		M.transform_tiny(pt3d, p);
		float kz = wrk_sz.y / m_parent->m_fAfDetectRadius;
		pt3d.x *= kz;
		pt3d.z *= kz;

		pt3d.x += wrk_sz.x / 2.0f;
		pt3d.z -= wrk_sz.y;

		Fvector2 pos;
		pos.set(pt3d.x, -pt3d.z);
		pos.sub(rp);
		if (1 /* r.in(pos)*/)
		{
			(*it).pStatic->SetWndPos(pos);
			(*it).pStatic->Draw();
		}
	}

	UI().ScreenFrustumLIT().Clear();
}

void CUIArtefactDetectorElite::GetUILocatorMatrix(Fmatrix& _m)
{
	Fmatrix trans = m_parent->HudItemData()->m_item_transform;

	// Lucy: custom boneid for ui
	u16 bid = m_parent->HudItemData()->m_model->LL_BoneID(READ_IF_EXISTS(pSettings, r_string, *m_parent->HudItemData()->m_sect_name, "detector_ui_bone", "cover"));

	Fmatrix cover_bone = m_parent->HudItemData()->m_model->LL_GetTransform(bid);
	_m.mul(trans, cover_bone);
	_m.mulB_43(m_map_attach_offset);
}


void CUIArtefactDetectorElite::Clear()
{
	m_items_to_draw.clear();
}

void CUIArtefactDetectorElite::RegisterItemToDraw(const Fvector& p, const shared_str& palette_idx)
{
	xr_map<shared_str, CUIStatic*>::iterator it = m_palette.find(palette_idx);
	if (it == m_palette.end())
	{
		Msg("! RegisterItemToDraw. static not found for [%s]", palette_idx.c_str());
		return;
	}
	CUIStatic* S = m_palette[palette_idx];
	SDrawOneItem itm(S, p);
	m_items_to_draw.push_back(itm);
}

CScientificDetector::CScientificDetector()
{
	m_artefacts.m_af_rank = 3;
}

CScientificDetector::~CScientificDetector()
{
	m_zones.destroy();
}

void CScientificDetector::Load(LPCSTR section)
{
	inherited::Load(section);
	m_zones.load(section, "zone");
}

void CScientificDetector::UpdateWork()
{
	ui().Clear();

	CAfList::ItemsMapIt ait_b = m_artefacts.m_ItemInfos.begin();
	CAfList::ItemsMapIt ait_e = m_artefacts.m_ItemInfos.end();
	CAfList::ItemsMapIt ait = ait_b;
	Fvector detector_pos = Position();
	for (; ait_b != ait_e; ++ait_b)
	{
		CArtefact* pAf = ait_b->first;
		if (pAf->H_Parent())
			continue;

		ui().RegisterItemToDraw(pAf->Position(), pAf->cNameSect());

		if (pAf->CanBeInvisible())
		{
			float d = detector_pos.distance_to(pAf->Position());
			if (d < m_fAfVisRadius)
				pAf->SwitchVisibility(true);
		}
	}

	CZoneList::ItemsMapIt zit_b = m_zones.m_ItemInfos.begin();
	CZoneList::ItemsMapIt zit_e = m_zones.m_ItemInfos.end();
	CZoneList::ItemsMapIt zit = zit_b;

	for (; zit_b != zit_e; ++zit_b)
	{
		CCustomZone* pZone = zit_b->first;
		ui().RegisterItemToDraw(pZone->Position(), pZone->cNameSect());
	}

	m_ui->update();
}

void CScientificDetector::shedule_Update(u32 dt)
{
	inherited::shedule_Update(dt);

	if (!H_Parent()) return;
	Fvector P;
	P.set(H_Parent()->Position());
	m_zones.feel_touch_update(P, m_fAfDetectRadius);
}

void CScientificDetector::OnH_B_Independent(bool just_before_destroy)
{
	inherited::OnH_B_Independent(just_before_destroy);

	m_zones.clear();
}

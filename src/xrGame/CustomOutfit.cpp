////////////////////////////////////////////////////////////////////////////
//	Modified by Axel DominatoR
//	Last updated: 13/08/2015
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "customoutfit.h"
#include "../xrphysics/PhysicsShell.h"
#include "inventory_space.h"
#include "Inventory.h"
#include "Actor.h"
#include "game_cl_base.h"
#include "Level.h"
#include "BoneProtections.h"
#include "../Include/xrRender/Kinematics.h"
#include "player_hud.h"
#include "ActorHelmet.h"


CCustomOutfit::CCustomOutfit()
{
	m_flags.set(FUsingCondition, TRUE);

	m_HitTypeProtection.resize(ALife::eHitTypeMax);
	for (int i = 0; i < ALife::eHitTypeMax; i++)
		m_HitTypeProtection[i] = 1.0f;

	m_boneProtection = xr_new<SBoneProtections>();
	m_artefact_count = 0;
	m_BonesProtectionSect = NULL;
}

CCustomOutfit::~CCustomOutfit()
{
	xr_delete(m_boneProtection);
}

BOOL CCustomOutfit::net_Spawn(CSE_Abstract* DC)
{
	if (IsGameTypeSingle())
		ReloadBonesProtection();

	BOOL res = inherited::net_Spawn(DC);
	return (res);
}

void CCustomOutfit::net_Export(NET_Packet& P)
{
	inherited::net_Export(P);
	P.w_float_q8(GetCondition(), 0.0f, 1.0f);
}

void CCustomOutfit::net_Import(NET_Packet& P)
{
	inherited::net_Import(P);
	float _cond;
	P.r_float_q8(_cond, 0.0f, 1.0f);
	SetCondition(_cond);
}

void CCustomOutfit::OnH_A_Chield()
{
	inherited::OnH_A_Chield();
	if (!IsGameTypeSingle())
		ReloadBonesProtection();
}


void CCustomOutfit::Load(LPCSTR section)
{
	inherited::Load(section);

	m_HitTypeProtection[ALife::eHitTypeBurn] = pSettings->r_float(section, "burn_protection");
	m_HitTypeProtection[ALife::eHitTypeStrike] = pSettings->r_float(section, "strike_protection");
	m_HitTypeProtection[ALife::eHitTypeShock] = pSettings->r_float(section, "shock_protection");
	m_HitTypeProtection[ALife::eHitTypeWound] = pSettings->r_float(section, "wound_protection");
	m_HitTypeProtection[ALife::eHitTypeRadiation] = pSettings->r_float(section, "radiation_protection");
	m_HitTypeProtection[ALife::eHitTypeTelepatic] = pSettings->r_float(section, "telepatic_protection");
	m_HitTypeProtection[ALife::eHitTypeChemicalBurn] = pSettings->r_float(section, "chemical_burn_protection");
	m_HitTypeProtection[ALife::eHitTypeExplosion] = pSettings->r_float(section, "explosion_protection");
	m_HitTypeProtection[ALife::eHitTypeFireWound] = 0.f; //pSettings->r_float(section,"fire_wound_protection");
	//	m_HitTypeProtection[ALife::eHitTypePhysicStrike]= pSettings->r_float(section,"physic_strike_protection");
	m_HitTypeProtection[ALife::eHitTypeLightBurn] = m_HitTypeProtection[ALife::eHitTypeBurn];
	m_boneProtection->m_fHitFracActor = pSettings->r_float(section, "hit_fraction_actor");

	if (pSettings->line_exist(section, "nightvision_sect"))
		m_NightVisionSect = pSettings->r_string(section, "nightvision_sect");
	else
		m_NightVisionSect = "";

	if (pSettings->line_exist(section, "actor_visual"))
		m_ActorVisual = pSettings->r_string(section, "actor_visual");
	else
		m_ActorVisual = NULL;

	m_ef_equipment_type = pSettings->r_u32(section, "ef_equipment_type");
	m_fPowerLoss = READ_IF_EXISTS(pSettings, r_float, section, "power_loss", 1.0f);
	clamp(m_fPowerLoss, EPS, 1.0f);

	m_additional_weight = pSettings->r_float(section, "additional_inventory_weight");
	m_additional_weight2 = pSettings->r_float(section, "additional_inventory_weight2");

	m_fHealthRestoreSpeed = READ_IF_EXISTS(pSettings, r_float, section, "health_restore_speed", 0.0f);
	m_fRadiationRestoreSpeed = READ_IF_EXISTS(pSettings, r_float, section, "radiation_restore_speed", 0.0f);
	m_fSatietyRestoreSpeed = READ_IF_EXISTS(pSettings, r_float, section, "satiety_restore_speed", 0.0f);
	m_fPowerRestoreSpeed = READ_IF_EXISTS(pSettings, r_float, section, "power_restore_speed", 0.0f);
	m_fBleedingRestoreSpeed = READ_IF_EXISTS(pSettings, r_float, section, "bleeding_restore_speed", 0.0f);
	m_fControlInertionFactor = READ_IF_EXISTS(pSettings, r_float, section, "control_inertion_factor", 1.0f);

	m_full_icon_name = pSettings->r_string(section, "full_icon_name");
	m_artefact_count = READ_IF_EXISTS(pSettings, r_u32, section, "artefact_count", 0);
	//clamp(m_artefact_count, (u32)0, (u32)5);
	m_artefact_count = _max(m_artefact_count, (u32)0);

	m_BonesProtectionSect = READ_IF_EXISTS(pSettings, r_string, section, "bones_koeff_protection", "");
	bIsHelmetAvaliable = !!READ_IF_EXISTS(pSettings, r_bool, section, "helmet_avaliable", true);
	bIsBackpackAvaliable = !!READ_IF_EXISTS(pSettings, r_bool, section, "backpack_avaliable", true);

	// Added by Axel, to enable optional condition use on any item
	m_flags.set(FUsingCondition, READ_IF_EXISTS(pSettings, r_bool, section, "use_condition", TRUE));
}

void CCustomOutfit::ReloadBonesProtection()
{
	CObject* parent = H_Parent();
	if (IsGameTypeSingle())
		parent = smart_cast<CObject*>(Level().CurrentViewEntity());

	if (parent && parent->Visual() && m_BonesProtectionSect.size())
		m_boneProtection->reload(m_BonesProtectionSect, smart_cast<IKinematics*>(parent->Visual()));
}

void CCustomOutfit::Hit(float hit_power, ALife::EHitType hit_type)
{
	hit_power *= GetHitImmunity(hit_type);
	ChangeCondition(-hit_power);
}

float CCustomOutfit::GetDefHitTypeProtection(ALife::EHitType hit_type)
{
	return m_HitTypeProtection[hit_type] * GetCondition();
}

float CCustomOutfit::GetHitTypeProtection(ALife::EHitType hit_type, s16 element)
{
	float fBase = m_HitTypeProtection[hit_type] * GetCondition();
	float bone = m_boneProtection->getBoneProtection(element);
	return fBase * bone;
}

float CCustomOutfit::GetBoneArmor(s16 element)
{
	return m_boneProtection->getBoneArmor(element);
}
#pragma optimize( "", off )
float CCustomOutfit::HitThroughArmor(float hit_power, s16 element, float ap, bool& add_wound, ALife::EHitType hit_type)
{
	if (strstr(Core.Params, "-dbgbullet"))
		Msg("CCustomOutfit::HitThroughArmor hit_type=%d | unmodified hit_power=%f", (u32)hit_type, hit_power);

	float NewHitPower = hit_power;
	if (hit_type == ALife::eHitTypeFireWound)
	{
		float ba = GetBoneArmor(element);
		if (ba <= 0.0f)
			return NewHitPower;

		float BoneArmor = ba * GetCondition();
		if (ap <= BoneArmor)
		{
			//пуля НЕ пробила бронь
			NewHitPower *= m_boneProtection->m_fHitFracActor;
			//add_wound = false; 	//раны нет

			if (strstr(Core.Params, "-dbgbullet"))
				Msg("CCustomOutfit::HitThroughArmor AP(%f) <= bone_armor(%f) [HitFracActor=%f] modified hit_power=%f",
				    ap, BoneArmor, m_boneProtection->m_fHitFracActor, NewHitPower);
		}

		else
		{
			float d_hit_power = (ap - BoneArmor) / (ap * m_boneProtection->APScale);
			clamp(d_hit_power, m_boneProtection->m_fHitFracActor, 1.0f);

			NewHitPower *= d_hit_power;

			if (strstr(Core.Params, "-dbgbullet"))
				Msg("CCustomOutfit::HitThroughArmor AP(%f) > bone_armor(%f) [HitFracActor=%f] modified hit_power=%f",
				    ap, BoneArmor, m_boneProtection->m_fHitFracActor, NewHitPower);
		}
	}
	else
	{
		float one = 0.1f;
		if (hit_type == ALife::eHitTypeStrike ||
			hit_type == ALife::eHitTypeWound ||
			hit_type == ALife::eHitTypeWound_2 ||
			hit_type == ALife::eHitTypeExplosion)
		{
			one = 1.0f;
		}
		float protect = GetDefHitTypeProtection(hit_type);
		NewHitPower -= protect * one;

		if (NewHitPower < 0.f)
			NewHitPower = 0.f;

		if (strstr(Core.Params, "-dbgbullet"))
			Msg("CCustomOutfit::HitThroughArmor hit_type=%d | After HitTypeProtection(%f) hit_power=%f", (u32)hit_type,
			    protect * one, NewHitPower);
	}
	//увеличить изношенность костюма
	Hit(hit_power, hit_type);

	if (strstr(Core.Params, "-dbgbullet"))
		Msg("CCustomOutfit::HitThroughArmor hit_type=%d | After Immunities hit_power=%f", (u32)hit_type, NewHitPower);

	return NewHitPower;
}
#pragma optimize( "", on )
BOOL CCustomOutfit::BonePassBullet(int boneID)
{
	return m_boneProtection->getBonePassBullet(s16(boneID));
}

#include "torch.h"

void CCustomOutfit::OnMoveToSlot(const SInvItemPlace& prev)
{
	if (m_pInventory)
	{
		CActor* pActor = smart_cast<CActor*>(H_Parent());
		if (pActor)
		{
			ApplySkinModel(pActor, true, false);
			if (prev.type == eItemPlaceSlot && !bIsHelmetAvaliable)
			{
				if (pActor->GetNightVisionStatus())
					pActor->SwitchNightVision(true, false);
			}
			PIItem pHelmet = pActor->inventory().ItemFromSlot(HELMET_SLOT);
			if (pHelmet && !bIsHelmetAvaliable)
				pActor->inventory().Ruck(pHelmet, false);

			PIItem pBackpack = pActor->inventory().ItemFromSlot(BACKPACK_SLOT);
			if (pBackpack && !bIsBackpackAvaliable)
				pActor->inventory().Ruck(pBackpack, false);
		}
	}
}

void CCustomOutfit::ApplySkinModel(CActor* pActor, bool bDress, bool bHUDOnly)
{
	if (bDress)
	{
		if (!bHUDOnly && m_ActorVisual.size())
		{
			shared_str NewVisual = NULL;
			char* TeamSection = Game().getTeamSection(pActor->g_Team());
			if (TeamSection)
			{
				if (pSettings->line_exist(TeamSection, *cNameSect()))
				{
					NewVisual = pSettings->r_string(TeamSection, *cNameSect());
					string256 SkinName;

					xr_strcpy(SkinName, pSettings->r_string("mp_skins_path", "skin_path"));
					xr_strcat(SkinName, *NewVisual);
					xr_strcat(SkinName, ".ogf");
					NewVisual._set(SkinName);
				}
			}
			if (!NewVisual.size())
				NewVisual = m_ActorVisual;

			pActor->ChangeVisual(NewVisual);
		}


		if (pActor == Level().CurrentViewEntity())
			g_player_hud->load(pSettings->r_string(cNameSect(), "player_hud_section"));
	}
	else
	{
		if (!bHUDOnly && m_ActorVisual.size())
		{
			shared_str DefVisual = pActor->GetDefaultVisualOutfit();
			if (DefVisual.size())
			{
				pActor->ChangeVisual(DefVisual);
			};
		}

		if (pActor == Level().CurrentViewEntity())
			g_player_hud->load_default();
	}
}

void CCustomOutfit::OnMoveToRuck(const SInvItemPlace& prev)
{
	if (m_pInventory && prev.type == eItemPlaceSlot)
	{
		CActor* pActor = smart_cast<CActor*>(H_Parent());
		if (pActor)
		{
			ApplySkinModel(pActor, false, false);
			if (!bIsHelmetAvaliable)
				pActor->SwitchNightVision(false);
		}
	}
};

u32 CCustomOutfit::ef_equipment_type() const
{
	return (m_ef_equipment_type);
}

u32 CCustomOutfit::get_artefact_count() const
{
	u32 artefact_count = m_artefact_count;
	::luabind::functor<int> funct;
		if (ai().script_engine().functor("_G.CCustomOutfit_get_artefact_count", funct))
		{
			artefact_count = funct(artefact_count);
		}
	return (artefact_count);
}

bool CCustomOutfit::install_upgrade_impl(LPCSTR section, bool test)
{
	bool result = inherited::install_upgrade_impl(section, test);

	result |= process_if_exists(section, "burn_protection", &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeBurn], test);
	result |= process_if_exists(section, "shock_protection", &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeShock], test);
	result |= process_if_exists(section, "strike_protection", &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeStrike], test);
	result |= process_if_exists(section, "wound_protection", &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeWound], test);
	result |= process_if_exists(section, "radiation_protection", &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeRadiation], test);
	result |= process_if_exists(section, "telepatic_protection", &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeTelepatic], test);
	result |= process_if_exists(section, "chemical_burn_protection", &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeChemicalBurn], test);
	result |= process_if_exists(section, "explosion_protection", &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeExplosion], test);
	result |= process_if_exists(section, "fire_wound_protection", &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypeFireWound], test);
	//	result |= process_if_exists( section, "physic_strike_protection", &CInifile::r_float, m_HitTypeProtection[ALife::eHitTypePhysicStrike], test );
	LPCSTR str;
	bool result2 = process_if_exists_set(section, "nightvision_sect", &CInifile::r_string, str, test);
	if (result2 && !test)
	{
		m_NightVisionSect._set(str);
	}
	result |= result2;

	result2 = process_if_exists_set(section, "bones_koeff_protection", &CInifile::r_string, str, test);
	if (result2 && !test)
	{
		m_BonesProtectionSect = str;
		ReloadBonesProtection();
	}
	result2 = process_if_exists_set(section, "bones_koeff_protection_add", &CInifile::r_string, str, test);
	if (result2 && !test)
		AddBonesProtection(str);

	result |= result2;
	result |= process_if_exists(section, "hit_fraction_actor", &CInifile::r_float, m_boneProtection->m_fHitFracActor, test);

	result |= process_if_exists(section, "additional_inventory_weight", &CInifile::r_float, m_additional_weight, test);
	result |= process_if_exists(section, "additional_inventory_weight2", &CInifile::r_float, m_additional_weight2, test);

	result |= process_if_exists(section, "health_restore_speed", &CInifile::r_float, m_fHealthRestoreSpeed, test);
	result |= process_if_exists(section, "radiation_restore_speed", &CInifile::r_float, m_fRadiationRestoreSpeed, test);
	result |= process_if_exists(section, "satiety_restore_speed", &CInifile::r_float, m_fSatietyRestoreSpeed, test);
	result |= process_if_exists(section, "power_restore_speed", &CInifile::r_float, m_fPowerRestoreSpeed, test);
	result |= process_if_exists(section, "bleeding_restore_speed", &CInifile::r_float, m_fBleedingRestoreSpeed, test);

	result |= process_if_exists(section, "power_loss", &CInifile::r_float, m_fPowerLoss, test);
	clamp(m_fPowerLoss, 0.0f, 1.0f);

	result |= process_if_exists(section, "artefact_count", &CInifile::r_u32, m_artefact_count, test);
	//clamp(m_artefact_count, (u32)0, (u32)5);
	m_artefact_count = _max(m_artefact_count, (u32)0);

	return result;
}

void CCustomOutfit::AddBonesProtection(LPCSTR bones_section)
{
	CObject* parent = H_Parent();
	if (IsGameTypeSingle())
		parent = smart_cast<CObject*>(Level().CurrentViewEntity());

	if (parent && parent->Visual() && m_BonesProtectionSect.size())
		m_boneProtection->add(bones_section, smart_cast<IKinematics*>(parent->Visual()));
}

float CCustomOutfit::get_HitFracActor() const
{
	return m_boneProtection->m_fHitFracActor;
}
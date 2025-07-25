////////////////////////////////////////////////////////////////////////////
//	Module 		: script_game_object.cpp
//	Created 	: 25.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script game object class
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_game_object.h"
#include "script_game_object_impl.h"
#include "script_entity_action.h"
#include "ai_space.h"
#include "script_engine.h"
#include "script_entity.h"
#include "physicsshellholder.h"
#include "helicopter.h"
#include "holder_custom.h"
#include "inventoryowner.h"
#include "movement_manager.h"
#include "entity_alive.h"
#include "weaponmagazined.h"
#include "xrmessages.h"
#include "inventory.h"
#include "script_ini_file.h"
#include "../Include/xrRender/Kinematics.h"
#include "HangingLamp.h"
#include "patrol_path_manager.h"
#include "ai_object_location.h"
#include "custommonster.h"
#include "entitycondition.h"
#include "space_restrictor.h"
#include "detail_path_manager.h"
#include "level_graph.h"
#include "actor.h"
#include "actor_memory.h"
#include "visual_memory_manager.h"
#include "smart_cover_object.h"
#include "smart_cover.h"
#include "smart_cover_description.h"
#include "physics_shell_scripted.h"
#include "ai\phantom\phantom.h"

#include "UIGameSP.h"
#include "uigamecustom.h"
#include "ui/UIActorMenu.h"
#include "InventoryBox.h"
#include "Pda.h"
#include "player_hud.h"
#include "script_attachment_manager.h"

class CScriptBinderObject;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
Fvector CScriptGameObject::Center(bool bHud)
{
	if (bHud)
	{
		CHudItem* itm = smart_cast<CHudItem*>(&object());
		if (itm)
			return itm->HudItemData()->m_model->dcast_RenderVisual()->getVisData().sphere.P;
	}

	Fvector c;
	object().Center(c);
	return c;
}

Fmatrix CScriptGameObject::Xform(bool bHud)
{
	Fmatrix* xform = nullptr;

	if (bHud)
	{
		CHudItem* itm = smart_cast<CHudItem*>(&object());
		if (itm)
		{
			xform = &itm->HudItemData()->m_item_transform;
		}
	} else {
		xform = &object().XFORM();
	}

	if (!xform) {
		Msg("!CScriptGameObject::XForm xform is null for %s", object().Name());
		return Fmatrix().identity();
	}

	return *xform;
}

Fbox CScriptGameObject::bounding_box(bool bHud)
{	
	if (bHud)
	{
		CHudItem* itm = smart_cast<CHudItem*>(&object());
		if (itm)
			return itm->HudItemData()->m_model->dcast_RenderVisual()->getVisData().box;
	}

	return object().BoundingBox();
}

BIND_FUNCTION10(&object(), CScriptGameObject::Position, CGameObject, Position, Fvector, Fvector());
BIND_FUNCTION10(&object(), CScriptGameObject::Direction, CGameObject, Direction, Fvector, Fvector());
BIND_FUNCTION10(&object(), CScriptGameObject::Mass, CPhysicsShellHolder, GetMass, float, float(-1));
BIND_FUNCTION10(&object(), CScriptGameObject::ID, CGameObject, ID, u16, u16(-1));
BIND_FUNCTION10(&object(), CScriptGameObject::getVisible, CGameObject, getVisible, BOOL, FALSE);
//BIND_FUNCTION01	(&object(),	CScriptGameObject::setVisible,			CGameObject,	setVisible,			BOOL,							BOOL);
BIND_FUNCTION10(&object(), CScriptGameObject::getEnabled, CGameObject, getEnabled, BOOL, FALSE);
//BIND_FUNCTION01	(&object(),	CScriptGameObject::setEnabled,			CGameObject,	setEnabled,			BOOL,							BOOL);
BIND_FUNCTION10(&object(), CScriptGameObject::story_id, CGameObject, story_id, ALife::_STORY_ID, ALife::_STORY_ID(-1));
BIND_FUNCTION10(&object(), CScriptGameObject::DeathTime, CEntity, GetLevelDeathTime, u32, 0);
BIND_FUNCTION10(&object(), CScriptGameObject::MaxHealth, CEntity, GetMaxHealth, float, -1);
BIND_FUNCTION10(&object(), CScriptGameObject::Accuracy, CInventoryOwner, GetWeaponAccuracy, float, -1);
BIND_FUNCTION10(&object(), CScriptGameObject::Team, CEntity, g_Team, int, -1);
BIND_FUNCTION10(&object(), CScriptGameObject::Squad, CEntity, g_Squad, int, -1);
BIND_FUNCTION10(&object(), CScriptGameObject::Group, CEntity, g_Group, int, -1);
BIND_FUNCTION10(&object(), CScriptGameObject::GetFOV, CEntityAlive, ffGetFov, float, -1);
BIND_FUNCTION10(&object(), CScriptGameObject::GetRange, CEntityAlive, ffGetRange, float, -1);

BIND_FUNCTION10(&object(), CScriptGameObject::GetHealth, CEntityAlive, conditions().GetHealth, float, -1);
BIND_FUNCTION01(&object(), CScriptGameObject::SetHealth, CEntityAlive, conditions().SetHealth, float, float);
BIND_FUNCTION01(&object(), CScriptGameObject::ChangeHealth, CEntityAlive, conditions().ChangeHealth, float, float);

BIND_FUNCTION10(&object(), CScriptGameObject::GetPsyHealth, CEntityAlive, conditions().GetPsyHealth, float, -1);
BIND_FUNCTION01(&object(), CScriptGameObject::SetPsyHealth, CEntityAlive, conditions().SetPsyHealth, float, float);
BIND_FUNCTION01(&object(), CScriptGameObject::ChangePsyHealth, CEntityAlive, conditions().ChangePsyHealth, float, float);

BIND_FUNCTION10(&object(), CScriptGameObject::GetPower, CEntityAlive, conditions().GetPower, float, -1);
BIND_FUNCTION01(&object(), CScriptGameObject::SetPower, CEntityAlive, conditions().SetPower, float, float);
BIND_FUNCTION01(&object(), CScriptGameObject::ChangePower, CEntityAlive, conditions().ChangePower, float, float);

BIND_FUNCTION10(&object(), CScriptGameObject::GetSatiety, CEntityAlive, conditions().GetSatiety, float, -1);
BIND_FUNCTION01(&object(), CScriptGameObject::SetSatiety, CEntityAlive, conditions().SetSatiety, float, float);
BIND_FUNCTION01(&object(), CScriptGameObject::ChangeSatiety, CEntityAlive, conditions().ChangeSatiety, float, float);

BIND_FUNCTION10(&object(), CScriptGameObject::GetRadiation, CEntityAlive, conditions().GetRadiation, float, -1);
BIND_FUNCTION01(&object(), CScriptGameObject::SetRadiation, CEntityAlive, conditions().SetRadiation, float, float);
BIND_FUNCTION01(&object(), CScriptGameObject::ChangeRadiation, CEntityAlive, conditions().ChangeRadiation, float, float);

BIND_FUNCTION10(&object(), CScriptGameObject::GetMorale, CEntityAlive, conditions().GetEntityMorale, float, -1);
BIND_FUNCTION01(&object(), CScriptGameObject::SetMorale, CEntityAlive, conditions().SetEntityMorale, float, float);
BIND_FUNCTION01(&object(), CScriptGameObject::ChangeMorale, CEntityAlive, conditions().ChangeEntityMorale, float, float);

BIND_FUNCTION10(&object(), CScriptGameObject::GetBleeding, CEntityAlive, conditions().BleedingSpeed, float, -1);
BIND_FUNCTION01(&object(), CScriptGameObject::ChangeBleeding, CEntityAlive, conditions().ChangeBleeding, float, float);

BIND_FUNCTION01(&object(), CScriptGameObject::ChangeCircumspection, CEntityAlive, conditions().ChangeCircumspection, float, float);

BIND_FUNCTION02(&object(), CScriptGameObject::SetScriptControl, CScriptEntity, SetScriptControl, bool, LPCSTR, bool, shared_str);
BIND_FUNCTION10(&object(), CScriptGameObject::GetScriptControl, CScriptEntity, GetScriptControl, bool, false);
BIND_FUNCTION10(&object(), CScriptGameObject::GetScriptControlName, CScriptEntity, GetScriptControlName, LPCSTR, "");
BIND_FUNCTION10(&object(), CScriptGameObject::GetEnemyStrength, CScriptEntity, get_enemy_strength, int, 0);
BIND_FUNCTION10(&object(), CScriptGameObject::GetActionCount, CScriptEntity, GetActionCount, u32, 0);
BIND_FUNCTION10(&object(), CScriptGameObject::can_script_capture, CScriptEntity, can_script_capture, bool, 0);

BOOL print_bone_warnings = TRUE;

u32 CScriptGameObject::level_vertex_id() const
{
	return (object().ai_location().level_vertex_id());
}

u32 CScriptGameObject::game_vertex_id() const
{
	return (object().ai_location().game_vertex_id());
}

CScriptIniFile* CScriptGameObject::spawn_ini() const
{
	return ((CScriptIniFile*)object().spawn_ini());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void CScriptGameObject::ResetActionQueue()
{
	CScriptEntity* l_tpScriptMonster = smart_cast<CScriptEntity*>(&object());
	if (!l_tpScriptMonster)
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CSciptEntity : cannot access class member ResetActionQueue!");
	else
		l_tpScriptMonster->ClearActionQueue();
}

CScriptEntityAction* CScriptGameObject::GetCurrentAction() const
{
	CScriptEntity* l_tpScriptMonster = smart_cast<CScriptEntity*>(&object());
	if (!l_tpScriptMonster)
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CSciptEntity : cannot access class member GetCurrentAction!");
	else if (l_tpScriptMonster->GetCurrentAction())
		return (xr_new<CScriptEntityAction>(l_tpScriptMonster->GetCurrentAction()));
	return (0);
}

void CScriptGameObject::AddAction(const CScriptEntityAction* tpEntityAction, bool bHighPriority)
{
	CScriptEntity* l_tpScriptMonster = smart_cast<CScriptEntity*>(&object());
	if (!l_tpScriptMonster)
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CSciptEntity : cannot access class member AddAction!");
	else
		l_tpScriptMonster->AddAction(tpEntityAction, bHighPriority);
}

const CScriptEntityAction* CScriptGameObject::GetActionByIndex(u32 action_index)
{
	CScriptEntity* l_tpScriptMonster = smart_cast<CScriptEntity*>(&object());
	if (!l_tpScriptMonster)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CScriptEntity : cannot access class member GetActionByIndex!");
		return (0);
	}
	else
		return (l_tpScriptMonster->GetActionByIndex(action_index));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

cphysics_shell_scripted* CScriptGameObject::get_physics_shell() const
{
	CPhysicsShellHolder* ph_shell_holder = smart_cast<CPhysicsShellHolder*>(&object());
	if (!ph_shell_holder)
		return NULL;
	if (!ph_shell_holder->PPhysicsShell())
		return NULL;
	return get_script_wrapper<cphysics_shell_scripted>(*ph_shell_holder->PPhysicsShell());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CHelicopter* CScriptGameObject::get_helicopter()
{
	CHelicopter* helicopter = smart_cast<CHelicopter*>(&object());
	if (!helicopter)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CGameObject : cannot access class member get_helicopter!");
		NODEFAULT;
	}
	return helicopter;
}

CHangingLamp* CScriptGameObject::get_hanging_lamp()
{
	CHangingLamp* lamp = smart_cast<CHangingLamp*>(&object());
	if (!lamp)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "CGameObject : it is not a lamp!");
		NODEFAULT;
	}
	return lamp;
}

CHolderCustom* CScriptGameObject::get_custom_holder()
{
	CHolderCustom* holder = smart_cast<CHolderCustom*>(&object());
	if (!holder)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "CGameObject : it is not a holder!");
	}
	return holder;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

LPCSTR CScriptGameObject::WhoHitName()
{
	CEntityAlive* entity_alive = smart_cast<CEntityAlive*>(&object());
	if (entity_alive)
		return entity_alive->conditions().GetWhoHitLastTime()
			       ? (*entity_alive->conditions().GetWhoHitLastTime()->cName())
			       : NULL;
	else
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CScriptGameObject : cannot access class member  WhoHitName()");
		return NULL;
	}
}

LPCSTR CScriptGameObject::WhoHitSectionName()
{
	CEntityAlive* entity_alive = smart_cast<CEntityAlive*>(&object());
	if (entity_alive)
		return entity_alive->conditions().GetWhoHitLastTime()
			       ? (*entity_alive->conditions().GetWhoHitLastTime()->cNameSect())
			       : NULL;
	else
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CScriptGameObject : cannot access class member  WhoHitName()");
		return NULL;
	}
}

bool CScriptGameObject::CheckObjectVisibility(const CScriptGameObject* tpLuaGameObject)
{
	CEntityAlive* entity_alive = smart_cast<CEntityAlive*>(&object());
	if (entity_alive && !entity_alive->g_Alive())
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CScriptGameObject : cannot check visibility of dead object!");
		return (false);
	}

	CScriptEntity* script_entity = smart_cast<CScriptEntity*>(&object());
	if (!script_entity)
	{
		CActor* actor = smart_cast<CActor*>(&object());
		if (!actor)
		{
			ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
			                                "CScriptGameObject : cannot access class member CheckObjectVisibility!");
			return (false);
		}
		return (actor->memory().visual().visible_now(&tpLuaGameObject->object()));
	}

	return (script_entity->CheckObjectVisibility(&tpLuaGameObject->object()));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CScriptBinderObject* CScriptGameObject::binded_object()
{
	return (object().object());
}

void CScriptGameObject::bind_object(CScriptBinderObject* game_object)
{
	object().set_object(game_object);
}

void CScriptGameObject::set_previous_point(int point_index)
{
	CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
	if (!monster)
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CGameObject : cannot access class member set_previous_point!");
	else
		monster->movement().patrol().set_previous_point(point_index);
}

void CScriptGameObject::set_start_point(int point_index)
{
	CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
	if (!monster)
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CGameObject : cannot access class member set_start_point!");
	else
		monster->movement().patrol().set_start_point(point_index);
}

u32 CScriptGameObject::get_current_patrol_point_index()
{
	CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
	if (!monster)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CGameObject : cannot call [get_current_patrol_point_index()]!");
		return (u32(-1));
	}
	return (monster->movement().patrol().get_current_point_index());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

u16 CScriptGameObject::bone_id(LPCSTR bone_name, bool bHud)
{
	IKinematics* k = nullptr;

	if (bHud)
	{
		CActor* act = smart_cast<CActor*>(&object());
		CHudItem* itm = smart_cast<CHudItem*>(&object());
		if (itm)
			k = itm->HudItemData()->m_model;
		else if (act)
			k = g_player_hud->m_model->dcast_PKinematics();
	} else {
		k = object().Visual()->dcast_PKinematics();
	}

	if (!k) return BI_NONE;

	u16 bone_id = BI_NONE;
	if (xr_strlen(bone_name))
		bone_id = k->LL_BoneID(bone_name);

	return bone_id;
}

Fmatrix CScriptGameObject::bone_transform(u16 bone_id, bool bHud)
{
	//if (bone_id == BI_NONE) return Fvector().set(0, 0, 0);

	IKinematics* k = nullptr;
	Fmatrix* xform = nullptr;

	if (bHud)
	{
		CActor* act = smart_cast<CActor*>(&object());
		CHudItem* itm = smart_cast<CHudItem*>(&object());
		if (itm)
		{
			k = itm->HudItemData()->m_model;
			xform = &itm->HudItemData()->m_item_transform;
		} else if (act)
		{
			k = (bone_id > 20) ? g_player_hud->m_model->dcast_PKinematics() : g_player_hud->m_model_2->dcast_PKinematics();
			xform = (bone_id > 20) ? &g_player_hud->m_transform : &g_player_hud->m_transform_2;
		}
	} else {
		k = object().Visual()->dcast_PKinematics();
		xform = &object().XFORM();
	}

	if (!k) return Fmatrix().identity();

	// demonized: backwards compatibility with scripts, get root bone if bone_id is BI_NONE
	if (bone_id == BI_NONE) {
		if (strstr(Core.Params, "-dbg") && print_bone_warnings) {
			Msg("![bone_position] Incorrect bone_id provided for %s (%d), fallback to root bone", object().cNameSect_str(), object().ID());
			ai().script_engine().print_stack();
		}
		bone_id = k->LL_GetBoneRoot();
	}

	Fmatrix matrix;
	matrix.mul_43(*xform, k->LL_GetTransform(bone_id));
	return matrix;
}

Fvector CScriptGameObject::bone_position(u16 bone_id, bool bHud)
{
	return bone_transform(bone_id, bHud).c;
}

Fvector CScriptGameObject::bone_direction(u16 bone_id, bool bHud)
{
	Fmatrix matrix = bone_transform(bone_id, bHud);
	Fvector res;
	matrix.getHPB(res);
	return res;
}

u16 CScriptGameObject::bone_parent(u16 bone_id, bool bHud)
{
	IKinematics* k = nullptr;

	if (bHud)
	{
		CActor* act = smart_cast<CActor*>(&object());
		CHudItem* itm = smart_cast<CHudItem*>(&object());
		if (itm)
			k = itm->HudItemData()->m_model;
		else if (act)
			k = g_player_hud->m_model->dcast_PKinematics();
	} else {
		k = object().Visual()->dcast_PKinematics();
	}

	if (!k || bone_id == k->LL_GetBoneRoot() || bone_id >= k->LL_BoneCount()) return BI_NONE;

	CBoneData* data = &k->LL_GetData(bone_id);
	u16 ParentID = data->GetParentID();
	return ParentID;
}

LPCSTR CScriptGameObject::bone_name(u16 bone_id, bool bHud)
{
	if (bone_id == BI_NONE) return "";

	IKinematics* k = nullptr;

	if (bHud)
	{
		CActor* act = smart_cast<CActor*>(&object());
		CHudItem* itm = smart_cast<CHudItem*>(&object());
		if (itm)
			k = itm->HudItemData()->m_model;
		else if (act)
			k = g_player_hud->m_model->dcast_PKinematics();
	} else {
		k = object().Visual()->dcast_PKinematics();
	}

	if (!k) return "";

	return (k->LL_BoneName_dbg(bone_id));
}

void CScriptGameObject::set_bone_visible(u16 bone_id, bool bVisibility, bool bRecursive, bool bHud)
{
	if (bone_id == BI_NONE) return;

	IKinematics* k = nullptr;

	if (bHud)
	{
		CActor* act = smart_cast<CActor*>(&object());
		CHudItem* itm = smart_cast<CHudItem*>(&object());
		if (itm)
			k = itm->HudItemData()->m_model;
		else if (act)
			k = (bone_id > 20) ? g_player_hud->m_model->dcast_PKinematics() : g_player_hud->m_model_2->dcast_PKinematics();
	} else {
		k = object().Visual()->dcast_PKinematics();
	}

	if (!k)
		return;

	if (bVisibility != !!k->LL_GetBoneVisible(bone_id))
		k->LL_SetBoneVisible(bone_id, bVisibility, bRecursive);
}

bool CScriptGameObject::is_bone_visible(u16 bone_id, bool bHud)
{
	if (bone_id == BI_NONE) return false;

	IKinematics* k = nullptr;

	if (bHud)
	{
		CActor* act = smart_cast<CActor*>(&object());
		CHudItem* itm = smart_cast<CHudItem*>(&object());
		if (itm)
			k = itm->HudItemData()->m_model;
		else if (act)
			k = (bone_id > 20) ? g_player_hud->m_model->dcast_PKinematics() : g_player_hud->m_model_2->dcast_PKinematics();
	} else {
		k = object().Visual()->dcast_PKinematics();
	}

	if (!k) return false;

	return !!k->LL_GetBoneVisible(bone_id);
}

// demonized: list all bones
::luabind::object CScriptGameObject::list_bones(bool bHud)
{
	::luabind::object result = ::luabind::newtable(ai().script_engine().lua());
	IKinematics* k = nullptr;

	if (bHud)
	{
		CActor* act = smart_cast<CActor*>(&object());
		CHudItem* itm = smart_cast<CHudItem*>(&object());
		if (itm)
			k = itm->HudItemData()->m_model;
		else if (act)
			k = g_player_hud->m_model->dcast_PKinematics();
	} else {
		k = object().Visual()->dcast_PKinematics();
	}

	if (!k) return result;

	auto bones = k->list_bones();
	for (const auto& bone : bones)
		result[bone.first] = bone.second.c_str();

	return result;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

u32 CScriptGameObject::GetAmmoElapsed()
{
	const CWeapon* weapon = smart_cast<const CWeapon*>(&object());
	if (!weapon)
		return (0);
	return (weapon->GetAmmoElapsed());
}

void CScriptGameObject::SetAmmoElapsed(int ammo_elapsed)
{
	CWeapon* weapon = smart_cast<CWeapon*>(&object());
	if (!weapon) return;
	weapon->SetAmmoElapsed(ammo_elapsed);
}

//Alundaio
int CScriptGameObject::GetAmmoCount(u8 type)
{
	CWeapon* weapon = smart_cast<CWeapon*>(&object());
	if (!weapon) return 0;

	if (type < weapon->m_ammoTypes.size())
		return weapon->GetAmmoCount_forType(weapon->m_ammoTypes[type]);

	return 0;
}

void CScriptGameObject::SetAmmoType(u8 type)
{
	CWeapon* weapon = smart_cast<CWeapon*>(&object());
	if (!weapon) return;

	weapon->SetAmmoType(type);
}

u8 CScriptGameObject::GetAmmoType()
{
	CWeapon* weapon = smart_cast<CWeapon*>(&object());
	if (!weapon) return 255;

	return weapon->GetAmmoType();
}

void CScriptGameObject::SetMainWeaponType(u32 type)
{
	CWeapon* weapon = smart_cast<CWeapon*>(&object());
	if (!weapon) return;

	weapon->set_ef_main_weapon_type(type);
}

void CScriptGameObject::SetWeaponType(u32 type)
{
	CWeapon* weapon = smart_cast<CWeapon*>(&object());
	if (!weapon) return;

	weapon->set_ef_weapon_type(type);
}

u32 CScriptGameObject::GetMainWeaponType()
{
	CWeapon* weapon = smart_cast<CWeapon*>(&object());
	if (!weapon) return 255;

	return weapon->ef_main_weapon_type();
}

u32 CScriptGameObject::GetWeaponType()
{
	CWeapon* weapon = smart_cast<CWeapon*>(&object());
	if (!weapon) return 255;

	return weapon->ef_weapon_type();
}

bool CScriptGameObject::HasAmmoType(u8 type)
{
	CWeapon* weapon = smart_cast<CWeapon*>(&object());
	if (!weapon) return false;

	return type < weapon->m_ammoTypes.size();
}

u8 CScriptGameObject::GetWeaponSubstate()
{
	CWeapon* weapon = smart_cast<CWeapon*>(&object());
	if (!weapon) return 255;

	return weapon->m_sub_state;
}

//-Alundaio

u32 CScriptGameObject::GetSuitableAmmoTotal() const
{
	const CWeapon* weapon = smart_cast<const CWeapon*>(&object());
	if (!weapon)
		return (0);
	return (weapon->GetSuitableAmmoTotal(true));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void CScriptGameObject::SetQueueSize(u32 queue_size)
{
	CWeaponMagazined* weapon = smart_cast<CWeaponMagazined*>(&object());
	if (!weapon)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CWeaponMagazined : cannot access class member SetQueueSize!");
		return;
	}
	weapon->SetQueueSize(queue_size);
}

////////////////////////////////////////////////////////////////////////////
//// Inventory Owner
////////////////////////////////////////////////////////////////////////////

u32 CScriptGameObject::Cost() const
{
	CInventoryItem* inventory_item = smart_cast<CInventoryItem*>(&object());
	if (!inventory_item)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CSciptEntity : cannot access class member Cost!");
		return (false);
	}
	return (inventory_item->Cost());
}

float CScriptGameObject::GetCondition() const
{
	CInventoryItem* inventory_item = smart_cast<CInventoryItem*>(&object());
	if (!inventory_item)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CSciptEntity : cannot access class member GetCondition!");
		return (false);
	}
	return (inventory_item->GetCondition());
}

void CScriptGameObject::SetCondition(float val)
{
	CInventoryItem* inventory_item = smart_cast<CInventoryItem*>(&object());
	if (!inventory_item)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CSciptEntity : cannot access class member SetCondition!");
		return;
	}
	val -= inventory_item->GetCondition();
	inventory_item->ChangeCondition(val);
}

float CScriptGameObject::GetPowerCritical() const
{
	CInventoryItem* inventory_item = smart_cast<CInventoryItem*>(&object());
	if (!inventory_item)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
			"CSciptEntity : cannot access class member GetPowerCritical!");
		return 0.f;
	}
	return (inventory_item->GetLowestBatteryCharge());
}

float CScriptGameObject::GetPsyFactor() const
{
	CPda* pda = smart_cast<CPda*>(&object());
	if (!pda)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
			"CSciptEntity : cannot access class member GetPsyFactor!");
		return 0.f;
	}
	return (pda->m_psy_factor);
}

void CScriptGameObject::SetPsyFactor(float val)
{
	CPda* pda = smart_cast<CPda*>(&object());
	if (!pda)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
			"CSciptEntity : cannot access class member SetPsyFactor!");
		return;
	}
	pda->m_psy_factor = val;
}

void CScriptGameObject::eat(CScriptGameObject* item)
{
	if (!item)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CSciptEntity : cannot access class member eat!");
		return;
	}

	CInventoryItem* inventory_item = smart_cast<CInventoryItem*>(&item->object());
	if (!inventory_item)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CSciptEntity : cannot access class member eat!");
		return;
	}

	CInventoryOwner* inventory_owner = smart_cast<CInventoryOwner*>(&object());
	if (!inventory_owner)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CSciptEntity : cannot access class member eat!");
		return;
	}

	inventory_owner->inventory().Eat(inventory_item);
}

bool CScriptGameObject::inside(const Fvector& position, float epsilon) const
{
	CSpaceRestrictor* space_restrictor = smart_cast<CSpaceRestrictor*>(&object());
	if (!space_restrictor)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CSpaceRestrictor : cannot access class member inside!");
		return (false);
	}
	Fsphere sphere;
	sphere.P = position;
	sphere.R = epsilon;
	return (space_restrictor->inside(sphere));
}

bool CScriptGameObject::inside(const Fvector& position) const
{
	return (inside(position, EPS_L));
}

void CScriptGameObject::set_patrol_extrapolate_callback(const ::luabind::functor<bool>& functor)
{
	CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
	if (!monster)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CCustomMonster : cannot access class member set_patrol_extrapolate_callback!");
		return;
	}
	monster->movement().patrol().extrapolate_callback().set(functor);
}

void CScriptGameObject::set_patrol_extrapolate_callback(const ::luabind::functor<bool>& functor,
                                                        const ::luabind::object& object)
{
	CCustomMonster* monster = smart_cast<CCustomMonster*>(&this->object());
	if (!monster)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CCustomMonster : cannot access class member set_patrol_extrapolate_callback!");
		return;
	}
	monster->movement().patrol().extrapolate_callback().set(functor, object);
}

void CScriptGameObject::set_patrol_extrapolate_callback()
{
	CCustomMonster* monster = smart_cast<CCustomMonster*>(&this->object());
	if (!monster)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CCustomMonster : cannot access class member set_patrol_extrapolate_callback!");
		return;
	}
	monster->movement().patrol().extrapolate_callback().clear();
}

void CScriptGameObject::extrapolate_length(float extrapolate_length)
{
	CCustomMonster* monster = smart_cast<CCustomMonster*>(&this->object());
	if (!monster)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CCustomMonster : cannot access class member extrapolate_length!");
		return;
	}
	monster->movement().detail().extrapolate_length(extrapolate_length);
}

float CScriptGameObject::extrapolate_length() const
{
	CCustomMonster* monster = smart_cast<CCustomMonster*>(&this->object());
	if (!monster)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CCustomMonster : cannot access class member extrapolate_length!");
		return (0.f);
	}
	return (monster->movement().detail().extrapolate_length());
}

void CScriptGameObject::set_fov(float new_fov)
{
	CCustomMonster* monster = smart_cast<CCustomMonster*>(&this->object());
	if (!monster)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CCustomMonster : cannot access class member set_fov!");
		return;
	}
	monster->set_fov(new_fov);
}

void CScriptGameObject::set_range(float new_range)
{
	CCustomMonster* monster = smart_cast<CCustomMonster*>(&this->object());
	if (!monster)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CCustomMonster : cannot access class member set_range!");
		return;
	}
	monster->set_range(new_range);
}

u32 CScriptGameObject::vertex_in_direction(u32 level_vertex_id, Fvector direction, float max_distance) const
{
	CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
	if (!monster)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CCustomMonster : cannot access class member vertex_in_direction!");
		return (u32(-1));
	}

	if (!monster->movement().restrictions().accessible(level_vertex_id))
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CCustomMonster::vertex_in_direction - start vertex id is not accessible!");
		return (u32(-1));
	}

	direction.normalize_safe();
	direction.mul(max_distance);
	Fvector start_position = ai().level_graph().vertex_position(level_vertex_id);
	Fvector finish_position = Fvector(start_position).add(direction);
	u32 result = u32(-1);
	monster->movement().restrictions().add_border(level_vertex_id, max_distance);
	ai().level_graph().farthest_vertex_in_direction(level_vertex_id, start_position, finish_position, result, 0, true);
	monster->movement().restrictions().remove_border();
	return (ai().level_graph().valid_vertex_id(result) ? result : level_vertex_id);
}

bool CScriptGameObject::invulnerable() const
{
	CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
	if (!monster)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CCustomMonster : cannot access class member invulnerable!");
		return (false);
	}

	return (monster->invulnerable());
}

void CScriptGameObject::invulnerable(bool invulnerable)
{
	CCustomMonster* monster = smart_cast<CCustomMonster*>(&object());
	if (!monster)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "CCustomMonster : cannot access class member invulnerable!");
		return;
	}

	monster->invulnerable(invulnerable);
}

LPCSTR CScriptGameObject::get_smart_cover_description() const
{
	smart_cover::object* smart_cover_object = smart_cast<smart_cover::object*>(&object());
	if (!smart_cover_object)
	{
		ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError,
		                                "smart_cover::object : cannot access class member get_smart_cover_description!");
		return (0);
	}
	return smart_cover_object->cover().description()->table_id().c_str();
}

void CScriptGameObject::PhantomSetEnemy(CScriptGameObject* enemy)
{
	CPhantom* phant = smart_cast<CPhantom*>(&object());
	if (!phant)
		return;

	phant->SetEnemy(&enemy->object());
}

//Allows to force use an object if passed obj is the actor
bool CScriptGameObject::Use(CScriptGameObject* obj)
{
	bool ret = object().use(&obj->object());

	CActor* actor = smart_cast<CActor*>(&obj->object());
	if (!actor)
		return ret;

	CInventoryOwner* pActorInv = smart_cast<CInventoryOwner*>(actor);
	if (!pActorInv)
		return ret;

	CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());
	if (!pGameSP)
		return ret;
	
	CInventoryBox* pBox = smart_cast<CInventoryBox*>(&object());
	if (pBox)
	{
		pGameSP->StartCarBody(pActorInv, pBox);
		return true;
	}

	CInventoryOwner* pOtherOwner = smart_cast<CInventoryOwner*>(&object());
	if (pOtherOwner)
	{
		pGameSP->StartCarBody(pActorInv, pOtherOwner);
		return true;
	}

	return false;
}

void CScriptGameObject::StartTrade(CScriptGameObject* obj)
{
	CActor* actor = smart_cast<CActor*>(&obj->object());
	if (!actor)
		return;

	CInventoryOwner* pActorInv = smart_cast<CInventoryOwner*>(actor);
	if (!pActorInv)
		return;

	CInventoryOwner* pOtherOwner = smart_cast<CInventoryOwner*>(&object());
	if (!pOtherOwner)
		return;

	CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());
	if (pGameSP)
		pGameSP->StartTrade(pActorInv, pOtherOwner);
}

void CScriptGameObject::StartUpgrade(CScriptGameObject* obj)
{
	CActor* actor = smart_cast<CActor*>(&obj->object());
	if (!actor)
		return;

	CInventoryOwner* pActorInv = smart_cast<CInventoryOwner*>(actor);
	if (!pActorInv)
		return;

	CInventoryOwner* pOtherOwner = smart_cast<CInventoryOwner*>(&object());
	if (!pOtherOwner)
		return;
	
	CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(CurrentGameUI());
	if (pGameSP)
		pGameSP->StartUpgrade(pActorInv, pOtherOwner);
}

script_attachment* CScriptGameObject::AddAttachment(LPCSTR name, LPCSTR model_name)
{
	script_attachment* att = xr_new<script_attachment>(name, model_name);
	R_ASSERT(att);
	att->SetParent(&object());
	return att;
}

script_attachment* CScriptGameObject::GetAttachment(LPCSTR name)
{
	return object().get_attachment(name);
}

void CScriptGameObject::RemoveAttachment(LPCSTR name)
{
	object().remove_attachment(name);
}

void CScriptGameObject::RemoveAttachment(script_attachment* child)
{
	object().remove_attachment(child);
}

void CScriptGameObject::IterateAttachments(::luabind::functor<bool> functor)
{
	object().iterate_attachments(functor);
}

CGameObject& CScriptGameObject::object() const
{
#ifdef DEBUG
    __try {
        if (m_game_object && m_game_object->lua_game_object() == this)
            return	(*m_game_object);
    } __except (EXCEPTION_EXECUTE_HANDLER) {}

    ai().script_engine().script_log(eLuaMessageTypeError, "you are trying to use a destroyed object [%x]", m_game_object);
    THROW2(m_game_object && m_game_object->lua_game_object() == this, "Probably, you are trying to use a destroyed object!");
#endif // #ifdef DEBUG
	static CGameObject* m_game_object_dummy = NULL;
	if (!m_game_object || m_game_object->lua_game_object() != this)
		return (*m_game_object_dummy);

	return (*m_game_object);
}

//////////////////////////////////////////////////////////////////////////
// Shader / Textures Magic
::luabind::object CScriptGameObject::GetShaders(bool bHud)
{
	IKinematics* k = nullptr;

	if (bHud)
	{
		CActor* act = smart_cast<CActor*>(&object());
		CHudItem* itm = smart_cast<CHudItem*>(&object());
		if (itm)
			k = itm->HudItemData()->m_model;
		else if (act)
			k = g_player_hud->m_model->dcast_PKinematics();
	}

	if (!k)
		k = object().Visual()->dcast_PKinematics();

	::luabind::object table = ::luabind::newtable(ai().script_engine().lua());

	if (!k)
	{
		table["error"] = true;
		return table;
	}

	IRenderVisual* vis = k->dcast_RenderVisual();
	xr_vector<IRenderVisual*>* children = vis->get_children();

	if (!children)
	{
		::luabind::object subtable = ::luabind::newtable(ai().script_engine().lua());
		subtable["shader"] = vis->getDebugShader();
		subtable["texture"] = vis->getDebugTexture();
		table[1] = subtable;
		return table;
	}

	int i = 1;

	for (auto* child : *children)
	{
		::luabind::object subtable = ::luabind::newtable(ai().script_engine().lua());
		subtable["shader"] = child->getDebugShader();
		subtable["texture"] = child->getDebugTexture();
		table[i] = subtable;
		++i;
	}

	return table;
}

::luabind::object CScriptGameObject::GetDefaultShaders(bool bHud)
{
	IKinematics* k = nullptr;

	if (bHud)
	{
		CActor* act = smart_cast<CActor*>(&object());
		CHudItem* itm = smart_cast<CHudItem*>(&object());
		if (itm)
			k = itm->HudItemData()->m_model;
		else if (act)
			k = g_player_hud->m_model->dcast_PKinematics();
	}

	if (!k)
		k = object().Visual()->dcast_PKinematics();

	::luabind::object table = ::luabind::newtable(ai().script_engine().lua());

	if (!k)
	{
		table["error"] = true;
		return table;
	}

	IRenderVisual* vis = k->dcast_RenderVisual();
	xr_vector<IRenderVisual*>* children = vis->get_children();

	if (!children)
	{
		::luabind::object subtable = ::luabind::newtable(ai().script_engine().lua());
		subtable["shader"] = vis->getDebugShaderDef();
		subtable["texture"] = vis->getDebugTextureDef();
		table[1] = subtable;
		return table;
	}

	int i = 1;

	for (auto* child : *children)
	{
		::luabind::object subtable = ::luabind::newtable(ai().script_engine().lua());
		subtable["shader"] = child->getDebugShaderDef();
		subtable["texture"] = child->getDebugTextureDef();
		table[i] = subtable;
		++i;
	}

	return table;
}

void set_shader_tex(IRenderVisual* vis, int id, LPCSTR shader, LPCSTR texture)
{
	xr_vector<IRenderVisual*>* children = vis->get_children();

	if (!children)
	{
		vis->SetShaderTexture(shader, texture);
		return;
	}

	if (id == -1)
	{
		for (auto* child : *children)
		{
			child->SetShaderTexture(shader, texture);
		}
		return;
	}

	id--;

	if (id >= 0 && children->size() > id)
		children->at(id)->SetShaderTexture(shader, texture);
}

void reset_shader_tex(IRenderVisual* vis, int id)
{
	xr_vector<IRenderVisual*>* children = vis->get_children();

	if (!children)
	{
		vis->ResetShaderTexture();
		return;
	}

	if (id == -1)
	{
		for (auto* child : *children)
		{
			child->ResetShaderTexture();
		}
		return;
	}

	id--;

	if (id >= 0 && children->size() > id)
		children->at(id)->ResetShaderTexture();
}

void CScriptGameObject::SetShaderTexture(int id, LPCSTR shader, LPCSTR texture, bool bHud)
{
	IKinematics* k = nullptr;

	if (bHud)
	{
		CActor* act = smart_cast<CActor*>(&object());
		CHudItem* itm = smart_cast<CHudItem*>(&object());
		if (itm)
			k = itm->HudItemData()->m_model;
		else if (act)
		{
			set_shader_tex(g_player_hud->m_model->dcast_RenderVisual(), id, shader, texture);
			set_shader_tex(g_player_hud->m_model_2->dcast_RenderVisual(), id, shader, texture);
			return;
		}
	}

	if (!k)
		k = object().Visual()->dcast_PKinematics();

	if (!k) return;

	set_shader_tex(k->dcast_RenderVisual(), id, shader, texture);
}

void CScriptGameObject::ResetShaderTexture(int id, bool bHud)
{
	IKinematics* k = nullptr;

	if (bHud)
	{
		CActor* act = smart_cast<CActor*>(&object());
		CHudItem* itm = smart_cast<CHudItem*>(&object());
		if (itm)
			k = itm->HudItemData()->m_model;
		else if (act)
		{
			reset_shader_tex(g_player_hud->m_model->dcast_RenderVisual(), id);
			reset_shader_tex(g_player_hud->m_model_2->dcast_RenderVisual(), id);
			return;
		}
	}

	if (!k)
		k = object().Visual()->dcast_PKinematics();

	if (!k) return;

	reset_shader_tex(k->dcast_RenderVisual(), id);
}
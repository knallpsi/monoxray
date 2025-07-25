////////////////////////////////////////////////////////////////////////////
//	Module 		: script_game_object.h
//	Created 	: 25.09.2003
//  Modified 	: 29.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script game object class
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_space_forward.h"
#include "script_bind_macroses.h"
#include "script_export_space.h"
#include "xr_time.h"
#include "character_info_defs.h"
#include "game_graph_space.h"
#include "game_location_selector.h"
#include "Actor.h"
#include "Car.h"
#include "helicopter.h"
#include "InventoryOwner.h"
#include "InventoryBox.h"
#include "CustomZone.h"
#include "TorridZone.h"
#include "MosquitoBald.h"
#include "ZoneCampfire.h"
#include "CustomOutfit.h"
#include "ActorHelmet.h"
#include "Artefact.h"
#include "Weapon.h"
#include "WeaponAmmo.h"
#include "WeaponMagazined.h"
#include "WeaponMagazinedWGrenade.h"
#include "eatable_item.h"
#include "FoodItem.h"
#include "medkit.h"
#include "antirad.h"
#include "BottleItem.h"

enum EPdaMsg;
enum ESoundTypes;
enum ETaskState;

namespace ALife { enum ERelationType; }
namespace ScriptEntity { enum EActionType; }
namespace MovementManager { enum EPathType; }
namespace DetailPathManager { enum EDetailPathType; }
namespace SightManager { enum ESightType; }
namespace smart_cover { class object; }
namespace doors { class door; }

class NET_Packet;
class CGameTask;

namespace PatrolPathManager {
	enum EPatrolStartType;
	enum EPatrolRouteType;
};

namespace MemorySpace {
	struct CMemoryInfo;
	struct CVisibleObject;
	struct CSoundObject;
	struct CHitObject;
	struct CNotYetVisibleObject;
};

namespace MonsterSpace {
	enum EBodyState;
	enum EMovementType;
	enum EMovementDirection;
	enum EDirectionType;
	enum EPathState;
	enum EObjectAction;
	enum EMentalState;
	enum EScriptMonsterMoveAction;
	enum EScriptMonsterSpeedParam;
	enum EScriptMonsterAnimAction;
	enum EScriptMonsterGlobalAction;
	enum EScriptSoundAnim;
	enum EMonsterSounds;
	enum EMonsterHeadAnimType;
	struct SBoneRotation;
};

namespace GameObject {
	enum ECallbackType;
};

class CGameObject;
class CScriptHit;
class CScriptEntityAction;
class CScriptTask;
class CScriptSoundInfo;
class CScriptMonsterHitInfo;
class CScriptBinderObject;
class CCoverPoint;
class CScriptIniFile;
class cphysics_shell_scripted;
class CHelicopter;
class CHangingLamp;
class CHolderCustom;
struct ScriptCallbackInfo;
struct STasks;
class CCar;
class CDangerObject;
class CScriptGameObject;
class CZoneCampfire;
class CPhysicObject;
class CArtefact;
class script_attachment;

#ifdef DEBUG
    template <typename _object_type>
    class CActionBase;

    template <typename _object_type>
    class CPropertyEvaluator;

    template <
        typename _object_type,
        bool	 _reverse_search,
        typename _world_operator,
        typename _condition_evaluator,
        typename _world_operator_ptr,
        typename _condition_evaluator_ptr
    >
    class CActionPlanner;

    typedef CActionPlanner<
        CScriptGameObject,
        false,
        CActionBase<CScriptGameObject>,
        CPropertyEvaluator<CScriptGameObject>,
        CActionBase<CScriptGameObject>*,
        CPropertyEvaluator<CScriptGameObject>*
    >								script_planner;
#endif // DEBUG

class CScriptGameObject;

namespace SightManager
{
	enum ESightType;
}

struct CSightParams
{
	SightManager::ESightType m_sight_type;
	CScriptGameObject* m_object;
	Fvector m_vector;
};

namespace luabind
{
	template <typename return_type>
	class functor;

	class object;
} // namespace luabind

class CScriptGameObject
{
	mutable CGameObject* m_game_object;
	CScriptGameObject(CScriptGameObject const& game_object);

public:

	CScriptGameObject(CGameObject* tpGameObject);
	virtual ~CScriptGameObject();
	operator CObject*();

	CGameObject& object() const;
	CScriptGameObject* Parent() const;
	void Hit(CScriptHit* tLuaHit);
	int clsid() const;
	void play_cycle(LPCSTR anim, bool mix_in);
	void play_cycle(LPCSTR anim);
	Fvector Center(bool bHud = false);
	Fmatrix Xform(bool bHud = false);
	Fbox bounding_box(bool bHud);
	_DECLARE_FUNCTION10(Position, Fvector);
	_DECLARE_FUNCTION10(Direction, Fvector);
	_DECLARE_FUNCTION10(Mass, float);
	_DECLARE_FUNCTION10(ID, u16);
	_DECLARE_FUNCTION10(getVisible, BOOL);
	_DECLARE_FUNCTION10(getEnabled, BOOL);
	_DECLARE_FUNCTION10(story_id, ALife::_STORY_ID);

	LPCSTR Name() const;
	shared_str cName() const;
	LPCSTR Section() const;
	// CInventoryItem
	u32 Cost() const;
	float GetCondition() const;
	void SetCondition(float val);
	float GetPowerCritical() const;
	float GetPsyFactor() const;
	void SetPsyFactor(float val);

	// CEntity
	_DECLARE_FUNCTION10(DeathTime, u32);
	_DECLARE_FUNCTION10(MaxHealth, float);
	_DECLARE_FUNCTION10(Accuracy, float);
	_DECLARE_FUNCTION10(Team, int);
	_DECLARE_FUNCTION10(Squad, int);
	_DECLARE_FUNCTION10(Group, int);

	void Kill(CScriptGameObject* who, bool bypass_actor_check = false /*AVO: added for actor before death callback*/);

	// CEntityAlive
	_DECLARE_FUNCTION10(GetFOV, float);
	_DECLARE_FUNCTION10(GetRange, float);

	_DECLARE_FUNCTION10(GetHealth, float);
	_DECLARE_FUNCTION11(SetHealth, void, float);
	_DECLARE_FUNCTION11(ChangeHealth, void, float);

	_DECLARE_FUNCTION10(GetPsyHealth, float);
	_DECLARE_FUNCTION11(SetPsyHealth, void, float);
	_DECLARE_FUNCTION11(ChangePsyHealth, void, float);

	_DECLARE_FUNCTION10(GetPower, float);
	_DECLARE_FUNCTION11(SetPower, void, float);
	_DECLARE_FUNCTION11(ChangePower, void, float);

	_DECLARE_FUNCTION10(GetSatiety, float);
	_DECLARE_FUNCTION11(SetSatiety, void, float);
	_DECLARE_FUNCTION11(ChangeSatiety, void, float);

	_DECLARE_FUNCTION10(GetRadiation, float);
	_DECLARE_FUNCTION11(SetRadiation, void, float);
	_DECLARE_FUNCTION11(ChangeRadiation, void, float);

	_DECLARE_FUNCTION10(GetMorale, float);
	_DECLARE_FUNCTION11(SetMorale, void, float);
	_DECLARE_FUNCTION11(ChangeMorale, void, float);

	_DECLARE_FUNCTION10(GetBleeding, float);
	_DECLARE_FUNCTION11(ChangeBleeding, void, float);

	_DECLARE_FUNCTION11(ChangeCircumspection, void, float);

	void set_fov(float new_fov);
	void set_range(float new_range);
	bool Alive() const;
	ALife::ERelationType GetRelationType(CScriptGameObject* who);

	// CScriptEntity

	_DECLARE_FUNCTION12(SetScriptControl, void, bool, LPCSTR);
	_DECLARE_FUNCTION10(GetScriptControl, bool);
	_DECLARE_FUNCTION10(GetScriptControlName, LPCSTR);
	_DECLARE_FUNCTION10(GetEnemyStrength, int);
	_DECLARE_FUNCTION10(can_script_capture, bool);


	CScriptEntityAction* GetCurrentAction() const;
	void AddAction(const CScriptEntityAction* tpEntityAction, bool bHighPriority = false);
	void ResetActionQueue();
	// Actor only
	void SetActorPosition(Fvector pos, bool bskip_collision_correct = false, bool bkeep_speed = false);	// momopate: allow movespeed to be kept if bskip_collision_correct == true
	void SetActorDirection(float dir);
	void SetActorDirection(float dir, float pitch);
	void SetActorDirection(float dir, float pitch, float roll);
	void SetActorDirection(const Fvector& dir);
	void SetNpcPosition(Fvector pos);
	void DisableHitMarks(bool disable);
	bool DisableHitMarks() const;
	Fvector GetMovementSpeed() const;
	void SetMovementSpeed(Fvector vel);		// momopate: db.actor:set_movement_speed(vector vel)

	// CCustomMonster
	bool CheckObjectVisibility(const CScriptGameObject* tpLuaGameObject);
	bool CheckTypeVisibility(const char* section_name);
	LPCSTR WhoHitName();
	LPCSTR WhoHitSectionName();

	void ChangeTeam(u8 team, u8 squad, u8 group);
	void SetVisualMemoryEnabled(bool enabled);

	// CAI_Stalker
	CScriptGameObject* GetCurrentWeapon() const;
	CScriptGameObject* GetFood() const;
	CScriptGameObject* GetMedikit() const;
	void SetPlayShHdRldSounds(bool val);

	void set_force_anti_aim(bool force);
	bool get_force_anti_aim();

	// Burer
	void burer_set_force_gravi_attack(bool force);
	bool burer_get_force_gravi_attack();

	// Poltergeist
	void poltergeist_set_actor_ignore(bool ignore);
	bool poltergeist_get_actor_ignore();

	// CAI_Bloodsucker
	void force_visibility_state(int state);
	int get_visibility_state();

	// CBaseMonster
	void set_override_animation(pcstr anim_name);
	void set_override_animation(u32 AnimType, u32 AnimIndex);
	void clear_override_animation();

	void force_stand_sleep_animation(u32 index);
	void release_stand_sleep_animation();

	void set_invisible(bool val);
	bool get_invisible();
	void set_manual_invisibility(bool val);
	void set_alien_control(bool val);
	void set_enemy(CScriptGameObject* e);
	void set_vis_state(float value);
	void off_collision(bool val);
	void bloodsucker_drag_jump(CScriptGameObject* e, LPCSTR e_str, const Fvector& position, float factor);

	// Zombie
	bool fake_death_fall_down();
	void fake_death_stand_up();

	// CBaseMonster
	void skip_transfer_enemy(bool val);
	void set_home(LPCSTR name, float r_min, float r_max, bool aggressive, float r_mid);
	void set_home(u32 lv_ID, float r_min, float r_max, bool aggressive, float r_mid);
	void remove_home();
	void berserk();
	void set_custom_panic_threshold(float value);
	void set_default_panic_threshold();

	// CAI_Trader
	void set_trader_global_anim(LPCSTR anim);
	void set_trader_head_anim(LPCSTR anim);
	void set_trader_sound(LPCSTR sound, LPCSTR anim);
	void external_sound_start(LPCSTR sound);
	void external_sound_stop();


	template <typename T>
	IC T* action_planner();

	// CProjector
	Fvector GetCurrentDirection();

	bool IsInvBoxEmpty();
	bool inv_box_closed(bool status, LPCSTR reason);
	bool inv_box_closed_status();
	bool inv_box_can_take(bool status);
	bool inv_box_can_take_status();

	//передача порции информации InventoryOwner
	bool GiveInfoPortion(LPCSTR info_id);
	bool DisableInfoPortion(LPCSTR info_id);
	void GiveGameNews(LPCSTR caption, LPCSTR news, LPCSTR texture_name, int delay, int show_time);
	void GiveGameNews(LPCSTR caption, LPCSTR news, LPCSTR texture_name, int delay, int show_time, int type);

	void AddIconedTalkMessage_old(LPCSTR text, LPCSTR texture_name, LPCSTR templ_name)
	{
	};
	void AddIconedTalkMessage(LPCSTR caption, LPCSTR text, LPCSTR texture_name, LPCSTR templ_name);
	//предикаты наличия/отсутствия порции информации у персонажа
	bool HasInfo(LPCSTR info_id);
	bool DontHasInfo(LPCSTR info_id);
	//работа с заданиями
	ETaskState GetGameTaskState(LPCSTR task_id);
	void SetGameTaskState(ETaskState state, LPCSTR task_id);
	void GiveTaskToActor(CGameTask* t, u32 dt, bool bCheckExisting, u32 t_timer);
	void SetActiveTask(CGameTask* t);
	bool IsActiveTask(CGameTask* t);
	CGameTask* GetTask(LPCSTR id, bool only_inprocess);


	bool IsTalking();
	void StopTalk();
	void EnableTalk();
	void DisableTalk();
	bool IsTalkEnabled();

	void EnableTrade();
	void DisableTrade();
	bool IsTradeEnabled();

	void EnableInvUpgrade();
	void DisableInvUpgrade();
	bool IsInvUpgradeEnabled();


	void ActorLookAtPoint(Fvector point);
	void ActorStopLookAtPoint();
	void IterateInventory(::luabind::functor<bool> functor, ::luabind::object object);
	void IterateRuck(::luabind::functor<bool> functor, ::luabind::object object);
	void IterateBelt(::luabind::functor<bool> functor, ::luabind::object object);
	void IterateInventoryBox(::luabind::functor<bool> functor, ::luabind::object object);
	void MarkItemDropped(CScriptGameObject* item, bool flag);
	bool MarkedDropped(CScriptGameObject* item);
	void UnloadMagazine(bool bKeepAmmo);
	void ForceUnloadMagazine(bool bKeepAmmo);

	void SetCanBeHarmed(bool state);
	bool CanBeHarmed();

	void DropItem(CScriptGameObject* pItem);
	void DropItemAndTeleport(CScriptGameObject* pItem, Fvector position);
	void ForEachInventoryItems(const ::luabind::functor<bool>& functor);
	void TransferItem(CScriptGameObject* pItem, CScriptGameObject* pForWho);
	void TakeItem(CScriptGameObject* pItem);
	void TransferMoney(int money, CScriptGameObject* pForWho);
	void GiveMoney(int money);
	u32 Money();
	void MakeItemActive(CScriptGameObject* pItem);
	void MoveItemToRuck(CScriptGameObject* pItem);
	void MoveItemToSlot(CScriptGameObject* pItem, u16 slot_id);
	void MoveItemToBelt(CScriptGameObject* pItem);
	void ItemAllowTrade(CScriptGameObject* pItem);
	void ItemDenyTrade(CScriptGameObject* pItem);

	void SetRelation(ALife::ERelationType relation, CScriptGameObject* pWhoToSet);

	float GetSympathy();
	void SetSympathy(float sympathy);

	int GetCommunityGoodwill_obj(LPCSTR community);
	void SetCommunityGoodwill_obj(LPCSTR community, int goodwill);

	int GetAttitude(CScriptGameObject* pToWho);

	int GetGoodwill(CScriptGameObject* pToWho);
	void SetGoodwill(int goodwill, CScriptGameObject* pWhoToSet);
	void ForceSetGoodwill(int goodwill, CScriptGameObject* pWhoToSet);
	void ChangeGoodwill(int delta_goodwill, CScriptGameObject* pWhoToSet);


	void SetStartDialog(LPCSTR dialog_id);
	void GetStartDialog();
	void RestoreDefaultStartDialog();

	void SwitchToTrade();
	void SwitchToUpgrade();
	void SwitchToTalk();
	void RunTalkDialog(CScriptGameObject* pToWho, bool disable_break);
	void AllowBreakTalkDialog(bool disable_break);

	void HideWeapon();
	void RestoreWeapon();
	void AllowSprint(bool b);

	bool Weapon_IsGrenadeLauncherAttached();
	bool Weapon_IsScopeAttached();
	bool Weapon_IsSilencerAttached();

	int Weapon_GrenadeLauncher_Status();
	int Weapon_Scope_Status();
	int Weapon_Silencer_Status();

	LPCSTR ProfileName();
	LPCSTR CharacterName();
	LPCSTR CharacterIcon();
	LPCSTR CharacterCommunity();
	::luabind::object CharacterDialogs();
	int CharacterRank();
	int CharacterReputation();


	void SetCharacterRank(int);
	void ChangeCharacterRank(int);
	void ChangeCharacterReputation(int);
	void SetCharacterReputation(int);
	void SetCharacterCommunity(LPCSTR, int, int);


	u32 GetInventoryObjectCount() const;

	CScriptGameObject* GetActiveItem();

	CScriptGameObject* GetObjectByName(LPCSTR caObjectName) const;
	CScriptGameObject* GetObjectByIndex(int iIndex) const;
	CScriptGameObject * GetObjectById(u16 id) const;


	// Callbacks			
	void SetCallback(GameObject::ECallbackType type, const ::luabind::functor<void>& functor);
	void SetCallback(GameObject::ECallbackType type, const ::luabind::functor<void>& functor,
	                 const ::luabind::object& object);
	void SetCallback(GameObject::ECallbackType type);

	void set_patrol_extrapolate_callback(const ::luabind::functor<bool>& functor);
	void set_patrol_extrapolate_callback(const ::luabind::functor<bool>& functor, const ::luabind::object& object);
	void set_patrol_extrapolate_callback();

	void set_enemy_callback(const ::luabind::functor<bool>& functor);
	void set_enemy_callback(const ::luabind::functor<bool>& functor, const ::luabind::object& object);
	void set_enemy_callback();

	//////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////use calback///////////////////////////////////////////////
	void SetTipText(LPCSTR tip_text);
	void SetTipTextDefault();
	void SetNonscriptUsable(bool nonscript_usable);
	///////////////////////////////////////////////////////////////////////////////////////////
	void set_fastcall(const ::luabind::functor<bool>& functor, const ::luabind::object& object);
	void set_const_force(const Fvector& dir, float value, u32 time_interval);
	//////////////////////////////////////////////////////////////////////////

	LPCSTR GetPatrolPathName();
	u32 GetAmmoElapsed();
	void SetAmmoElapsed(int ammo_elapsed);
	u32 GetSuitableAmmoTotal() const;
	void SetQueueSize(u32 queue_size);
	CScriptGameObject* GetBestEnemy();
	const CDangerObject* GetBestDanger();
	CScriptGameObject* GetBestItem();

	_DECLARE_FUNCTION10(GetActionCount, u32);

	const CScriptEntityAction* GetActionByIndex(u32 action_index = 0);

	//////////////////////////////////////////////////////////////////////////
	// Inventory Owner
	//////////////////////////////////////////////////////////////////////////

	//////////////////////////////////////////////////////////////////////////
	Flags32 get_actor_relation_flags() const;
	void set_actor_relation_flags(Flags32);
	LPCSTR sound_voice_prefix() const;

	//////////////////////////////////////////////////////////////////////////
	u32 memory_time(const CScriptGameObject& lua_game_object);
	Fvector memory_position(const CScriptGameObject& lua_game_object);
	CScriptGameObject* best_weapon();
	void explode(u32 level_time);
	CScriptGameObject* GetEnemy() const;
	CScriptGameObject* GetCorpse() const;
	CScriptSoundInfo GetSoundInfo();
	CScriptMonsterHitInfo GetMonsterHitInfo();
	void bind_object(CScriptBinderObject* object);
	CScriptGameObject* GetCurrentOutfit() const;
	float GetCurrentOutfitProtection(int hit_type);

	bool IsOnBelt(CScriptGameObject* obj) const;
	CScriptGameObject* ItemOnBelt(u32 item_id) const;
	u32 BeltSize() const;

	void deadbody_closed(bool status);
	bool deadbody_closed_status();
	void deadbody_can_take(bool status);
	bool deadbody_can_take_status();

	void can_select_weapon(bool status);
	bool can_select_weapon() const;
	//////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////
	void set_body_state(MonsterSpace::EBodyState body_state);
	void set_movement_type(MonsterSpace::EMovementType movement_type);
	void set_mental_state(MonsterSpace::EMentalState mental_state);
	void set_path_type(MovementManager::EPathType path_type);
	void set_detail_path_type(DetailPathManager::EDetailPathType detail_path_type);

	MonsterSpace::EBodyState body_state() const;
	MonsterSpace::EBodyState target_body_state() const;
	MonsterSpace::EMovementType movement_type() const;
	MonsterSpace::EMovementType target_movement_type() const;
	MonsterSpace::EMentalState mental_state() const;
	MonsterSpace::EMentalState target_mental_state() const;
	MovementManager::EPathType path_type() const;
	DetailPathManager::EDetailPathType detail_path_type() const;

	u32 add_sound(LPCSTR prefix, u32 max_count, ESoundTypes type, u32 priority, u32 mask, u32 internal_type,
	              LPCSTR bone_name);
	u32 add_sound(LPCSTR prefix, u32 max_count, ESoundTypes type, u32 priority, u32 mask, u32 internal_type);
	u32 add_sound(LPCSTR prefix, u32 max_count, ESoundTypes type, u32 priority, u32 mask, u32 internal_type,
	              LPCSTR bone_name, LPCSTR head_anim);
	u32 add_combat_sound(LPCSTR prefix, u32 max_count, ESoundTypes type, u32 priority, u32 mask, u32 internal_type,
	                     LPCSTR bone_name);
	void remove_sound(u32 internal_type);
	void set_sound_mask(u32 sound_mask);
	void set_sight(SightManager::ESightType sight_type, Fvector* vector3d, u32 dwLookOverDelay);
	void set_sight(SightManager::ESightType sight_type, bool torso_look, bool path);
	void set_sight(SightManager::ESightType sight_type, Fvector& vector3d, bool torso_look);
	void set_sight(SightManager::ESightType sight_type, Fvector* vector3d);
	void set_sight(CScriptGameObject* object_to_look);
	void set_sight(CScriptGameObject* object_to_look, bool torso_look);
	void set_sight(CScriptGameObject* object_to_look, bool torso_look, bool fire_object);
	void set_sight(CScriptGameObject* object_to_look, bool torso_look, bool fire_object, bool no_pitch);
	void set_sight(const MemorySpace::CMemoryInfo* memory_object, bool torso_look);
	CHARACTER_RANK_VALUE GetRank();
	void play_sound(u32 internal_type);
	void play_sound(u32 internal_type, u32 max_start_time);
	void play_sound(u32 internal_type, u32 max_start_time, u32 min_start_time);
	void play_sound(u32 internal_type, u32 max_start_time, u32 min_start_time, u32 max_stop_time);
	void play_sound(u32 internal_type, u32 max_start_time, u32 min_start_time, u32 max_stop_time, u32 min_stop_time);
	void play_sound(u32 internal_type, u32 max_start_time, u32 min_start_time, u32 max_stop_time, u32 min_stop_time,
	                u32 id);

	void set_item(MonsterSpace::EObjectAction object_action);
	void set_item(MonsterSpace::EObjectAction object_action, CScriptGameObject* game_object);
	void set_item(MonsterSpace::EObjectAction object_action, CScriptGameObject* game_object, u32 queue_size);
	void set_item(MonsterSpace::EObjectAction object_action, CScriptGameObject* game_object, u32 queue_size,
	              u32 queue_interval);
	void set_desired_position();
	void set_desired_position(const Fvector* desired_position);
	void set_desired_direction();
	void set_desired_direction(const Fvector* desired_direction);
	void set_patrol_path(LPCSTR path_name, const PatrolPathManager::EPatrolStartType patrol_start_type,
	                     const PatrolPathManager::EPatrolRouteType patrol_route_type, bool random);
	void inactualize_patrol_path();
	void set_dest_level_vertex_id(u32 level_vertex_id);
	void set_dest_game_vertex_id(GameGraph::_GRAPH_ID game_vertex_id);
	void set_movement_selection_type(ESelectionType selection_type);
	u32 level_vertex_id() const;
	u32 game_vertex_id() const;
	void add_animation(LPCSTR animation, bool hand_usage, bool use_movement_controller);
	void add_animation(LPCSTR animation, bool hand_usage, Fvector position, Fvector rotation, bool local_animation);
	void clear_animations();
	int animation_count() const;
	int animation_slot() const;
	CScriptBinderObject* binded_object();
	void set_previous_point(int point_index);
	void set_start_point(int point_index);
	u32 get_current_patrol_point_index();
	bool path_completed() const;
	void patrol_path_make_inactual();
	void extrapolate_length(float extrapolate_length);
	float extrapolate_length() const;
	void enable_memory_object(CScriptGameObject* object, bool enable);
	int active_sound_count();
	int active_sound_count(bool only_playing);
	const CCoverPoint* best_cover(const Fvector& position, const Fvector& enemy_position, float radius,
	                              float min_enemy_distance, float max_enemy_distance);
	const CCoverPoint* safe_cover(const Fvector& position, float radius, float min_distance);
	CScriptIniFile* spawn_ini() const;
	bool active_zone_contact(u16 id);

	///
	void add_restrictions(LPCSTR out, LPCSTR in);
	void remove_restrictions(LPCSTR out, LPCSTR in);
	void remove_all_restrictions();
	LPCSTR in_restrictions();
	LPCSTR out_restrictions();
	LPCSTR base_in_restrictions();
	LPCSTR base_out_restrictions();
	bool accessible_position(const Fvector& position);
	bool accessible_vertex_id(u32 level_vertex_id);
	u32 accessible_nearest(const Fvector& position, Fvector& result);

	const xr_vector<MemorySpace::CVisibleObject>& memory_visible_objects() const;
	const xr_vector<MemorySpace::CSoundObject>& memory_sound_objects() const;
	const xr_vector<MemorySpace::CHitObject>& memory_hit_objects() const;
	const xr_vector<MemorySpace::CNotYetVisibleObject>& not_yet_visible_objects() const;
	float visibility_threshold() const;
	void enable_vision(bool value);
	bool vision_enabled() const;
	void set_sound_threshold(float value);
	void restore_sound_threshold();
	//////////////////////////////////////////////////////////////////////////
	void enable_attachable_item(bool value);
	bool attachable_item_enabled() const;
	void enable_night_vision(bool value);
	void night_vision_allowed(bool value);
	bool night_vision_enabled() const;
	void enable_torch(bool value);
	bool torch_enabled() const;

	// VodoXleb: add force update for torch for npc
	void update_torch();

	void attachable_item_load_attach(LPCSTR section);
	// CustomZone
	void EnableAnomaly();
	void DisableAnomaly();
	void ChangeAnomalyIdlePart(LPCSTR name, bool bIdleLight);
	float GetAnomalyPower();
	void SetAnomalyPower(float p);
	float GetAnomalyRadius();
	void SetAnomalyRadius(float p);
	void MoveAnomaly(Fvector pos);

	// HELICOPTER
	CHelicopter* get_helicopter();
	//CAR
	CCar* get_car();
	//LAMP
	CHangingLamp* get_hanging_lamp();

	//Torch
	void set_color_animator(LPCSTR name, bool bFlicker, int flickerChance, float flickerDelay, int framerate);
	void reset_color_animator();

	CHolderCustom* get_custom_holder();
	CHolderCustom* get_current_holder(); //actor only

	void start_particles(LPCSTR pname, LPCSTR bone);
	void stop_particles(LPCSTR pname, LPCSTR bone);

	bool is_body_turning() const;
	cphysics_shell_scripted* get_physics_shell() const;
	bool weapon_strapped() const;
	bool weapon_unstrapped() const;
	void eat(CScriptGameObject* item);
	bool inside(const Fvector& position, float epsilon) const;
	bool inside(const Fvector& position) const;

	Fvector head_orientation() const;
	u32 vertex_in_direction(u32 level_vertex_id, Fvector direction, float max_distance) const;

	void info_add(LPCSTR text);
	void info_clear();

	// Monster Jumper
	void jump(const Fvector& position, float factor);

	void set_ignore_monster_threshold(float ignore_monster_threshold);
	void restore_ignore_monster_threshold();
	float ignore_monster_threshold() const;
	void set_max_ignore_monster_distance(const float& max_ignore_monster_distance);
	void restore_max_ignore_monster_distance();
	float max_ignore_monster_distance() const;

	void make_object_visible_somewhen(CScriptGameObject* object);

	CScriptGameObject* item_in_slot(u32 slot_id) const;
	CScriptGameObject* active_device() const;
	void show_device(bool bFast);
	void hide_device(bool bFast);
	void force_hide_device();
	u32 active_slot();
	void activate_slot(u32 slot_id);
	void enable_level_changer(bool b);
	bool is_level_changer_enabled();
	void set_level_changer_invitation(LPCSTR str);
#ifdef DEBUG
            void				debug_planner						(const script_planner *planner);
#endif

	void sell_condition(CScriptIniFile* ini_file, LPCSTR section);
	void sell_condition(float friend_factor, float enemy_factor);
	void buy_condition(CScriptIniFile* ini_file, LPCSTR section);
	void buy_condition(float friend_factor, float enemy_factor);
	void show_condition(CScriptIniFile* ini_file, LPCSTR section);
	void buy_supplies(CScriptIniFile* ini_file, LPCSTR section);
	void buy_item_condition_factor(float factor);
	void buy_item_exponent(float factor);
	void sell_item_exponent(float factor);

	LPCSTR sound_prefix() const;
	void sound_prefix(LPCSTR sound_prefix);

	u32 location_on_path(float distance, Fvector* location);
	bool is_there_items_to_pickup() const;

	bool wounded() const;
	void wounded(bool value);

	void set_enable_movement_collision(bool value);

	CSightParams sight_params();

	void enable_movement(bool enable);
	bool movement_enabled();

	bool critically_wounded();

	bool invulnerable() const;
	void invulnerable(bool invulnerable);
	LPCSTR get_smart_cover_description() const;
	void set_visual_name(LPCSTR visual, bool bForce);
	float get_current_weight();
	float get_max_weight();
	LPCSTR get_visual_name() const;

	void reload_weapon();

	bool can_throw_grenades() const;
	void can_throw_grenades(bool can_throw_grenades);

	u32 throw_time_interval() const;
	void throw_time_interval(u32 throw_time_interval);

	u32 group_throw_time_interval() const;
	void group_throw_time_interval(u32 throw_time_interval);
	CArtefact* get_artefact();
	CZoneCampfire* get_campfire();
	CPhysicObject* get_physics_object();

	void aim_time(CScriptGameObject* weapon, u32 time);
	u32 aim_time(CScriptGameObject* weapon);

	void special_danger_move(bool value);
	bool special_danger_move();

	void sniper_update_rate(bool value);
	bool sniper_update_rate() const;

	void sniper_fire_mode(bool value);
	bool sniper_fire_mode() const;

	void aim_bone_id(LPCSTR value);
	LPCSTR aim_bone_id() const;

	void register_in_combat();
	void unregister_in_combat();
	CCoverPoint const* find_best_cover(Fvector position_to_cover_from);

	// approved by Dima smart covers functions
	bool use_smart_covers_only() const;
	void use_smart_covers_only(bool value);

	bool in_smart_cover() const;

	void set_dest_smart_cover(LPCSTR cover_id);
	void set_dest_smart_cover();
	CCoverPoint const* get_dest_smart_cover();
	LPCSTR get_dest_smart_cover_name();

	void set_dest_loophole(LPCSTR loophole_id);
	void set_dest_loophole();

	void set_smart_cover_target(Fvector position);
	void set_smart_cover_target(CScriptGameObject* object);
	void set_smart_cover_target();

	void set_smart_cover_target_selector();
	void set_smart_cover_target_selector(::luabind::functor<void> functor);
	void set_smart_cover_target_selector(::luabind::functor<void> functor, ::luabind::object object);

	void set_smart_cover_target_idle();
	void set_smart_cover_target_lookout();
	void set_smart_cover_target_fire();
	void set_smart_cover_target_fire_no_lookout();
	void set_smart_cover_target_default(bool value);

	float const idle_min_time() const;
	void idle_min_time(float value);
	float const idle_max_time() const;
	void idle_max_time(float value);
	float const lookout_min_time() const;
	void lookout_min_time(float value);
	float const lookout_max_time() const;
	void lookout_max_time(float value);

	bool in_loophole_fov(LPCSTR cover_id, LPCSTR loophole_id, Fvector object_position) const;
	bool in_current_loophole_fov(Fvector object_position) const;
	bool in_loophole_range(LPCSTR cover_id, LPCSTR loophole_id, Fvector object_position) const;
	bool in_current_loophole_range(Fvector object_position) const;

	float apply_loophole_direction_distance() const;
	void apply_loophole_direction_distance(float value);

	bool movement_target_reached();
	bool suitable_smart_cover(CScriptGameObject* object);

	void take_items_enabled(bool value);
	bool take_items_enabled() const;

	void death_sound_enabled(bool value);
	bool death_sound_enabled() const;

	void register_door();
	void unregister_door();
	void on_door_is_open();
	void on_door_is_closed();
	bool is_door_locked_for_npc() const;
	void lock_door_for_npc();
	void unlock_door_for_npc();
	bool is_door_blocked_by_npc() const;
	bool is_weapon_going_to_be_strapped(CScriptGameObject const* object) const;


#ifdef GAME_OBJECT_TESTING_EXPORTS
	//AVO: functions for object testing
	//_DECLARE_FUNCTION10(IsGameObject, bool);
	//_DECLARE_FUNCTION10(IsCar, bool);
	//_DECLARE_FUNCTION10(IsHeli, bool);
	//_DECLARE_FUNCTION10(IsHolderCustom, bool);
	_DECLARE_FUNCTION10(IsEntityAlive, bool);
	_DECLARE_FUNCTION10(IsInventoryItem, bool);
	_DECLARE_FUNCTION10(IsInventoryOwner, bool);
	_DECLARE_FUNCTION10(IsActor, bool);
	_DECLARE_FUNCTION10(IsCustomMonster, bool);
	_DECLARE_FUNCTION10(IsWeapon, bool);
	//_DECLARE_FUNCTION10(IsMedkit, bool);
	//_DECLARE_FUNCTION10(IsEatableItem, bool);
	//_DECLARE_FUNCTION10(IsAntirad, bool);
	_DECLARE_FUNCTION10(IsCustomOutfit, bool);
	_DECLARE_FUNCTION10(IsHelmet, bool);
	_DECLARE_FUNCTION10(IsScope, bool);
	_DECLARE_FUNCTION10(IsSilencer, bool);
	_DECLARE_FUNCTION10(IsGrenadeLauncher, bool);
	_DECLARE_FUNCTION10(IsWeaponMagazined, bool);
	_DECLARE_FUNCTION10(IsSpaceRestrictor, bool);
	_DECLARE_FUNCTION10(IsStalker, bool);
	_DECLARE_FUNCTION10(IsAnomaly, bool);
	_DECLARE_FUNCTION10(IsMonster, bool);
	//_DECLARE_FUNCTION10(IsExplosive, bool);
	//_DECLARE_FUNCTION10(IsScriptZone, bool);
	//_DECLARE_FUNCTION10(IsProjector, bool);
	_DECLARE_FUNCTION10(IsTrader, bool);
	_DECLARE_FUNCTION10(IsHudItem, bool);
	//_DECLARE_FUNCTION10(IsFoodItem, bool);
	_DECLARE_FUNCTION10(IsArtefact, bool);
	_DECLARE_FUNCTION10(IsAmmo, bool);
	//_DECLARE_FUNCTION10(IsMissile, bool);
	//_DECLARE_FUNCTION10(IsPhysicsShellHolder, bool);
	//_DECLARE_FUNCTION10(IsGrenade, bool);
	//_DECLARE_FUNCTION10(IsBottleItem, bool);
	//_DECLARE_FUNCTION10(IsTorch, bool);
	_DECLARE_FUNCTION10(IsWeaponGL, bool);
	_DECLARE_FUNCTION10(IsInventoryBox, bool);
#endif
	//Alundaio
#ifdef GAME_OBJECT_EXTENDED_EXPORTS
	_DECLARE_FUNCTION14(cast_Actor, CActor);
	_DECLARE_FUNCTION14(cast_Car, CCar);
	_DECLARE_FUNCTION14(cast_Heli, CHelicopter);
	_DECLARE_FUNCTION14(cast_InventoryOwner, CInventoryOwner);
	_DECLARE_FUNCTION14(cast_InventoryBox, CInventoryBox);
	_DECLARE_FUNCTION14(cast_CustomZone, CCustomZone);
	_DECLARE_FUNCTION14(cast_TorridZone, CTorridZone);
	_DECLARE_FUNCTION14(cast_MosquitoBald, CMosquitoBald);
	_DECLARE_FUNCTION14(cast_ZoneCampfire, CZoneCampfire);
	_DECLARE_FUNCTION14(cast_InventoryItem, CInventoryItem);
	_DECLARE_FUNCTION14(cast_CustomOutfit, CCustomOutfit);
	_DECLARE_FUNCTION14(cast_Helmet, CHelmet);
	_DECLARE_FUNCTION14(cast_Artefact, CArtefact);
	_DECLARE_FUNCTION14(cast_Ammo, CWeaponAmmo);
	_DECLARE_FUNCTION14(cast_Weapon, CWeapon);
	_DECLARE_FUNCTION14(cast_WeaponMagazined, CWeaponMagazined);
	_DECLARE_FUNCTION14(cast_WeaponMagazinedWGrenade, CWeaponMagazinedWGrenade);
	_DECLARE_FUNCTION14(cast_EatableItem, CEatableItem);
	_DECLARE_FUNCTION14(cast_Medkit, CMedkit);
	_DECLARE_FUNCTION14(cast_Antirad, CAntirad);
	_DECLARE_FUNCTION14(cast_FoodItem, CFoodItem);
	_DECLARE_FUNCTION14(cast_BottleItem, CBottleItem);

	void SetHealthEx(float hp); //AVO
	float GetLuminocityHemi();
	float GetLuminocity();
	bool Use(CScriptGameObject* obj);
	void StartTrade(CScriptGameObject* obj);
	void StartUpgrade(CScriptGameObject* obj);
	void SetWeight(float w);
	void IterateFeelTouch(::luabind::functor<void> functor);
	u32 GetSpatialType();
	void DestroyObject();
	void SetSpatialType(u32 sptype);
	u8 GetRestrictionType();
	void SetRestrictionType(u8 typ);

	// demonized: add getters and setters for pathfinding for npcs around anomalies and damage for npcs
	bool get_enable_anomalies_pathfinding();
	void set_enable_anomalies_pathfinding(bool v);
	bool get_enable_anomalies_damage();
	void set_enable_anomalies_damage(bool v);

	//Weapon
	void Weapon_AddonAttach(CScriptGameObject* item);
	void Weapon_AddonDetach(LPCSTR item_section, bool b_spawn_item = true);
	bool HasAmmoType(u8 type);
	int GetAmmoCount(u8 type);
	void SetAmmoType(u8 type);
	void SetMainWeaponType(u32 type);
	void SetWeaponType(u32 type);
	u32 GetMainWeaponType();
	u32 GetWeaponType();
	u8 GetWeaponSubstate();
	u8 GetAmmoType();

	//Scope
	void Weapon_SetCurrentScope(u8 type);
	u8 Weapon_GetCurrentScope();

	//CWeaponAmmo
	u16 AmmoGetCount();
	void AmmoSetCount(u16 count);
	u16 AmmoBoxSize();

	//Weapon & Outfit
	bool InstallUpgrade(LPCSTR upgrade);
	bool HasUpgrade(LPCSTR upgrade);
	void IterateInstalledUpgrades(const ::luabind::functor<bool>& functor);
	bool WeaponInGrenadeMode();

	//Car
	CScriptGameObject* GetAttachedVehicle();
	void AttachVehicle(CScriptGameObject* veh, bool bForce = false);
	void DetachVehicle(bool bForce = false);

	//Any class that is derived from CHudItem
	u32 PlayHudMotion(LPCSTR M, bool bMixIn, u32 state, float speed = 0.f, float end = 0.f);
	void SwitchState(u32 state);
	u32 GetState();
	//Works for anything with visual
	u16 bone_id(LPCSTR bone_name, bool bHud);
	u16 bone_id(LPCSTR bone_name) { return bone_id(bone_name, false); }
	LPCSTR bone_name(u16 bone_id, bool bHud);
	LPCSTR bone_name(u16 bone_id) { return bone_name(bone_id, false); }

	bool is_bone_visible(u16 bone_id, bool bHud);
	bool is_bone_visible(u16 bone_id) { return is_bone_visible(bone_id, false); }
	bool is_bone_visible(LPCSTR bone_name, bool bHud) { return is_bone_visible(bone_id(bone_name, bHud), bHud); }
	bool is_bone_visible(LPCSTR bone_name) { return is_bone_visible(bone_id(bone_name), false); }

	void set_bone_visible(u16 bone_id, bool bVisibility, bool bRecursive, bool bHud);
	void set_bone_visible(u16 bone_id, bool bVisibility, bool bRecursive) { set_bone_visible(bone_id, bVisibility, bRecursive, false); }
	void set_bone_visible(LPCSTR bone_name, bool bVisibility, bool bRecursive, bool bHud) { set_bone_visible(bone_id(bone_name, bHud), bVisibility, bRecursive, bHud); }
	void set_bone_visible(LPCSTR bone_name, bool bVisibility, bool bRecursive) { set_bone_visible(bone_id(bone_name), bVisibility, bRecursive, false); }

	Fmatrix bone_transform(u16 bone_id, bool bHud);
	Fmatrix bone_transform(u16 bone_id) { return bone_transform(bone_id, false); }
	Fmatrix bone_transform(LPCSTR bone_name, bool bHud) { return bone_transform(bone_id(bone_name, bHud), bHud); }
	Fmatrix bone_transform(LPCSTR bone_name) { return bone_transform(bone_id(bone_name), false); }

	Fvector bone_position(u16 bone_id, bool bHud);
	Fvector bone_position(u16 bone_id) { return bone_position(bone_id, false); }
	Fvector bone_position(LPCSTR bone_name, bool bHud) { return bone_position(bone_id(bone_name, bHud), bHud); }
	Fvector bone_position(LPCSTR bone_name) { return bone_position(bone_id(bone_name), false); }

	Fvector bone_direction(u16 bone_id, bool bHud);
	Fvector bone_direction(u16 bone_id) { return bone_direction(bone_id, false); }
	Fvector bone_direction(LPCSTR bone_name, bool bHud) { return bone_direction(bone_id(bone_name, bHud), bHud); }
	Fvector bone_direction(LPCSTR bone_name) { return bone_direction(bone_id(bone_name), false); }

	u16 bone_parent(u16 bone_id, bool bHud);
	u16 bone_parent(u16 bone_id) { return bone_parent(bone_id, false); }
	u16 bone_parent(LPCSTR bone_name, bool bHud) { return bone_parent(bone_id(bone_name, bHud), bHud); }
	u16 bone_parent(LPCSTR bone_name) { return bone_parent(bone_id(bone_name), false); }

	::luabind::object list_bones(bool bHud = false);

	bool IsBoneVisible(LPCSTR bone_name, bool bHud = false);	
	void SetBoneVisible(LPCSTR bone_name, bool bVisibility, bool bRecursive = true, bool bHud = false);	
	//CAI_Stalker
	void ResetBoneProtections(LPCSTR imm_sect, LPCSTR bone_sect);
	//Anything with PPhysicShell (ie. car, actor, stalker, monster, heli)
	void ForceSetPosition(Fvector pos, bool enable = true);
	void ForceSetRotation(Fvector rot, bool enable = true);
	void ForceSetAngle(Fvector ang, bool bActivate);
	Fvector Angle();

	//Artifacts
	float GetArtefactHealthRestoreSpeed();
	float GetArtefactRadiationRestoreSpeed();
	float GetArtefactSatietyRestoreSpeed();
	float GetArtefactPowerRestoreSpeed();
	float GetArtefactBleedingRestoreSpeed();
	float GetArtefactImmunity(ALife::EHitType hit_type);

	void SetArtefactHealthRestoreSpeed(float value);
	void SetArtefactRadiationRestoreSpeed(float value);
	void SetArtefactSatietyRestoreSpeed(float value);
	void SetArtefactPowerRestoreSpeed(float value);
	void SetArtefactBleedingRestoreSpeed(float value);
	void SetArtefactImmunity(ALife::EHitType hit_type, float value);

	float GetArtefactAdditionalInventoryWeight();
	void SetArtefactAdditionalInventoryWeight(float value);

	//Eatable items
	void SetRemainingUses(u8 value);
	u8 GetRemainingUses();
	u8 GetMaxUses();

	//Phantom
	void PhantomSetEnemy(CScriptGameObject*);

	//Actor

	float GetActorMaxWeight() const;
	void SetActorMaxWeight(float max_weight);
	float GetActorMaxWalkWeight() const;
	void SetActorMaxWalkWeight(float max_walk_weight);
	float GetAdditionalMaxWeight() const;
	void SetAdditionalMaxWeight(float add_max_weight);
	float GetAdditionalMaxWalkWeight() const;
	void SetAdditionalMaxWalkWeight(float add_max_walk_weight);
	float GetTotalWeight() const;

	// demonized: force update of weight
	float GetTotalWeightForceUpdate() const;
	void UpdateWeight() const;

	float Weight() const;

	// demonized: get luminosity as displayed in ui
	float GetActorUILuminosity();

	float GetActorJumpSpeed() const;
	void SetActorJumpSpeed(float jump_speed);
	float GetActorSprintKoef() const;
	void SetActorSprintKoef(float sprint_koef);
	float GetActorRunCoef() const;
	void SetActorRunCoef(float run_coef);
	float GetActorRunBackCoef() const;
	void SetActorRunBackCoef(float run_back_coef);
	void SetActorCamBoxYOffset(u32 box_num, float offset);
	float GetActorWalkAccel() const;
	void SetActorWalkAccel(float val);
	float GetActorWalkBackCoef() const;
	void SetActorWalkBackCoef(float val);

	// demonized: Adjust Lookout coeff
	float GetActorLookoutCoef() const;
	void SetActorLookoutCoef(float val);

	float GetActorCrouchCoef() const;
	void SetActorCrouchCoef(float val);
	float GetActorClimbCoef() const;
	void SetActorClimbCoef(float val);
	float GetActorWalkStrafeCoef() const;
	void SetActorWalkStrafeCoef(float val);
	float GetActorRunStrafeCoef() const;
	void SetActorRunStrafeCoef(float val);
	float GetActorSprintStrafeCoef() const;
	void SetActorSprintStrafeCoef(float val);

	CScriptGameObject* GetActorObjectLookingAt();
	CScriptGameObject* GetActorPersonLookingAt();
	LPCSTR GetActorDefaultActionForObject();

	void SetCharacterIcon(LPCSTR iconName);

	// demonized: get talking npc
	CScriptGameObject* get_talking_npc();

	// demonized: get and set scope UI
	::luabind::object get_scope_ui();
	void set_scope_ui(LPCSTR scope_texture);
#endif
	//-Alundaio

	::luabind::object GetShaders(bool bHud = false);
	::luabind::object GetDefaultShaders(bool bHud = false);
	void SetShaderTexture(int id, LPCSTR shader, LPCSTR texture, bool bHud = false);
	void ResetShaderTexture(int id, bool bHud = false);

	script_attachment* AddAttachment(LPCSTR name, LPCSTR model_name);
	script_attachment* GetAttachment(LPCSTR name);
	void RemoveAttachment(LPCSTR name);
	void RemoveAttachment(script_attachment* child);
	void IterateAttachments(::luabind::functor<bool> functor);
	void memory_remove_links(const CScriptGameObject* tpLuaGameObject);

	doors::door* m_door;

DECLARE_SCRIPT_REGISTER_FUNCTION
};

add_to_type_list(CScriptGameObject)
#undef script_type_list
#define script_type_list save_type_list(CScriptGameObject)

extern void sell_condition(CScriptIniFile* ini_file, LPCSTR section);
extern void sell_condition(float friend_factor, float enemy_factor);
extern void buy_condition(CScriptIniFile* ini_file, LPCSTR section);
extern void buy_condition(float friend_factor, float enemy_factor);
extern void show_condition(CScriptIniFile* ini_file, LPCSTR section);

////////////////////////////////////////////////////////////////////////////
//	Module 		: xrServer_Objects_ALife.h
//	Created 	: 19.09.2002
//  Modified 	: 04.06.2003
//	Author		: Oles Shyshkovtsov, Alexander Maksimchuk, Victor Reutskiy and Dmitriy Iassenev
//	Description : Server objects monsters for ALife simulator
////////////////////////////////////////////////////////////////////////////

#ifndef xrServer_Objects_ALife_MonstersH
#define xrServer_Objects_ALife_MonstersH

#include "xrServer_Objects_ALife.h"
#include "xrServer_Objects_ALife_Items.h"
#include "character_info_defs.h"
#include "associative_vector.h"
#include "alife_movement_manager_holder.h"


class CALifeMonsterBrain;
class CALifeHumanBrain;
class CALifeOnlineOfflineGroupBrain;

#pragma warning(push)
#pragma warning(disable:4005)

SERVER_ENTITY_DECLARE_BEGIN0(CSE_ALifeTraderAbstract)

	enum eTraderFlags
	{
		eTraderFlagInfiniteAmmo = u32(1) << 0,
		eTraderFlagNightVisionActive = u32(1) << 1,
		eTraderFlagDummy = u32(-1),
	};

	//	float							m_fCumulativeItemMass;
	//	int								m_iCumulativeItemVolume;
	u32 m_dwMoney;
	float m_fMaxItemMass;
	Flags32 m_trader_flags;

	////////////////////////////////////////////////////
	//character profile info
#ifndef  AI_COMPILER
	shared_str character_profile();
	void set_character_profile(shared_str);
	shared_str specific_character();
	void set_specific_character(shared_str);
#endif

	CHARACTER_COMMUNITY_INDEX m_community_index;
	CHARACTER_REPUTATION_VALUE m_reputation;
	CHARACTER_RANK_VALUE m_rank;
	xr_string m_character_name;
	xr_string m_character_name_str;
	shared_str m_icon_name;
	bool m_deadbody_can_take;
	bool m_deadbody_closed;

#ifdef XRGAME_EXPORTS
	//для работы с relation system
	u16 object_id() const;
	CHARACTER_COMMUNITY_INDEX Community() const;
	LPCSTR CommunityName() const;
	CHARACTER_RANK_VALUE Rank();
	CHARACTER_REPUTATION_VALUE Reputation();
	void SetRank(CHARACTER_RANK_VALUE val);

#endif

	shared_str m_sCharacterProfile;
	shared_str m_SpecificCharacter;

	//буферный вектор проверенных персонажей
	xr_vector<shared_str> m_CheckedCharacters;
	xr_vector<shared_str> m_DefaultCharacters;

public:
	CSE_ALifeTraderAbstract(LPCSTR caSection);
	virtual ~CSE_ALifeTraderAbstract();
	// we need this to prevent virtual inheritance :-(
	virtual CSE_Abstract* base() = 0;
	virtual const CSE_Abstract* base() const = 0;
	virtual CSE_Abstract* init();
	virtual CSE_Abstract* cast_abstract() { return 0; };
	virtual CSE_ALifeTraderAbstract* cast_trader_abstract() { return this; };
	// end of the virtual inheritance dependant code
	void __stdcall OnChangeProfile(PropValue* sender);

#ifdef XRGAME_EXPORTS
	virtual void add_online(const bool& update_registries);
	virtual void add_offline(const xr_vector<ALife::_OBJECT_ID>& saved_children, const bool& update_registries);
#if 0//def DEBUG
			bool					check_inventory_consistency	();
#endif
	void vfInitInventory();
	virtual void spawn_supplies();
#endif
SERVER_ENTITY_DECLARE_END

add_to_type_list(CSE_ALifeTraderAbstract)
#define script_type_list save_type_list(CSE_ALifeTraderAbstract)

SERVER_ENTITY_DECLARE_BEGIN2(CSE_ALifeTrader, CSE_ALifeDynamicObjectVisual, CSE_ALifeTraderAbstract)
	CSE_ALifeTrader(LPCSTR caSection);
	virtual ~CSE_ALifeTrader();
	virtual bool interactive() const;
	virtual CSE_Abstract* init();
	virtual CSE_Abstract* base();
	virtual const CSE_Abstract* base() const;
	virtual bool natural_weapon() const { return false; }
	virtual bool natural_detector() const { return false; }

#ifdef XRGAME_EXPORTS
	u32 dwfGetItemCost(CSE_ALifeInventoryItem* tpALifeInventoryItem);
	virtual void spawn_supplies();
	virtual void add_online(const bool& update_registries);
	virtual void add_offline(const xr_vector<ALife::_OBJECT_ID>& saved_children, const bool& update_registries);
#endif
#ifdef DEBUG
	virtual bool					match_configuration		() const;
#endif
	virtual CSE_Abstract* cast_abstract() { return this; };
	virtual CSE_ALifeTraderAbstract* cast_trader_abstract() { return this; };
	virtual CSE_ALifeTrader* cast_trader() { return this; };
SERVER_ENTITY_DECLARE_END

add_to_type_list(CSE_ALifeTrader)
#define script_type_list save_type_list(CSE_ALifeTrader)

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeCustomZone, CSE_ALifeSpaceRestrictor)
	//.	f32								m_maxPower;
	ALife::EHitType m_tHitType;
	u32 m_owner_id;
	u32 m_enabled_time;
	u32 m_disabled_time;
	u32 m_start_time_shift;

	CSE_ALifeCustomZone(LPCSTR caSection);
	virtual ~CSE_ALifeCustomZone();
SERVER_ENTITY_DECLARE_END

add_to_type_list(CSE_ALifeCustomZone)
#define script_type_list save_type_list(CSE_ALifeCustomZone)

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeAnomalousZone, CSE_ALifeCustomZone)
	CSE_ALifeItemWeapon* m_tpCurrentBestWeapon;
	float m_offline_interactive_radius;
	u32 m_artefact_position_offset;
	u16 m_artefact_spawn_count;

	CSE_ALifeAnomalousZone(LPCSTR caSection);
	virtual ~CSE_ALifeAnomalousZone();
	virtual CSE_Abstract* init();
	virtual CSE_Abstract* base();
	virtual const CSE_Abstract* base() const;
	virtual CSE_Abstract* cast_abstract() { return this; };
	virtual CSE_ALifeAnomalousZone* cast_anomalous_zone() { return this; };
	virtual u32 ef_anomaly_type() const;
	virtual u32 ef_weapon_type() const;
	virtual u32 ef_creature_type() const;
#ifdef XRGAME_EXPORTS
	//			void					spawn_artefacts			();
	virtual void on_spawn();
	virtual CSE_ALifeItemWeapon* tpfGetBestWeapon(ALife::EHitType& tHitType, float& fHitPower);
	virtual ALife::EMeetActionType tfGetActionType(CSE_ALifeSchedulable* tpALifeSchedulable, int iGroupIndex,
	                                               bool bMutualDetection);
	virtual bool bfActive();
	virtual CSE_ALifeDynamicObject* tpfGetBestDetector();
	virtual bool keep_saved_data_anyway() const;
#endif
SERVER_ENTITY_DECLARE_END

add_to_type_list(CSE_ALifeAnomalousZone)
#define script_type_list save_type_list(CSE_ALifeAnomalousZone)

SERVER_ENTITY_DECLARE_BEGIN2(CSE_ALifeTorridZone, CSE_ALifeCustomZone, CSE_Motion)
	CSE_ALifeTorridZone(LPCSTR caSection);
	virtual ~CSE_ALifeTorridZone();
	virtual CSE_Motion* __stdcall motion();
SERVER_ENTITY_DECLARE_END

add_to_type_list(CSE_ALifeTorridZone)
#define script_type_list save_type_list(CSE_ALifeTorridZone)

SERVER_ENTITY_DECLARE_BEGIN2(CSE_ALifeZoneVisual, CSE_ALifeAnomalousZone, CSE_Visual)
	shared_str attack_animation;
	CSE_ALifeZoneVisual(LPCSTR caSection);
	virtual ~CSE_ALifeZoneVisual();
	virtual CSE_Visual* __stdcall visual();
SERVER_ENTITY_DECLARE_END

add_to_type_list(CSE_ALifeZoneVisual)
#define script_type_list save_type_list(CSE_ALifeZoneVisual)

//---------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeCreatureAbstract, CSE_ALifeDynamicObjectVisual)
private:
	float fHealth;
	ALife::_OBJECT_ID m_killer_id;
public:
	u8 s_team;
	u8 s_squad;
	u8 s_group;

	float m_fMorale;
	float m_fAccuracy;
	float m_fIntelligence;

	u32 timestamp; // server(game) timestamp
	u8 flags;
	float o_model; // model yaw
	SRotation o_torso; // torso in world coords
	bool m_bDeathIsProcessed;

	xr_vector<ALife::_OBJECT_ID> m_dynamic_out_restrictions;
	xr_vector<ALife::_OBJECT_ID> m_dynamic_in_restrictions;

	u32 m_ef_creature_type;
	u32 m_ef_weapon_type;
	u32 m_ef_detector_type;

	ALife::_TIME_ID m_game_death_time;

	CSE_ALifeCreatureAbstract(LPCSTR caSection);
	virtual ~CSE_ALifeCreatureAbstract();
	virtual u8 g_team();
	virtual u8 g_squad();
	virtual u8 g_group();

	IC float get_health() const { return fHealth; }
	IC ALife::_OBJECT_ID get_killer_id() const { return m_killer_id; }

	void set_health(float const health_value);
	void set_killer_id(ALife::_OBJECT_ID const killer_id);

	IC bool g_Alive() const { return (get_health() > 0.f); }
	virtual bool used_ai_locations() const;
	virtual bool can_switch_online() const;
	virtual bool can_switch_offline() const;
	virtual u32 ef_creature_type() const;
	virtual u32 ef_weapon_type() const;
	virtual u32 ef_detector_type() const;
	virtual CSE_ALifeCreatureAbstract* cast_creature_abstract() { return this; };
#ifdef XRGAME_EXPORTS
	virtual void on_death(CSE_Abstract* killer);
	virtual void on_spawn();
#endif
#ifdef DEBUG
	virtual bool					match_configuration		() const;
#endif
SERVER_ENTITY_DECLARE_END

add_to_type_list(CSE_ALifeCreatureAbstract)
#define script_type_list save_type_list(CSE_ALifeCreatureAbstract)

SERVER_ENTITY_DECLARE_BEGIN3(CSE_ALifeMonsterAbstract, CSE_ALifeCreatureAbstract, CSE_ALifeSchedulable,
                             CMovementManagerHolder)

	float m_fMaxHealthValue;
	float m_fRetreatThreshold;
	float m_fEyeRange;
	float m_fHitPower;
	ALife::EHitType m_tHitType;
	shared_str m_out_space_restrictors;
	shared_str m_in_space_restrictors;
	svector<float, ALife::eHitTypeMax> m_fpImmunityFactors;

	ALife::_OBJECT_ID m_smart_terrain_id;

	//---------------------------------------------------------
	// bool if monster under smart terrain and currently executes task
	// if monster on the way then (m_smart_terrain_id != 0xffff) && (!m_task_reached)
	bool m_task_reached;
	//---------------------------------------------------------

	int m_rank;

	ALife::_TIME_ID m_stay_after_death_time_interval;
public:
	ALife::_OBJECT_ID m_group_id;

public:
	CSE_ALifeMonsterAbstract(LPCSTR caSection);
	virtual ~CSE_ALifeMonsterAbstract();
	IC float g_MaxHealth() const { return m_fMaxHealthValue; }
	virtual CSE_Abstract* init();
	virtual CSE_Abstract* base();
	virtual const CSE_Abstract* base() const;
	virtual CSE_Abstract* cast_abstract() { return this; };
	virtual CSE_ALifeSchedulable* cast_schedulable() { return this; };
	virtual CSE_ALifeMonsterAbstract* cast_monster_abstract() { return this; };

	IC CALifeMonsterBrain& brain() const
	{
		VERIFY(m_brain);
		return (*m_brain);
	}

	virtual CALifeMonsterBrain* create_brain();
	virtual u32 ef_creature_type() const;
	virtual u32 ef_weapon_type() const;
	virtual u32 ef_detector_type() const;

	IC int Rank() { return m_rank; }

#ifdef XRGAME_EXPORTS
	void kill();
	bool has_detector();
#endif // #ifdef XRGAME_EXPORTS

#ifndef XRGAME_EXPORTS
	virtual	void					update					()	{};
#else
	virtual void update();
	virtual CSE_ALifeItemWeapon* tpfGetBestWeapon(ALife::EHitType& tHitType, float& fHitPower);
	virtual ALife::EMeetActionType tfGetActionType(CSE_ALifeSchedulable* tpALifeSchedulable, int iGroupIndex,
	                                               bool bMutualDetection);
	virtual bool bfActive();
	virtual CSE_ALifeDynamicObject* tpfGetBestDetector();

	virtual void vfDetachAll(bool bFictitious = false)
	{
	};
	void vfCheckForPopulationChanges();
	virtual void add_online(const bool& update_registries);
	virtual void add_offline(const xr_vector<ALife::_OBJECT_ID>& saved_children, const bool& update_registries);
	virtual void on_register();
	virtual void on_unregister();
	virtual Fvector draw_level_position() const;
	virtual bool redundant() const;
	virtual void on_location_change() const;
	virtual CSE_ALifeDynamicObject const& get_object() const { return *this; }
	virtual CSE_ALifeDynamicObject& get_object() { return *this; }

#endif
	virtual bool need_update(CSE_ALifeDynamicObject* object);

private:
	CALifeMonsterBrain* m_brain;

public:
SERVER_ENTITY_DECLARE_END

add_to_type_list(CSE_ALifeMonsterAbstract)
#define script_type_list save_type_list(CSE_ALifeMonsterAbstract)

SERVER_ENTITY_DECLARE_BEGIN3(CSE_ALifeCreatureActor, CSE_ALifeCreatureAbstract, CSE_ALifeTraderAbstract, CSE_PHSkeleton)

	u16 mstate;
	Fvector accel;
	Fvector velocity;
	//	float							fArmor;
	float fRadiation;
	u8 weapon;
	///////////////////////////////////////////
	u16 m_u16NumItems;
	u16 m_holderID;
	//	DEF_DEQUE		(PH_STATES, SPHNetState); 
	SPHNetState m_AliveState;
	//	PH_STATES						m_DeadStates;

	// статический массив - 6 float(вектора пределов квантизации) + m_u16NumItems*(7 u8) (позиция и поворот кости)
	u8 m_BoneDataSize;
	char m_DeadBodyData[1024];
	///////////////////////////////////////////
	CSE_ALifeCreatureActor(LPCSTR caSection);
	virtual ~CSE_ALifeCreatureActor();
	virtual CSE_Abstract* base();
	virtual const CSE_Abstract* base() const;
	virtual CSE_Abstract* init();
	virtual void load(NET_Packet& tNetPacket);
	virtual bool can_save() const { return true; }
	virtual bool natural_weapon() const { return false; }
	virtual bool natural_detector() const { return false; }
#ifdef XRGAME_EXPORTS
	virtual void spawn_supplies();
	virtual void add_online(const bool& update_registries);
	virtual void add_offline(const xr_vector<ALife::_OBJECT_ID>& saved_children, const bool& update_registries);
#endif
#ifdef DEBUG
	virtual bool					match_configuration		() const;
#endif
	virtual CSE_Abstract* cast_abstract() { return this; };
	virtual CSE_ALifeTraderAbstract* cast_trader_abstract() { return this; };
public:
	virtual BOOL Net_Relevant();
SERVER_ENTITY_DECLARE_END

add_to_type_list(CSE_ALifeCreatureActor)
#define script_type_list save_type_list(CSE_ALifeCreatureActor)

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeCreatureCrow, CSE_ALifeCreatureAbstract)
	CSE_ALifeCreatureCrow(LPCSTR caSection);
	virtual ~CSE_ALifeCreatureCrow();
	virtual bool used_ai_locations() const;
SERVER_ENTITY_DECLARE_END

add_to_type_list(CSE_ALifeCreatureCrow)
#define script_type_list save_type_list(CSE_ALifeCreatureCrow)

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeCreaturePhantom, CSE_ALifeCreatureAbstract)
	CSE_ALifeCreaturePhantom(LPCSTR caSection);
	virtual ~CSE_ALifeCreaturePhantom();
	virtual bool used_ai_locations() const;
SERVER_ENTITY_DECLARE_END

add_to_type_list(CSE_ALifeCreaturePhantom)
#define script_type_list save_type_list(CSE_ALifeCreaturePhantom)

SERVER_ENTITY_DECLARE_BEGIN2(CSE_ALifeMonsterRat, CSE_ALifeMonsterAbstract, CSE_ALifeInventoryItem)
	// Personal characteristics:
	float fEyeFov;
	float fEyeRange;
	float fMinSpeed;
	float fMaxSpeed;
	float fAttackSpeed;
	float fMaxPursuitRadius;
	float fMaxHomeRadius;
	// morale
	float fMoraleSuccessAttackQuant;
	float fMoraleDeathQuant;
	float fMoraleFearQuant;
	float fMoraleRestoreQuant;
	u16 u16MoraleRestoreTimeInterval;
	float fMoraleMinValue;
	float fMoraleMaxValue;
	float fMoraleNormalValue;
	// attack
	float fHitPower;
	u16 u16HitInterval;
	float fAttackDistance;
	float fAttackAngle;
	float fAttackSuccessProbability;

	CSE_ALifeMonsterRat(LPCSTR caSection); // constructor for variable initialization
	virtual ~CSE_ALifeMonsterRat();
	virtual bool bfUseful();
	virtual CSE_Abstract* init();
	virtual CSE_Abstract* base();
	virtual const CSE_Abstract* base() const;
	virtual CSE_Abstract* cast_abstract() { return this; };
	virtual CSE_ALifeInventoryItem* cast_inventory_item() { return this; };
SERVER_ENTITY_DECLARE_END

add_to_type_list(CSE_ALifeMonsterRat)
#define script_type_list save_type_list(CSE_ALifeMonsterRat)

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifeMonsterZombie, CSE_ALifeMonsterAbstract)
	// Personal characteristics:
	float fEyeFov;
	float fEyeRange;
	float fMinSpeed;
	float fMaxSpeed;
	float fAttackSpeed;
	float fMaxPursuitRadius;
	float fMaxHomeRadius;
	// attack
	float fHitPower;
	u16 u16HitInterval;
	float fAttackDistance;
	float fAttackAngle;

	CSE_ALifeMonsterZombie(LPCSTR caSection); // constructor for variable initialization
	virtual ~CSE_ALifeMonsterZombie();
SERVER_ENTITY_DECLARE_END

add_to_type_list(CSE_ALifeMonsterZombie)
#define script_type_list save_type_list(CSE_ALifeMonsterZombie)

SERVER_ENTITY_DECLARE_BEGIN2(CSE_ALifeMonsterBase, CSE_ALifeMonsterAbstract, CSE_PHSkeleton)
	u16 m_spec_object_id;

	CSE_ALifeMonsterBase(LPCSTR caSection); // constructor for variable initialization
	virtual ~CSE_ALifeMonsterBase();
	virtual void load(NET_Packet& tNetPacket);
	virtual CSE_Abstract* cast_abstract() { return this; }

	virtual void spawn_supplies(LPCSTR)
	{
	}

	virtual void spawn_supplies()
	{
	}
#ifdef XRGAME_EXPORTS
	virtual void on_spawn();
	virtual void add_online(const bool& update_registries);
	virtual void add_offline(const xr_vector<ALife::_OBJECT_ID>& saved_children, const bool& update_registries);
#endif // XRGAME_EXPORTS
SERVER_ENTITY_DECLARE_END

add_to_type_list(CSE_ALifeMonsterBase)
#define script_type_list save_type_list(CSE_ALifeMonsterBase)

SERVER_ENTITY_DECLARE_BEGIN(CSE_ALifePsyDogPhantom, CSE_ALifeMonsterBase)
	CSE_ALifePsyDogPhantom(LPCSTR caSection); // constructor for variable initialization
	virtual ~CSE_ALifePsyDogPhantom();
	virtual CSE_Abstract* cast_abstract() { return this; }
	virtual bool bfActive() { return false; }
SERVER_ENTITY_DECLARE_END

add_to_type_list(CSE_ALifePsyDogPhantom)
#define script_type_list save_type_list(CSE_ALifePsyDogPhantom)


//-------------------------------
SERVER_ENTITY_DECLARE_BEGIN2(CSE_ALifeHumanAbstract, CSE_ALifeTraderAbstract, CSE_ALifeMonsterAbstract)


public:
	CSE_ALifeHumanAbstract(LPCSTR caSection);
	virtual ~CSE_ALifeHumanAbstract();
	virtual CSE_Abstract* init();
	virtual CSE_Abstract* base();
	virtual const CSE_Abstract* base() const;
	virtual CSE_Abstract* cast_abstract() { return this; };
	virtual CSE_ALifeTraderAbstract* cast_trader_abstract() { return this; };
	virtual CSE_ALifeHumanAbstract* cast_human_abstract() { return this; };
	virtual bool natural_weapon() const { return false; }
	virtual bool natural_detector() const { return false; }
	IC CALifeHumanBrain& brain() const
	{
		VERIFY(m_brain);
		return (*m_brain);
	}

	virtual CALifeMonsterBrain* create_brain();

#ifdef XRGAME_EXPORTS
	virtual void update();
	virtual CSE_ALifeItemWeapon* tpfGetBestWeapon(ALife::EHitType& tHitType, float& fHitPower);
	virtual bool bfPerformAttack();
	virtual void vfUpdateWeaponAmmo();
	virtual void vfProcessItems();
	virtual void vfAttachItems(ALife::ETakeType tTakeType = ALife::eTakeTypeAll);
	virtual ALife::EMeetActionType tfGetActionType(CSE_ALifeSchedulable* tpALifeSchedulable, int iGroupIndex,
	                                               bool bMutualDetection);
	virtual CSE_ALifeDynamicObject* tpfGetBestDetector();
	virtual void vfDetachAll(bool bFictitious = false);
	virtual void spawn_supplies();
	virtual void on_register();
	virtual void on_unregister();
	virtual void add_online(const bool& update_registries);
	virtual void add_offline(const xr_vector<ALife::_OBJECT_ID>& saved_children, const bool& update_registries);
#endif

private:
	CALifeHumanBrain* m_brain;

SERVER_ENTITY_DECLARE_END

add_to_type_list(CSE_ALifeHumanAbstract)
#define script_type_list save_type_list(CSE_ALifeHumanAbstract)

SERVER_ENTITY_DECLARE_BEGIN2(CSE_ALifeHumanStalker, CSE_ALifeHumanAbstract, CSE_PHSkeleton)
	shared_str m_start_dialog;

	CSE_ALifeHumanStalker(LPCSTR caSection);
	virtual ~CSE_ALifeHumanStalker();
	virtual void load(NET_Packet& tNetPacket);
	virtual CSE_Abstract* cast_abstract() { return this; }
SERVER_ENTITY_DECLARE_END

add_to_type_list(CSE_ALifeHumanStalker)
#define script_type_list save_type_list(CSE_ALifeHumanStalker)

SERVER_ENTITY_DECLARE_BEGIN3(CSE_ALifeOnlineOfflineGroup, CSE_ALifeDynamicObject, CSE_ALifeSchedulable,
                             CMovementManagerHolder)
public:
	CSE_ALifeOnlineOfflineGroup(LPCSTR caSection);
	virtual ~CSE_ALifeOnlineOfflineGroup();
	virtual CSE_Abstract* base();
	virtual const CSE_Abstract* base() const;
	virtual CSE_Abstract* init();
	virtual CSE_Abstract* cast_abstract() { return this; };
	virtual CSE_ALifeSchedulable* cast_schedulable() { return this; };
	virtual CSE_ALifeOnlineOfflineGroup* cast_online_offline_group() { return this; };
#ifdef XRGAME_EXPORTS
	virtual CALifeSmartTerrainTask* get_current_task();
#endif // #ifdef XRGAME_EXPORTS

public:
	typedef CSE_ALifeMonsterAbstract MEMBER;
	typedef associative_vector<ALife::_OBJECT_ID, MEMBER*> MEMBERS;

private:
	MEMBERS m_members;

#ifdef XRGAME_EXPORTS

private:
	CALifeOnlineOfflineGroupBrain* m_brain;

public:
	IC CALifeOnlineOfflineGroupBrain& brain() const
	{
		VERIFY(m_brain);
		return (*m_brain);
	}

public:
	virtual CSE_ALifeItemWeapon* tpfGetBestWeapon(ALife::EHitType& tHitType, float& fHitPower);
	virtual ALife::EMeetActionType tfGetActionType(CSE_ALifeSchedulable* tpALifeSchedulable, int iGroupIndex,
	                                               bool bMutualDetection);
	virtual bool bfActive();
	virtual CSE_ALifeDynamicObject* tpfGetBestDetector();
	virtual void update();
	virtual bool need_update(CSE_ALifeDynamicObject* object);
	void register_member(ALife::_OBJECT_ID member_id);
	void unregister_member(ALife::_OBJECT_ID member_id);
	void notify_on_member_death(MEMBER* member);
	MEMBER* member(ALife::_OBJECT_ID member_id, bool no_assert = false);
	virtual void on_before_register();
	void on_after_game_load();
	virtual bool synchronize_location();
	virtual void try_switch_online();
	virtual void try_switch_offline();
	virtual void switch_online();
	virtual void switch_offline();
	virtual bool redundant() const;
	virtual void on_location_change() const;
	virtual CSE_ALifeDynamicObject const& get_object() const { return *this; }
	virtual CSE_ALifeDynamicObject& get_object() { return *this; }
	ALife::_OBJECT_ID commander_id();
	MEMBERS const& squad_members() const;
	u32 npc_count() const;
	void clear_location_types();
	void add_location_type(LPCSTR mask);
	void force_change_position(Fvector position);
	//void					force_change_game_vertex_id(GameGraph::_GRAPH_ID game_vertex_id);
	virtual void on_failed_switch_online();
#else
public:
	virtual void					update					() {};
#endif

SERVER_ENTITY_DECLARE_END

add_to_type_list(CSE_ALifeOnlineOfflineGroup)
#define script_type_list save_type_list(CSE_ALifeOnlineOfflineGroup)

#pragma warning(pop)

#endif

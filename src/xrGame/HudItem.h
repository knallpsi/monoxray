﻿#pragma once

class CSE_Abstract;
class CPhysicItem;
class NET_Packet;
class CInventoryItem;
class CMotionDef;
class CUIWindow;

#include "actor_defs.h"
#include "inventory_space.h"
#include "hudsound.h"
#include "HUDManager.h"

#define TENDTO_SPEED         1.0f     // Модификатор силы инерции (больше - чувствительней)
#define TENDTO_SPEED_AIM     1.0f     // (Для прицеливания)
#define TENDTO_SPEED_RET     5.0f     // Модификатор силы отката инерции (больше - быстрее)
#define TENDTO_SPEED_RET_AIM 5.0f     // (Для прицеливания)
#define INERT_MIN_ANGLE      0.0f     // Минимальная сила наклона, необходимая для старта инерции
#define INERT_MIN_ANGLE_AIM  3.5f     // (Для прицеливания)

// Пределы смещения при инерции (лево / право / верх / низ)
#define ORIGIN_OFFSET        0.04f,  0.04f,  0.04f, 0.02f
#define ORIGIN_OFFSET_AIM    0.015f, 0.015f, 0.01f, 0.005f

struct attachable_hud_item;
class motion_marks;

enum ENearWallMode {
	NW_OFF = 0,
	NW_FOV,
	NW_POS,
	NW_MAX
};

enum ENearWallTrace {
	NT_CAM = 0,
	NT_ITEM,
	NT_MAX
};

class CHUDState
{
public:
	enum EHudStates
	{
		eIdle = 0,
		eShowing,
		eHiding,
		eHidden,
		eBore,
		eLastBaseState = eBore,
	};

private:
	u32 m_hud_item_state;
	u32 m_nextState;
	u32 m_dw_curr_state_time;
protected:
	u32 m_dw_curr_substate_time;
	u32 m_lastState;
public:
	CHUDState() { SetState(eHidden); }
	IC u32 GetNextState() const { return m_nextState; }
	IC u32 GetState() const { return m_hud_item_state; }

	IC void SetState(u32 v)
	{
		m_hud_item_state = v;
		m_dw_curr_state_time = Device.dwTimeGlobal;
		ResetSubStateTime();
	}

	IC void SetNextState(u32 v) { m_nextState = v; }
	IC u32 CurrStateTime() const { return Device.dwTimeGlobal - m_dw_curr_state_time; }
	IC void ResetSubStateTime() { m_dw_curr_substate_time = Device.dwTimeGlobal; }
	virtual void SwitchState(u32 S) = 0;
	virtual void OnStateSwitch(u32 S, u32 oldState) = 0;
};

class CHudItem : public CHUDState
{
protected:
	CHudItem();
	virtual ~CHudItem();
	virtual DLL_Pure* _construct();

	Flags16 m_huditem_flags;

	enum
	{
		fl_pending = (1 << 0),
		fl_renderhud = (1 << 1),
		fl_inertion_enable = (1 << 2),
		fl_inertion_allow = (1 << 3),
	};

	struct
	{
		const CMotionDef* m_current_motion_def;
		shared_str m_current_motion;
		u32 m_dwMotionCurrTm;
		u32 m_dwMotionStartTm;
		u32 m_dwMotionEndTm;
		u32 m_startedMotionState;
		u8 m_started_rnd_anim_idx;
		bool m_bStopAtEndAnimIsRunning;
	};

	attachable_hud_item* m_attachable;

	float m_fLR_CameraFactor; // Фактор бокового наклона худа при ходьбе [-1; +1]
	float m_fLR_MovingFactor; // Фактор бокового наклона худа при движении камеры [-1; +1]
	float m_fLR_InertiaFactor; // Фактор горизонтальной инерции худа при движении камеры [-1; +1]
	float m_fUD_InertiaFactor; // Фактор вертикальной инерции худа при движении камеры [-1; +1]

	CUIWindow* script_ui;
	LPCSTR script_ui_funct;
	LPCSTR script_ui_bone;
	Fvector script_ui_offset[2]; //pos, rot
	Fmatrix script_ui_matrix;

public:
	virtual void Load(LPCSTR section);
	virtual BOOL net_Spawn(CSE_Abstract* DC) { return TRUE; };

	virtual void net_Destroy()
	{
	};
	virtual void OnEvent(NET_Packet& P, u16 type);

	virtual void OnH_A_Chield();
	virtual void OnH_B_Chield();
	virtual void OnH_B_Independent(bool just_before_destroy);
	virtual void OnH_A_Independent();

	virtual void PlaySound(LPCSTR alias, const Fvector& position);
	virtual void PlaySound(LPCSTR alias, const Fvector& position, u8 index); //Alundaio: Play at index

	virtual bool Action(u16 cmd, u32 flags) { return false; }
	virtual void OnMovementChanged(ACTOR_DEFS::EMoveCommand cmd);

	virtual u8 GetCurrentHudOffsetIdx() { return 0; }

	BOOL GetHUDmode();
	void PlayBlendAnm(LPCSTR name, float speed = 1.f, float power = 1.f, bool stop_old = true);
	IC bool IsPending() const { return !!m_huditem_flags.test(fl_pending); }

	virtual void DeleteHudItemData();

	virtual bool ActivateItem();
	virtual void DeactivateItem();
	virtual void SendDeactivateItem();

	virtual void OnActiveItem()
	{
	};

	virtual void OnHiddenItem()
	{
	};
	virtual void SendHiddenItem(); //same as OnHiddenItem but for client... (sends message to a server)...
	virtual void OnMoveToRuck(const SInvItemPlace& prev);

	bool IsHidden() const { return GetState() == eHidden; } // Does weapon is in hidden state
	bool IsHiding() const { return GetState() == eHiding; }
	bool IsShowing() const { return GetState() == eShowing; }

	virtual void SwitchState(u32 S);
	virtual void OnStateSwitch(u32 S, u32 oldState);

	virtual void OnAnimationEnd(u32 state);

	virtual void OnMotionMark(u32 state, const motion_marks& M);

	virtual void PlayAnimIdle();
	virtual bool TryPlayAnimBore();
	virtual bool TryPlayAnimIdle();
	virtual bool MovingAnimAllowedNow() { return true; }

	virtual bool NeedBlendAnm();

	virtual void PlayAnimIdleMoving();
	virtual void PlayAnimIdleSprint();

	virtual void UpdateCL();
	virtual void renderable_Render();


	virtual void UpdateHudAdditional(Fmatrix& trans);


	virtual void UpdateXForm() = 0;

	u32 PlayHUDMotion(shared_str M, BOOL bMixIn, CHudItem* W, u32 state, float speed = 1.f, float end = 0.f, bool bMixIn2 = true);
	u32 PlayHUDMotion_noCB(const shared_str& M, BOOL bMixIn, float speed = 1.f, bool bMixIn2 = true);
	void StopCurrentAnimWithoutCallback();

	IC void RenderHud(BOOL B) { m_huditem_flags.set(fl_renderhud, B); }
	IC BOOL RenderHud() { return m_huditem_flags.test(fl_renderhud); }
	attachable_hud_item* HudItemData();
	bool IsAttachedToHUD();
	virtual bool ParentIsActor();
	virtual float GetNearWallRange();
	virtual float GetBaseHudFov();
	virtual float GetTargetHudFov();
	virtual float GetTargetNearWallOffset();
	void UpdateNearWall();
	float GetNearWallOffset();
	float GetHudFov();
	virtual void on_outfit_changed();
	virtual void on_a_hud_attach();
	virtual void on_b_hud_detach();
	IC BOOL HudInertionEnabled() const { return m_huditem_flags.test(fl_inertion_enable); }
	IC BOOL HudInertionAllowed() const { return m_huditem_flags.test(fl_inertion_allow); }
	virtual float GetInertionAimFactor() { return 1.f; }; //--#SM+#--
	virtual void render_hud_mode()
	{
	};
	virtual bool need_renderable() { return true; };

	virtual void render_item_3d_ui();

	virtual bool render_item_3d_ui_query() { return true; }

	virtual bool CheckCompatibility(CHudItem*) { return true; }
protected:

	IC void SetPending(bool H) { m_huditem_flags.set(fl_pending, H); }
	shared_str hud_sect;

	//êàäðû ìîìåíòà ïåðåñ÷åòà XFORM è FirePos
	u32 dwFP_Frame;
	u32 dwXF_Frame;

	IC void EnableHudInertion(BOOL B) { m_huditem_flags.set(fl_inertion_enable, B); }
	IC void AllowHudInertion(BOOL B) { m_huditem_flags.set(fl_inertion_allow, B); }

	u32 m_animation_slot;

	HUD_SOUND_COLLECTION_LAYERED m_sounds;

private:
	CPhysicItem* m_object;
	CInventoryItem* m_item;

public:
	const shared_str& HudSection() const { return hud_sect; }

	bool has_object() const
	{
		return m_object;
	}

	IC CPhysicItem& object() const
	{
		VERIFY(m_object);
		return (*m_object);
	}

	IC CInventoryItem& item() const
	{
		VERIFY(m_item);
		return (*m_item);
	}

	IC u32 animation_slot() { return m_animation_slot; }

	virtual void on_renderable_Render() = 0;

	virtual void debug_draw_firedeps()
	{
	};

	float m_hud_fov_add_mod;
	float m_nearwall_dist_max;
	float m_nearwall_dist_min;
	float m_nearwall_factor;
	float m_nearwall_target_hud_fov;
	float m_nearwall_speed_mod;
	float m_base_fov;
	float m_hud_fov;
	float m_nearwall_ofs;

	virtual CHudItem* cast_hud_item() { return this; }
	virtual bool PlayAnimCrouchIdleMoving(); //AVO: new crouch idle animation
	bool HudAnimationExist(LPCSTR anim_name);

private:
	SPickParam PP;

public:
	virtual void OnFrame();
	void net_Relcase(CObject* O);

	void ApplyAimModifiers(Fmatrix& matrix);
	virtual Fmatrix RayTransform();
	virtual void g_fireParams(SPickParam& pp) {};
	virtual void Ray(SPickParam& pp);
	void UpdatePick();
	SPickParam& GetPick() { return PP; };
	collide::rq_result& GetRQ() { return GetPick().result; };
	float GetRQVis() { return PP.power; };
};

class CAnonHudItem : public CHudItem
{
private:
	typedef CHudItem inherited;
public:
	CAnonHudItem();
	virtual ~CAnonHudItem();
	virtual void UpdateXForm();
	virtual void on_renderable_Render();
	virtual bool TryPlayAnimIdle() { return false; }
	virtual void UpdateHudAdditional(Fmatrix& trans) { }
	DECLARE_SCRIPT_REGISTER_FUNCTION
};
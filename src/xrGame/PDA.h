#pragma once

#include "../xrEngine/feel_touch.h"
#include "hud_item_object.h"

#include "InfoPortionDefs.h"
#include "character_info_defs.h"

#include "PdaMsg.h"

class CInventoryOwner;
class CPda;

DEF_VECTOR(PDA_LIST, CPda*);

class CPda :
	public CHudItemObject,
	public Feel::Touch
{
	typedef CHudItemObject inherited;
public:
	CPda();
	virtual ~CPda();

	virtual BOOL net_Spawn(CSE_Abstract* DC);
	virtual void Load(LPCSTR section);
	virtual void net_Destroy();

	virtual float GetNearWallOffset();
	virtual Fmatrix RayTransform();

	virtual void OnH_A_Chield();
	virtual void OnH_B_Independent(bool just_before_destroy);

	virtual void shedule_Update(u32 dt);

	virtual bool Action(u16 cmd, u32 flags);
	virtual void OnMovementChanged(ACTOR_DEFS::EMoveCommand cmd);

	virtual void feel_touch_new(CObject* O);
	virtual void feel_touch_delete(CObject* O);
	virtual bool feel_touch_contact(CObject* O);


	virtual u16 GetOriginalOwnerID() { return m_idOriginalOwner; }
	virtual CInventoryOwner* GetOriginalOwner();
	virtual CObject* GetOwnerObject();


	void TurnOn() { m_bTurnedOff = false; }
	void TurnOff() { m_bTurnedOff = true; }

	bool IsActive() { return IsOn(); }
	bool IsOn() { return !m_bTurnedOff; }
	bool IsOff() { return m_bTurnedOff; }


	void ActivePDAContacts(xr_vector<CPda*>& res);
	CPda* GetPdaFromOwner(CObject* owner);
	u32 ActiveContactsNum() { return m_active_contacts.size(); }
	void PlayScriptFunction();

	bool CanPlayScriptFunction()
	{
		if (!xr_strcmp(m_functor_str, "")) return false;
		return true;
	};


	virtual void save(NET_Packet& output_packet);
	virtual void load(IReader& input_packet);

protected:
	void UpdateActiveContacts();

	xr_vector<CObject*> m_active_contacts;
	float m_fRadius;

	u16 m_idOriginalOwner;
	shared_str m_SpecificChracterOwner;
	xr_string m_sFullName;

	bool m_bTurnedOff;
	shared_str m_functor_str;
	float m_fZoomfactor;
	float m_fDisplayBrightnessPowerSaving;
	float m_fPowerSavingCharge;
	bool bButtonL;
	bool bButtonR;
	LPCSTR m_joystick_bone;
	u16 joystick;
	float m_screen_on_delay, m_screen_off_delay;
	float target_screen_switch;
	float m_fLR_CameraFactor;
	float m_fLR_MovingFactor;
	float m_fLR_InertiaFactor;
	float m_fUD_InertiaFactor;
	bool hasEnoughBatteryPower(){ return (!IsUsingCondition() || (IsUsingCondition() && GetCondition() > m_fLowestBatteryCharge)); }
	static void _BCL JoystickCallback(CBoneInstance* B);
	bool m_bNoticedEmptyBattery;
	bool m_LastMBZoom;
public:
	virtual void OnStateSwitch(u32 S, u32 oldState);
	virtual void OnAnimationEnd(u32 state);
	virtual void UpdateHudAdditional(Fmatrix& trans);
	virtual void OnMoveToRuck(const SInvItemPlace& prev);
	virtual void UpdateCL();
	virtual void UpdateXForm();
	virtual void OnActiveItem();
	virtual void OnHiddenItem();
	Fvector m_hud_offset[2];

	enum eDeferredEnableState
	{
		eDefault,
		eDisable,
		eEnable,
		eEnableZoomed
	};

	enum ePDAState
	{
		eEmptyBattery = 7
	};

	bool m_bZoomed;
	eDeferredEnableState m_eDeferredEnable;
	bool m_bPowerSaving;
	float m_psy_factor;
	float m_thumb_rot[2];
	float m_nearwall_zoomed_range;
};

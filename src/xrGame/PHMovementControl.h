#pragma once
#ifndef CPHMOVEMENT_CONTROL_H
#define CPHMOVEMENT_CONTROL_H

#include "../xrphysics/physicsexternalcommon.h"
#include "../xrphysics/mathutils.h"
#include "../xrphysics/movementboxdynamicactivate.h"

namespace ALife
{
	enum EHitType;
};

namespace DetailPathManager
{
	struct STravelPathPoint;
};

class CPHAICharacter;
class CPHSimpleCharacter;
class CPHSynchronize;
class ICollisionDamageInfo;
class IElevatorState;
struct CPHCaptureBoneCallback;
class CPhysicsShellHolder;
class IPhysicsShellHolder;
class CPHCapture;
class IPHCapture;
class CPHCharacter;
class IPhysicsElement;


class CPHMovementControl : public IPHMovementControl
{
	collide::rq_results storage;
	static const int path_few_point = 10;

public:
	CObject* ParentObject() { return pObject; }
	IElevatorState* ElevatorState();
	void in_shedule_Update(u32 DT);
	void PHCaptureObject(CPhysicsShellHolder* object, CPHCaptureBoneCallback* cb = 0);
	void PHCaptureObject(CPhysicsShellHolder* object, u16 element);
	IPHCapture* PHCapture();
	CPHCharacter* PHCharacter() { return m_character; }
	const CPHCharacter* PHCharacter() const { return m_character; }
	const IPhysicsElement* IElement() const;
	void PHReleaseObject();
	Fvector PHCaptureGetNearestElemPos(const CPhysicsShellHolder* object);
	Fmatrix PHCaptureGetNearestElemTransform(CPhysicsShellHolder* object);
	void SetMaterial(u16 material);
	void SetAirControlParam(float param) { fAirControlParam = param; }
	void SetActorRestrictorRadius(ERestrictionType rt, float r);
	void SetRestrictionType(ERestrictionType rt);
	void SetActorMovable(bool v);
	void SetForcedPhysicsControl(bool v);
	bool ForcedPhysicsControl();
	void UpdateObjectBox(CPHCharacter* ach);
	void VirtualMoveTo(const Fvector& in_pos, Fvector& out_pos);
	void BlockDamageSet(u64 steps_num);

	enum JumpType
	{
		jtStrait,
		//end point before uppermost point
		jtCurved,
		//end point after uppermost point
		jtHigh //end point is uppermost point
	};

	void JumpV(const Fvector& jump_velocity);
	void Jump(const Fvector& start_point, const Fvector& end_point, float time);
	void Jump(const Fvector& end_point, float time);
	float Jump(const Fvector& end_point);
	bool JumpState();
	///
	bool PhysicsOnlyMode();
	void GetJumpMinVelParam(Fvector& min_vel, float& time, JumpType& type, const Fvector& end_point);
	//returns vector of velocity of jump with minimal start speed
	//in min_vel and correspondent jump time in time
	float JumpMinVelTime(const Fvector& end_point); // return time of jump with min start speed
	// input: end_point and time; return velocity and type of jump
	void GetJumpParam(Fvector& velocity, JumpType& type, const Fvector& end_point, float time);
	bool b_exect_position;
	int in_dead_area_count;

public:
	enum EEnvironment
	{
		peOnGround,
		peAtWall,
		peInAir
	};

	enum CharacterType
	{
		actor,
		ai
	};

	bool isOutBorder() { return in_dead_area_count > 0; }
	void setOutBorder() { in_dead_area_count = 1; }

private:
	void TraceBorder(const Fvector& previous_position);
	void CheckEnvironment(const Fvector& V);

	CharacterType eCharacterType;
	CPHCharacter* m_character;
	IPHCapture* m_capture;

	float m_fGroundDelayFactor;
	BOOL bIsAffectedByGravity;
	//------------------------------
	CObject* pObject;
	EEnvironment eOldEnvironment;
	EEnvironment eEnvironment;
	Fbox aabb;
	Fbox boxes [4];

	u32 trying_times[4];
	Fvector trying_poses[4];
	u64 block_damage_step_end;
	DWORD m_dwCurBox;

	float fMass;
	float fMinCrashSpeed;
	float fMaxCrashSpeed;
	float fCollisionDamageFactor;
	float fAirControlParam;

	Fvector vVelocity;
	Fvector vPosition;

	Fvector vPathPoint;
	Fvector _vPathDir;
	int m_path_size;
	int m_start_index;

	float m_path_distance;
	float fLastMotionMag;

	float fActualVelocity;
	float fContactSpeed;
	float fLastUpdateTime;

public:
	Fvector vExternalImpulse;
	bool bExernalImpulse;
	BOOL bSleep;
	bool bNonInteractiveMode;
	BOOL gcontact_Was; // Приземление
	float gcontact_Power; // Насколько сильно ударились
	float gcontact_HealthLost; // Скоко здоровья потеряли

public:
	void AllocateCharacterObject(CharacterType type);
	void DeleteCharacterObject();
	void CreateCharacter();
	void DestroyCharacter();
	void Load(LPCSTR section);
#ifdef DEBUG
	void dbg_Draw();
#endif

	void SetPLastMaterialIDX(u16* p);
	const Fvector& GetVelocity() { return vVelocity; }
	const Fvector& GetPathDir() { return _vPathDir; }
	void SetPathDir(const Fvector& v);

	void GetCharacterVelocity(Fvector& velocity);
	float GetVelocityMagnitude() { return vVelocity.magnitude(); }
	float GetVelocityActual() { return fActualVelocity; }
	float GetXZVelocityActual() { return dXZMag(vVelocity); }
	float GetActVelProj(const Fvector& dir) { return vVelocity.dotproduct(dir); }

	float GetActVelInGoingDir()
	{
		float r = GetActVelProj(GetPathDir());
		return r > 0.f ? r : 0.f;
	}

	float GetXZActVelInGoingDir()
	{
		float r = dXZDot(GetPathDir(), vVelocity);
		return r > EPS_L ? r : 0.f;
	}

	void GetSmoothedVelocity(Fvector& v);
	float GetContactSpeed() { return fContactSpeed; }
	void GroundNormal(Fvector& norm);
	CPHSynchronize* GetSyncItem();
	void Freeze();
	void UnFreeze();
	void SetVelocity(float x, float y, float z) { SetVelocity(Fvector().set(x, y, z)); }

	void SetVelocity(const Fvector& v)
	{
		vVelocity.set(v);
		SetCharacterVelocity(v);
	}

	void SetCharacterVelocity(const Fvector& v);
	void SetPhysicsRefObject(CPhysicsShellHolder* ref_object);
	IPhysicsShellHolder* PhysicsRefObject();
	void SetNonInteractive(bool v);

	void CalcMaximumVelocity(Fvector& /**dest/**/, Fvector& /**accel/**/, float /**friction/**/)
	{};

	void CalcMaximumVelocity(float& /**dest/**/, float /**accel/**/, float /**friction/**/)
	{};

	void ActivateBox(DWORD id, BOOL Check = false);
	bool ActivateBoxDynamic(DWORD id, int num_it = 9, int num_steps = 5, float resolve_depth = 0.01f);
	void InterpolateBox(DWORD id, float k);
	EEnvironment Environment() { return eEnvironment; }
	EEnvironment OldEnvironment() { return eOldEnvironment; }
	const Fbox& Box() { return aabb; }
	DWORD BoxID() const { return m_dwCurBox; }
	const Fbox* Boxes() { return boxes; }
	float FootRadius();
	void CollisionEnable(BOOL enable);

	void SetBox(DWORD id, const Fbox& BB)
	{
		boxes[id].set(BB);
		aabb.set(BB);
	}

	void SetMass(float M);

	float GetMass() { return fMass; }

	void SetCrashSpeeds(float min, float max)
	{
		fMinCrashSpeed = min;
		fMaxCrashSpeed = max;
	}

	void SetPosition(const Fvector& P);
	void GetPosition(Fvector& P);
	void GetCharacterPosition(Fvector& P);
	void InterpolatePosition(Fvector& P);
	bool TryPosition(Fvector& pos);
	bool IsCharacterEnabled();
	void DisableCharacter();
	void Calculate(Fvector& vAccel, const Fvector& camDir, float ang_speed, float jump, float dt, bool bLight);
	void Calculate(const xr_vector<DetailPathManager::STravelPathPoint>& path, float speed, u32& travel_point, float& precesition);
	void AddControlVel(const Fvector& vel);
	void SetVelocityLimit(float val);
	float VelocityLimit();
	void PathNearestPoint(const xr_vector<DetailPathManager::STravelPathPoint>& path, const Fvector& new_position, int& index, bool& type);
	void PathNearestPointFindUp(const xr_vector<DetailPathManager::STravelPathPoint>& path, const Fvector& new_position, int& index, float radius, bool& near_line);
	void PathNearestPointFindDown(const xr_vector<DetailPathManager::STravelPathPoint>& path, const Fvector& new_position, int& index, float radius, bool& near_line);
	void PathDIrPoint(const xr_vector<DetailPathManager::STravelPathPoint>& path, int index, float distance, float precesition, Fvector& dir);
	void PathDIrLine(const xr_vector<DetailPathManager::STravelPathPoint>& path, int index, float distance, float precesition, Fvector& dir);
	void CorrectPathDir(const Fvector& real_path_dir, const xr_vector<DetailPathManager::STravelPathPoint>& path, int index, Fvector& corrected_path_dir);

	void SetApplyGravity(BOOL flag);
	void GetDeathPosition(Fvector& pos);
	void SetEnvironment(int enviroment, int old_enviroment);
	void SetFrictionFactor(float f);
	float GetFrictionFactor();
	void MulFrictionFactor(float f);
	void ApplyImpulse(const Fvector& dir, const float P);
	void ApplyHit(const Fvector& dir, const float P, ALife::EHitType hit_type);
	void SetJumpUpVelocity(float velocity);
	void EnableCharacter();
	void SetOjectContactCallback(ObjectContactCallbackFun* callback);
	void SetFootCallBack(ObjectContactCallbackFun* callback);
	static BOOL CPHMovementControl::BorderTraceCallback(collide::rq_result& result, LPVOID params);
	ObjectContactCallbackFun* ObjectContactCallback();
	u16 ContactBone();
	const ICollisionDamageInfo* CollisionDamageInfo() const;
	ICollisionDamageInfo* CollisionDamageInfo();
	void GetDesiredPos(Fvector& dpos);
	bool CharacterExist() const;
	void update_last_material();
	u16 injurious_material_idx();
	CPHMovementControl(CObject* parent);
	~CPHMovementControl(void);

	CPHCharacter* character() { return m_character; };
	void NetRelcase(CObject* O);

private:
	void actor_calculate(Fvector& vAccel, const Fvector& camDir, float ang_speed, float jump, float dt, bool bLight);
	void UpdateCollisionDamage();
	bool MakeJumpPath(xr_vector<DetailPathManager::STravelPathPoint>& out_path, u32& travel_point, Fvector& dist_to_enemy);
};

#endif

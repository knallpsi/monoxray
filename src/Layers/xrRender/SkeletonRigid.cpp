//---------------------------------------------------------------------------
#include 	"stdafx.h"
#pragma hdrstop

#include 	"SkeletonCustom.h"

extern int psSkeletonUpdate;

#ifdef DEBUG
void check_kinematics(CKinematics* _k, LPCSTR s);
#endif

extern float IK_CALC_DIST;
extern float IK_ALWAYS_CALC_DIST;
extern ENGINE_API BOOL g_bootComplete;
BOOL r_optimize_calculate_bones = TRUE;

class IRenderable;

void CKinematics::CalculateBones(BOOL bForceExact)
{
	// early out.
	// check if the info is still relevant
	// skip all the computations - assume nothing changes in a small period of time :)
	if (RDEVICE.dwTimeGlobal == UCalc_Time) return; // early out for "fast" update

	// demonized: reduce calculate bones updates when the object is far away and not in frustum
	// Available only if can get parent xform
	// Refactor later for per object basis
	float update_rate_k = 1.f;
	if (g_bootComplete)
	{
		if (auto xForm = getXForm())
		{
			Fvector p;
			xForm.value().transform_tiny(p, vis.sphere.P);

			// Perceivable distance depending on FOV, so that objects will behave normal in binoculars
			float dist = Device.vCameraPosition.distance_to(p);
			float fov_rad = deg2rad(Device.fFOV); // Make sure Device.fFOV is in degrees
			float perceived_dist = dist / tanf(fov_rad * 0.5f);
			float dist_k = perceived_dist / dist;
			update_rate_k = _max(1.f, dist / (IK_CALC_DIST * dist_k));

			// Visibility check, perform always
			bool visibleCheck = (dist < IK_ALWAYS_CALC_DIST * dist_k) || ::Render->ViewBase.testSphere_dirty(p, vis.sphere.R);
			if (!visibleCheck)
			{
				bForceExact = FALSE;
				update_rate_k = _max(2.f, update_rate_k);

				/*if (RDEVICE.dwTimeGlobal % 100 < 10)
				{
					Msg("CKinematics::CalculateBones, object out of frustum, dist %.2f, update_rate_k %.2f", dist / dist_k, update_rate_k);
				}*/
			}

			// distance check, perform when cvar is enabled and can be optimized
			if (r_optimize_calculate_bones && canBeOptimized() && (dist > IK_CALC_DIST * dist_k))
			{
				bForceExact = FALSE;

				/*if (RDEVICE.dwTimeGlobal % 100 < 10)
				{
					Msg("CKinematics::CalculateBones, object canBeOptimized and dist > IK_CALC_DIST * dist_k, dist %.2f, update_rate_k %.2f", dist / dist_k, update_rate_k);
				}*/
			}
		}
	}
	

	UCalc_mtlock lock;
	OnCalculateBones();
	if (!bForceExact && (RDEVICE.dwTimeGlobal < (UCalc_Time + UCalc_Interval * update_rate_k))) return; // early out for "slow" update
	if (Update_Visibility) Visibility_Update();

	_DBG_SINGLE_USE_MARKER;
	// here we have either:
	//	1:	timeout elapsed
	//	2:	exact computation required
	UCalc_Time = RDEVICE.dwTimeGlobal;

	// exact computation
	// Calculate bones
#ifdef DEBUG
	RDEVICE.Statistic->Animation.Begin();
#endif

	Bone_Calculate(bones->at(iRoot), &Fidentity);
#ifdef DEBUG
	check_kinematics				(this, dbg_name.c_str() );
	RDEVICE.Statistic->Animation.End	();
#endif
	VERIFY(LL_GetBonesVisible()!=0);
	// Calculate BOXes/Spheres if needed
	UCalc_Visibox++;
	if (UCalc_Visibox >= psSkeletonUpdate)
	{
		// mark
		UCalc_Visibox = -(::Random.randI(psSkeletonUpdate - 1));
		UCalc_ThisFrame = true;

		// the update itself
		Fbox Box;
		Box.invalidate();
		for (u32 b = 0; b < bones->size(); b++)
		{
			if (!LL_GetBoneVisible(u16(b))) continue;
			Fobb& obb = (*bones)[b]->obb;
			Fmatrix& Mbone = bone_instances[b].mTransform;
			Fmatrix Mbox;
			obb.xform_get(Mbox);
			Fmatrix X;
			X.mul_43(Mbone, Mbox);
			Fvector& S = obb.m_halfsize;

			Fvector P, A;
			A.set(-S.x, -S.y, -S.z);
			X.transform_tiny(P, A);
			Box.modify(P);
			A.set(-S.x, -S.y, S.z);
			X.transform_tiny(P, A);
			Box.modify(P);
			A.set(S.x, -S.y, S.z);
			X.transform_tiny(P, A);
			Box.modify(P);
			A.set(S.x, -S.y, -S.z);
			X.transform_tiny(P, A);
			Box.modify(P);
			A.set(-S.x, S.y, -S.z);
			X.transform_tiny(P, A);
			Box.modify(P);
			A.set(-S.x, S.y, S.z);
			X.transform_tiny(P, A);
			Box.modify(P);
			A.set(S.x, S.y, S.z);
			X.transform_tiny(P, A);
			Box.modify(P);
			A.set(S.x, S.y, -S.z);
			X.transform_tiny(P, A);
			Box.modify(P);
		}
		if (bones->size())
		{
			// previous frame we have updated box - update sphere
			vis.box.min = (Box.min);
			vis.box.max = (Box.max);
			vis.box.getsphere(vis.sphere.P, vis.sphere.R);
		}
#ifdef DEBUG
		// Validate
		VERIFY3	(_valid(vis.box.min)&&_valid(vis.box.max),	"Invalid bones-xform in model", dbg_name.c_str());
		if(vis.sphere.R>1000.f)
		{
			for(u16 ii=0; ii<LL_BoneCount();++ii){
				Fmatrix tr;
				tr = LL_GetTransform(ii);
				Log("bone ",LL_BoneName_dbg(ii));
				Log("bone_matrix",tr);
			}
			Log("end-------");
		}
		VERIFY3	(vis.sphere.R<1000.f,						"Invalid bones-xform in model", dbg_name.c_str());
#endif
	}
	else
		UCalc_ThisFrame = false;
	//
	if (Update_Callback) Update_Callback(this);
}

#ifdef DEBUG
void check_kinematics(CKinematics* _k, LPCSTR s)
{
	CKinematics* K = _k;
	Fmatrix&	MrootBone		= K->LL_GetBoneInstance(K->LL_GetBoneRoot()).mTransform;
	if(MrootBone.c.y >10000)
	{	
		Msg("all bones transform:--------[%s]",s);
		
		for(u16 ii=0; ii<K->LL_BoneCount();++ii){
			Fmatrix tr;

			tr = K->LL_GetTransform(ii);
			Log("bone ",K->LL_BoneName_dbg(ii));
			Log("bone_matrix",tr);
		}
		Log("end-------");
		VERIFY3(0,"check_kinematics failed for ", s);
	}
}
#endif

void CKinematics::BuildBoneMatrix(const CBoneData* bd, CBoneInstance& bi, const Fmatrix* parent,
                                  u8 channel_mask/* = (1<<0)*/)
{
	if (LL_GetBoneVisible(bd->GetSelfID()))
	{
		bi.mTransform.mul_43(*parent, bd->bind_transform);
		bi.mTransformHidden.set(bi.mTransform);
	}
	else
	{
		bi.mTransform.c = (*parent).c;
		bi.mTransformHidden.mul_43(*parent, bd->bind_transform);
	}
}

void CKinematics::CLBone(const CBoneData* bd, CBoneInstance& bi, const Fmatrix* parent, u8 channel_mask /*= (1<<0)*/)
{
	if (!bi.callback_overwrite())
		BuildBoneMatrix(bd, bi, parent, channel_mask);

	if (bi.callback())
		bi.callback()(&bi);

	bi.mRenderTransform.mul_43(bi.mTransform, bd->m2b_transform);
}

void CKinematics::Bone_GetAnimPos(Fmatrix& pos, u16 id, u8 mask_channel, bool ignore_callbacks)
{
	R_ASSERT(id<LL_BoneCount());
	CBoneInstance bi = LL_GetBoneInstance(id);
	BoneChain_Calculate(&LL_GetData(id), bi, mask_channel, ignore_callbacks);
#ifndef MASTER_GOLD
	R_ASSERT( _valid( bi.mTransform ) );
#endif
	pos.set(bi.mTransform);
}

void CKinematics::Bone_Calculate(CBoneData* bd, Fmatrix* parent)
{
	u16 SelfID = bd->GetSelfID();
	CBoneInstance& BONE_INST = LL_GetBoneInstance(SelfID);
	CLBone(bd, BONE_INST, parent, u8(-1));
	// Calculate children
	for (xr_vector<CBoneData*>::iterator C = bd->children.begin(); C != bd->children.end(); C++)
		Bone_Calculate(*C, &BONE_INST.mTransform);
}

void CKinematics::BoneChain_Calculate(const CBoneData* bd, CBoneInstance& bi, u8 mask_channel, bool ignore_callbacks)
{
	u16 SelfID = bd->GetSelfID();
	//CBlendInstance& BLEND_INST	= LL_GetBlendInstance(SelfID);
	//CBlendInstance::BlendSVec &Blend = BLEND_INST.blend_vector();
	//ignore callbacks
	BoneCallback bc = bi.callback();
	BOOL ow = bi.callback_overwrite();
	if (ignore_callbacks)
	{
		bi.set_callback(bi.callback_type(), 0, bi.callback_param(), 0);
	}
	if (SelfID == LL_GetBoneRoot())
	{
		CLBone(bd, bi, &Fidentity, mask_channel);
		//restore callback	
		bi.set_callback(bi.callback_type(), bc, bi.callback_param(), ow);
		return;
	}
	u16 ParentID = bd->GetParentID();
	R_ASSERT(ParentID != BI_NONE);
	CBoneData* ParrentDT = &LL_GetData(ParentID);
	CBoneInstance parrent_bi = LL_GetBoneInstance(ParentID);
	BoneChain_Calculate(ParrentDT, parrent_bi, mask_channel, ignore_callbacks);
	CLBone(bd, bi, &parrent_bi.mTransform, mask_channel);
	//restore callback
	bi.set_callback(bi.callback_type(), bc, bi.callback_param(), ow);
}

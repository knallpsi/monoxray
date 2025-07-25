#include "stdafx.h"
#pragma hdrstop

#include "../../xrEngine/igame_persistent.h"
#include "../../xrEngine/igame_level.h"
#include "../../xrEngine/environment.h"
#include "../../xrEngine/fmesh.h"

#include "../../build_config_defines.h"

#include "ftreevisual.h"

shared_str m_xform;
shared_str m_xform_v;
shared_str c_consts;
shared_str c_wave;
shared_str c_wind;
shared_str c_c_bias;
shared_str c_c_scale;
shared_str c_c_sun;

shared_str c_prev_wave;
shared_str c_prev_wind;

shared_str c_c_PrevBendersPos;
shared_str c_c_BendersPos;
shared_str c_c_BendersSetup;

FTreeVisual::FTreeVisual(void)
{
}

FTreeVisual::~FTreeVisual(void)
{
}

void FTreeVisual::Release()
{
	dxRender_Visual::Release();
}

void FTreeVisual::Load(const char* N, IReader* data, u32 dwFlags)
{
	dxRender_Visual::Load(N, data, dwFlags);

	D3DVERTEXELEMENT9* vFormat = NULL;

	// read vertices
	R_ASSERT(data->find_chunk(OGF_GCONTAINER));
	{
		// verts
		u32 ID = data->r_u32();
		vBase = data->r_u32();
		vCount = data->r_u32();
		vFormat = RImplementation.getVB_Format(ID);

		VERIFY(NULL==p_rm_Vertices);

		p_rm_Vertices = RImplementation.getVB(ID);
		p_rm_Vertices->AddRef();

		// indices
		dwPrimitives = 0;
		ID = data->r_u32();
		iBase = data->r_u32();
		iCount = data->r_u32();
		dwPrimitives = iCount / 3;

		VERIFY(NULL==p_rm_Indices);
		p_rm_Indices = RImplementation.getIB(ID);
		p_rm_Indices->AddRef();
	}

	// load tree-def
	R_ASSERT(data->find_chunk(OGF_TREEDEF2));
	{
		data->r(&xform, sizeof(xform));
		data->r(&c_scale, sizeof(c_scale));
		c_scale.rgb.mul(.5f);
		c_scale.hemi *= .5f;
		c_scale.sun *= .5f;
		data->r(&c_bias, sizeof(c_bias));
		c_bias.rgb.mul(.5f);
		c_bias.hemi *= .5f;
		c_bias.sun *= .5f;
		//Msg				("hemi[%f / %f], sun[%f / %f]",c_scale.hemi,c_bias.hemi,c_scale.sun,c_bias.sun);
	}

	// Geom
	rm_geom.create(vFormat, p_rm_Vertices, p_rm_Indices);

	// Get constants
	m_xform = "m_xform";
	m_xform_v = "m_xform_v";
	c_consts = "consts";
	c_wave = "wave";
	c_wind = "wind";
	c_c_bias = "c_bias";
	c_c_scale = "c_scale";
	c_c_sun = "c_sun";

	c_prev_wave = "prev_wave";
	c_prev_wind = "prev_wind";

	c_c_PrevBendersPos = "benders_prevpos";
	c_c_BendersPos = "benders_pos";
	c_c_BendersSetup = "benders_setup";
}

struct FTreeVisual_setup
{
	u32 dwFrame;
	float scale;
	Fvector4 wave;
	Fvector4 wind;

	FTreeVisual_setup()
	{
		dwFrame = 0;
	}

	void calculate()
	{
		dwFrame = Device.dwFrame;

		// Calc wind-vector3, scale
		float tm_rot = PI_MUL_2 * Device.fTimeGlobal / ps_r__Tree_w_rot;

#ifdef TREE_WIND_EFFECT
		CEnvDescriptor& E = *g_pGamePersistent->Environment().CurrentEnv;
		float fValue = E.m_fTreeAmplitudeIntensity;
		wind.set(_sin(tm_rot), 0, _cos(tm_rot), 0);
		wind.normalize();
#if RENDER!=R_R1
		wind.mul(fValue); // dir1*amplitude
#else // R1
		wind.mul(ps_r__Tree_w_amp); // dir1*amplitude
#endif //-RENDER!=R_R1
#else //!TREE_WIND_EFFECT
		wind.set				(_sin(tm_rot),0,_cos(tm_rot),0);	wind.normalize	();	wind.mul(ps_r__Tree_w_amp);	// dir1*amplitude
#endif //-TREE_WIND_EFFECT
		scale = 1.f / float(FTreeVisual_quant);

		// setup constants
		wave.set(ps_r__Tree_Wave.x, ps_r__Tree_Wave.y, ps_r__Tree_Wave.z,
		         Device.fTimeGlobal * ps_r__Tree_w_speed); // wave
		wave.div(PI_MUL_2);
	}
};

void FTreeVisual::Render(float LOD)
{
	static FTreeVisual_setup tvs, prev_tvs;
	if (tvs.dwFrame != Device.dwFrame)
	{
		prev_tvs = tvs; // Save previous frame calculations
		tvs.calculate();
	}
	// setup constants
#if RENDER!=R_R1
	Fmatrix xform_v;
	xform_v.mul_43(RCache.get_xform_view(), xform);
	RCache.tree.set_m_xform_v(xform_v); // matrix
#endif
	float s = ps_r__Tree_SBC;
	RCache.tree.set_m_xform(xform); // matrix
	RCache.tree.set_consts(tvs.scale, tvs.scale, 0, 0); // consts/scale
	RCache.tree.set_wave(tvs.wave); // wave
	RCache.tree.set_wind(tvs.wind); // wind

	RCache.set_c(c_prev_wave, prev_tvs.wave);
	RCache.set_c(c_prev_wind, prev_tvs.wind);

#if RENDER!=R_R1
	s *= 1.3333f;
	RCache.tree.set_c_scale(s * c_scale.rgb.x, s * c_scale.rgb.y, s * c_scale.rgb.z, s * c_scale.hemi); // scale
	RCache.tree.set_c_bias(s * c_bias.rgb.x, s * c_bias.rgb.y, s * c_bias.rgb.z, s * c_bias.hemi); // bias
#else
	CEnvDescriptor& desc = *g_pGamePersistent->Environment().CurrentEnv;
	RCache.tree.set_c_scale(s * c_scale.rgb.x, s * c_scale.rgb.y, s * c_scale.rgb.z, s * c_scale.hemi); // scale
	RCache.tree.set_c_bias(s * c_bias.rgb.x + desc.ambient.x, s * c_bias.rgb.y + desc.ambient.y,
	                       s * c_bias.rgb.z + desc.ambient.z, s * c_bias.hemi); // bias
#endif
	RCache.tree.set_c_sun(s * c_scale.sun, s * c_bias.sun, 0, 0); // sun

#if RENDER==R_R4 || RENDER==R_R3

	if (ps_ssfx_grass_interactive.y > 0)
	{
		// Inter grass Settings
		RCache.set_c(c_c_BendersSetup, ps_ssfx_int_grass_params_1);

		// Grass benders data ( Player + Characters )
		IGame_Persistent::grass_data& GData = g_pGamePersistent->grass_shader_data;
		Fvector4 player_pos = { 0, 0, 0, 0 };
		int BendersQty = _min(16, ps_ssfx_grass_interactive.y + 1);

		// Add Player?
		if (ps_ssfx_grass_interactive.x > 0)
		{
			player_pos.set(Device.vCameraPosition.x, Device.vCameraPosition.y, Device.vCameraPosition.z, -1);
		}

		Fvector4* c_grass;
		{
			void* GrassData;
			RCache.get_ConstantDirect(c_c_BendersPos, BendersQty * sizeof(Fvector4) * 2, &GrassData, 0, 0);

			c_grass = (Fvector4*)GrassData;
		}
		VERIFY(c_grass);

		if (c_grass)
		{
			c_grass[0].set(player_pos);
			c_grass[16].set(0.0f, -99.0f, 0.0f, 1.0f);

			for (int Bend = 1; Bend < BendersQty; Bend++)
			{
				c_grass[Bend].set(GData.pos[Bend].x, GData.pos[Bend].y, GData.pos[Bend].z, GData.radius_curr[Bend]);
				c_grass[Bend + 16].set(GData.dir[Bend].x, GData.dir[Bend].y, GData.dir[Bend].z, GData.str[Bend]);
			}
		}

		Fvector4* c_prevgrass;
		{
			void* prevGrassData;
			RCache.get_ConstantDirect(c_c_PrevBendersPos, BendersQty * sizeof(Fvector4) * 2, &prevGrassData, 0, 0);

			c_prevgrass = (Fvector4*)prevGrassData;
		}
		VERIFY(c_prevgrass);

		if (c_prevgrass)
		{
			for (int Bend = 0; Bend < BendersQty; Bend++)
			{
				c_prevgrass[Bend].set(GData.prev_pos[Bend]);
				c_prevgrass[Bend + 16].set(GData.prev_dir[Bend]);
			}
		}
	}
#endif
}

#define PCOPY(a)	a = pFrom->a

void FTreeVisual::Copy(dxRender_Visual* pSrc)
{
	dxRender_Visual::Copy(pSrc);

	FTreeVisual* pFrom = dynamic_cast<FTreeVisual*>(pSrc);

	PCOPY(rm_geom);

	PCOPY(p_rm_Vertices);
	if (p_rm_Vertices) p_rm_Vertices->AddRef();

	PCOPY(vBase);
	PCOPY(vCount);

	PCOPY(p_rm_Indices);
	if (p_rm_Indices) p_rm_Indices->AddRef();

	PCOPY(iBase);
	PCOPY(iCount);
	PCOPY(dwPrimitives);

	PCOPY(xform);
	PCOPY(c_scale);
	PCOPY(c_bias);
}

//-----------------------------------------------------------------------------------
// Stripified Tree
//-----------------------------------------------------------------------------------
FTreeVisual_ST::FTreeVisual_ST(void)
{
}

FTreeVisual_ST::~FTreeVisual_ST(void)
{
}

void FTreeVisual_ST::Release()
{
	inherited::Release();
}

void FTreeVisual_ST::Load(const char* N, IReader* data, u32 dwFlags)
{
	inherited::Load(N, data, dwFlags);
}

void FTreeVisual_ST::Render(float LOD)
{
	inherited::Render(LOD);
	RCache.set_Geometry(rm_geom);
	RCache.Render(D3DPT_TRIANGLELIST, vBase, 0, vCount, iBase, dwPrimitives);
	RCache.stat.r.s_flora.add(vCount);
}

void FTreeVisual_ST::Copy(dxRender_Visual* pSrc)
{
	inherited::Copy(pSrc);
}

//-----------------------------------------------------------------------------------
// Progressive Tree
//-----------------------------------------------------------------------------------
FTreeVisual_PM::FTreeVisual_PM(void)
{
	pSWI = 0;
	last_lod = 0;
}

FTreeVisual_PM::~FTreeVisual_PM(void)
{
}

void FTreeVisual_PM::Release()
{
	inherited::Release();
}

void FTreeVisual_PM::Load(const char* N, IReader* data, u32 dwFlags)
{
	inherited::Load(N, data, dwFlags);
	R_ASSERT(data->find_chunk(OGF_SWICONTAINER));
	{
		u32 ID = data->r_u32();
		pSWI = RImplementation.getSWI(ID);
	}
}

void FTreeVisual_PM::Render(float LOD)
{
	inherited::Render(LOD);
	int lod_id = last_lod;
	if (LOD >= 0.f)
	{
		lod_id = iFloor((1.f - LOD) * float(pSWI->count - 1) + 0.5f);
		last_lod = lod_id;
	}
	VERIFY(lod_id>=0 && lod_id<int(pSWI->count));
	FSlideWindow& SW = pSWI->sw[lod_id];
	RCache.set_Geometry(rm_geom);
	RCache.Render(D3DPT_TRIANGLELIST, vBase, 0, SW.num_verts, iBase + SW.offset, SW.num_tris);
	RCache.stat.r.s_flora.add(SW.num_verts);
}

void FTreeVisual_PM::Copy(dxRender_Visual* pSrc)
{
	inherited::Copy(pSrc);
	FTreeVisual_PM* pFrom = dynamic_cast<FTreeVisual_PM*>(pSrc);
	PCOPY(pSWI);
}

#include "stdafx.h"
#pragma hdrstop

#include "Blender_light_direct.h"

CBlender_accum_direct::CBlender_accum_direct() { description.CLS = 0; }

CBlender_accum_direct::~CBlender_accum_direct()
{
}

void CBlender_accum_direct::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	//	BOOL	b_HW_smap		= RImplementation.o.HW_smap;
	//	BOOL	b_HW_PCF		= RImplementation.o.HW_smap_PCF;
	BOOL blend = FALSE; //RImplementation.o.fp16_blend;
	D3DBLEND dest = blend ? D3DBLEND_ONE : D3DBLEND_ZERO;
	if (RImplementation.o.sunfilter)
	{
		blend = FALSE;
		dest = D3DBLEND_ZERO;
	}

	switch (C.iElement)
	{
	case SE_SUN_NEAR: // near pass - enable Z-test to perform depth-clipping
	case SE_SUN_MIDDLE: // middle pass - enable Z-test to perform depth-clipping
		//	FVF::TL2uv
		C.r_Pass("accum_sun", "accum_sun_near_nomsaa_nominmax", false, TRUE, FALSE, blend, D3DBLEND_ONE, dest);

		C.r_CullMode(D3DCULL_NONE);
		C.PassSET_ZB(TRUE, FALSE, TRUE); // force inverted Z-Buffer

		C.r_dx10Texture("s_position", r2_RT_P);
		C.r_dx10Texture("s_diffuse", r2_RT_albedo);

		C.r_dx10Texture("s_material", r2_material);
		C.r_dx10Texture("s_accumulator", r2_RT_accum);
		C.r_dx10Texture("s_lmap", r2_sunmask);
		C.r_dx10Texture("s_smap", r2_RT_smap_depth);
		C.r_dx10Texture("s_smap_minmax", r2_RT_smap_depth_minmax);

		//C.r_dx10Texture("s_hud_mask", r2_RT_ssfx_hud);
		C.r_dx10Texture("s_ssfx_sss", r2_RT_ssfx_sss);

		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_material");
		C.r_dx10Sampler("smp_linear");
		jitter(C);
		C.r_dx10Sampler("smp_smap");

		C.r_End();
		break;
	case SE_SUN_FAR: // far pass, only stencil clipping performed
		//	FVF::TL2uv
		//C.r_Pass			("null",			"accum_sun_far",	false,	TRUE,	FALSE,blend,D3DBLEND_ONE,dest);
		C.r_Pass("accum_sun", "accum_sun_far_nomsaa", false, TRUE, FALSE, blend, D3DBLEND_ONE, dest);
		C.r_CullMode(D3DCULL_NONE);

		C.r_dx10Texture("s_position", r2_RT_P);
		C.r_dx10Texture("s_diffuse", r2_RT_albedo);

		C.r_dx10Texture("s_material", r2_material);
		C.r_dx10Texture("s_accumulator", r2_RT_accum);
		C.r_dx10Texture("s_lmap", r2_sunmask);
		C.r_dx10Texture("s_smap", r2_RT_smap_depth);

		//C.r_dx10Texture("s_hud_mask", r2_RT_ssfx_hud);
		C.r_dx10Texture("s_ssfx_sss", r2_RT_ssfx_sss);

		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_material");
		C.r_dx10Sampler("smp_linear");
		jitter(C);
		{
			u32 s = C.r_dx10Sampler("smp_smap");
			if (s != u32(-1)) {
				C.i_dx10Address(s, D3DTADDRESS_BORDER);
				C.i_dx10BorderColor(s, D3DCOLOR_ARGB(255, 255, 255, 255));
			}
		}

		C.r_End();
		break;
	case SE_SUN_LUMINANCE: // luminance pass
		//C.r_Pass			("null",			"accum_sun",		false,	FALSE,	FALSE);
		C.r_Pass("stub_notransform_aa_AA", "accum_sun_nomsaa", false, FALSE, FALSE);
		C.r_CullMode(D3DCULL_NONE);

		C.r_dx10Texture("s_position", r2_RT_P);
		C.r_dx10Texture("s_diffuse", r2_RT_albedo);

		C.r_dx10Texture("s_material", r2_material);
		C.r_dx10Texture("s_smap", r2_RT_generic0);

		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_material");
		jitter(C);
		C.r_End();
		break;

		//	SE_SUN_NEAR for min/max
	case SE_SUN_NEAR_MINMAX: // near pass - enable Z-test to perform depth-clipping
		//	FVF::TL2uv
		C.r_Pass("accum_sun", "accum_sun_near_nomsaa_minmax", false, TRUE, FALSE, blend, D3DBLEND_ONE, dest);
		C.r_CullMode(D3DCULL_NONE);
		C.PassSET_ZB(TRUE, FALSE, TRUE); // force inverted Z-Buffer

		C.r_dx10Texture("s_position", r2_RT_P);
		C.r_dx10Texture("s_diffuse", r2_RT_albedo);

		C.r_dx10Texture("s_material", r2_material);
		C.r_dx10Texture("s_accumulator", r2_RT_accum);
		C.r_dx10Texture("s_lmap", r2_sunmask);
		C.r_dx10Texture("s_smap", r2_RT_smap_depth);
		C.r_dx10Texture("s_smap_minmax", r2_RT_smap_depth_minmax);

		//C.r_dx10Texture("s_hud_mask", r2_RT_ssfx_hud);

		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_material");
		C.r_dx10Sampler("smp_linear");
		jitter(C);
		C.r_dx10Sampler("smp_smap");

		C.r_End();
		break;
	}
}


CBlender_accum_direct_msaa::CBlender_accum_direct_msaa()
{
	Name = 0;
	Definition = 0;
	description.CLS = 0;
}

CBlender_accum_direct_msaa::~CBlender_accum_direct_msaa()
{
}

//	TODO: DX10:	implement CBlender_accum_direct::Compile
void CBlender_accum_direct_msaa::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	if (Name)
		::Render->m_MSAASample = atoi(Definition);
	else
		::Render->m_MSAASample = -1;

	//	BOOL	b_HW_smap		= RImplementation.o.HW_smap;
	//	BOOL	b_HW_PCF		= RImplementation.o.HW_smap_PCF;
	BOOL blend = FALSE; //RImplementation.o.fp16_blend;
	D3DBLEND dest = blend ? D3DBLEND_ONE : D3DBLEND_ZERO;
	if (RImplementation.o.sunfilter)
	{
		blend = FALSE;
		dest = D3DBLEND_ZERO;
	}

	switch (C.iElement)
	{
	case SE_SUN_NEAR: // near pass - enable Z-test to perform depth-clipping
	case SE_SUN_MIDDLE: // middle pass - enable Z-test to perform depth-clipping
		//	FVF::TL2uv
		//C.r_Pass			("null",			"accum_sun_near",	false,	TRUE,	FALSE,blend,D3DBLEND_ONE,dest);
		C.r_Pass("accum_sun", "accum_sun_near_msaa_nominmax", false, TRUE, FALSE, blend, D3DBLEND_ONE, dest);
		C.r_CullMode(D3DCULL_NONE);
		C.PassSET_ZB(TRUE, FALSE, TRUE); // force inverted Z-Buffer

		C.r_dx10Texture("s_position", r2_RT_P);
		C.r_dx10Texture("s_diffuse", r2_RT_albedo);

		C.r_dx10Texture("s_material", r2_material);
		C.r_dx10Texture("s_accumulator", r2_RT_accum);
		C.r_dx10Texture("s_lmap", r2_sunmask);
		C.r_dx10Texture("s_smap", r2_RT_smap_depth);

		//C.r_dx10Texture("s_hud_mask", r2_RT_ssfx_hud);
		C.r_dx10Texture("s_ssfx_sss", r2_RT_ssfx_sss);

		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_material");
		C.r_dx10Sampler("smp_linear");
		jitter(C);
		C.r_dx10Sampler("smp_smap");

		C.r_End();
		break;
	case SE_SUN_FAR: // far pass, only stencil clipping performed
		//	FVF::TL2uv
		//C.r_Pass			("null",			"accum_sun_far",	false,	TRUE,	FALSE,blend,D3DBLEND_ONE,dest);
		C.r_Pass("accum_sun", "accum_sun_far_msaa", false, TRUE, FALSE, blend, D3DBLEND_ONE, dest);
		C.r_CullMode(D3DCULL_NONE);

		C.r_dx10Texture("s_position", r2_RT_P);
		C.r_dx10Texture("s_diffuse", r2_RT_albedo);

		C.r_dx10Texture("s_material", r2_material);
		C.r_dx10Texture("s_accumulator", r2_RT_accum);
		C.r_dx10Texture("s_lmap", r2_sunmask);
		C.r_dx10Texture("s_smap", r2_RT_smap_depth);

		//C.r_dx10Texture("s_hud_mask", r2_RT_ssfx_hud);
		C.r_dx10Texture("s_ssfx_sss", r2_RT_ssfx_sss);

		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_material");
		C.r_dx10Sampler("smp_linear");
		jitter(C);
		{
			u32 s = C.r_dx10Sampler("smp_smap");
			if (s != u32(-1)) {
				C.i_dx10Address(s, D3DTADDRESS_BORDER);
				C.i_dx10BorderColor(s, D3DCOLOR_ARGB(255, 255, 255, 255));
			}
		}

		C.r_End();
		break;
	case SE_SUN_LUMINANCE: // luminance pass
		//C.r_Pass			("null",			"accum_sun",		false,	FALSE,	FALSE);
		C.r_Pass("stub_notransform_aa_AA", "accum_sun_msaa", false, FALSE, FALSE);
		C.r_CullMode(D3DCULL_NONE);
		//C.r_Sampler_rtf		("s_position",		r2_RT_P			);
		//C.r_Sampler_rtf		("s_normal",		r2_RT_N			);
		//C.r_Sampler_clw		("s_material",		r2_material		);
		//C.r_Sampler_clf		("s_smap",			r2_RT_generic0	);
		//jitter				(C);

		C.r_dx10Texture("s_position", r2_RT_P);
		C.r_dx10Texture("s_diffuse", r2_RT_albedo);

		C.r_dx10Texture("s_material", r2_material);
		C.r_dx10Texture("s_smap", r2_RT_generic0);

		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_material");
		jitter(C);
		C.r_End();
		break;


		//	SE_SUN_NEAR for minmax
	case SE_SUN_NEAR_MINMAX: // near pass - enable Z-test to perform depth-clipping
		//	FVF::TL2uv
		C.r_Pass("accum_sun", "accum_sun_near_msaa_minmax", false, TRUE, FALSE, blend, D3DBLEND_ONE, dest);
		C.r_CullMode(D3DCULL_NONE);
		C.PassSET_ZB(TRUE, FALSE, TRUE); // force inverted Z-Buffer

		C.r_dx10Texture("s_position", r2_RT_P);
		C.r_dx10Texture("s_diffuse", r2_RT_albedo);

		C.r_dx10Texture("s_material", r2_material);
		C.r_dx10Texture("s_accumulator", r2_RT_accum);
		C.r_dx10Texture("s_lmap", r2_sunmask);
		C.r_dx10Texture("s_smap", r2_RT_smap_depth);
		C.r_dx10Texture("s_smap_minmax", r2_RT_smap_depth_minmax);

		//C.r_dx10Texture("s_hud_mask", r2_RT_ssfx_hud);

		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_material");
		C.r_dx10Sampler("smp_linear");
		jitter(C);
		C.r_dx10Sampler("smp_smap");

		C.r_End();
		break;
	}
	::Render->m_MSAASample = -1;
}

CBlender_accum_direct_volumetric_msaa::CBlender_accum_direct_volumetric_msaa()
{
	Name = 0;
	Definition = 0;
	description.CLS = 0;
}

CBlender_accum_direct_volumetric_msaa::~CBlender_accum_direct_volumetric_msaa()
{
}

//	TODO: DX10:	implement CBlender_accum_direct::Compile
void CBlender_accum_direct_volumetric_msaa::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	if (Name)
		::Render->m_MSAASample = atoi(Definition);
	else
		::Render->m_MSAASample = -1;

	//	BOOL	b_HW_smap		= RImplementation.o.HW_smap;
	//	BOOL	b_HW_PCF		= RImplementation.o.HW_smap_PCF;
	BOOL blend = FALSE; //RImplementation.o.fp16_blend;
	D3DBLEND dest = blend ? D3DBLEND_ONE : D3DBLEND_ZERO;
	if (RImplementation.o.sunfilter)
	{
		blend = FALSE;
		dest = D3DBLEND_ZERO;
	}

	switch (C.iElement)
	{
	case 0: // near pass - enable Z-test to perform depth-clipping
		C.r_Pass("accum_sun", "accum_volumetric_sun_msaa", false, TRUE, FALSE, blend, D3DBLEND_ONE, dest);
		C.r_dx10Texture("s_lmap", C.L_textures[0]);
		C.r_dx10Texture("s_smap", r2_RT_smap_depth);
		C.r_dx10Texture("s_noise", "fx\\fx_noise");

		C.r_dx10Sampler("smp_rtlinear");
		C.r_dx10Sampler("smp_linear");
		C.r_dx10Sampler("smp_smap");
		C.r_End();
		break;
	}
	::Render->m_MSAASample = -1;
}

CBlender_accum_direct_volumetric_sun_msaa::CBlender_accum_direct_volumetric_sun_msaa()
{
	Name = 0;
	Definition = 0;
	description.CLS = 0;
}

CBlender_accum_direct_volumetric_sun_msaa::~CBlender_accum_direct_volumetric_sun_msaa()
{
}

//	TODO: DX10:	implement CBlender_accum_direct::Compile
void CBlender_accum_direct_volumetric_sun_msaa::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);

	if (Name)
		::Render->m_MSAASample = atoi(Definition);
	else
		::Render->m_MSAASample = -1;

	switch (C.iElement)
	{
	case 0: // near pass - enable Z-test to perform depth-clipping
		C.r_Pass("accum_sun", "accum_volumetric_sun_msaa", false, false, false, true, D3DBLEND_ONE, D3DBLEND_ONE, false,
			0);
		C.r_dx10Texture("s_smap", r2_RT_smap_depth);
		C.r_dx10Texture("s_position", r2_RT_P);
		jitter(C);

		C.r_dx10Sampler("smp_nofilter");
		C.r_dx10Sampler("smp_smap");
		C.r_End();
		break;
	}
	::Render->m_MSAASample = -1;
}

#pragma once

#include "../xrRender/ColorMapManager.h"
#include "../xrRender/light_db.h"
class light;

//#define DU_SPHERE_NUMVERTEX 92
//#define DU_SPHERE_NUMFACES	180
//#define DU_CONE_NUMVERTEX	18
//#define DU_CONE_NUMFACES	32
//	no less than 2
#define	VOLUMETRIC_SLICES	100

class CRenderTarget : public IRender_Target
{
private:
	u32 dwWidth;
	u32 dwHeight;
	u32 dwAccumulatorClearMark;
public:
	enum eStencilOptimizeMode
	{
		SO_Light = 0,
		//	Default
		SO_Combine,
		//	Default
	};

	u32 dwLightMarkerID;
	//
	IBlender* b_occq;
	IBlender* b_accum_mask;
	IBlender* b_accum_direct;
	IBlender* b_accum_point;
	IBlender* b_accum_spot;
	IBlender* b_accum_reflected;
	IBlender* b_bloom;
	IBlender* b_luminance;
	IBlender* b_combine;
	IBlender* b_sunshafts;
	IBlender* b_postprocess_msaa;
	IBlender* b_bloom_msaa;
	IBlender* b_combine_msaa[8];
	IBlender* b_accum_mask_msaa[8];
	IBlender* b_accum_spot_msaa[8];
	IBlender* b_accum_direct_msaa[8];
	IBlender* b_accum_direct_volumetric_msaa[8];
	IBlender* b_accum_direct_volumetric_sun_msaa[8];
	IBlender* b_accum_volumetric_msaa[8];
	IBlender* b_accum_point_msaa[8];
	IBlender* b_accum_reflected_msaa[8];
	IBlender* b_ssao;
	IBlender* b_ssao_msaa[8];

	IBlender* b_hdr10_bloom_downsample;
	IBlender* b_hdr10_bloom_blur;
	IBlender* b_hdr10_bloom_upsample;
	
	IBlender* b_hdr10_lens_flare_downsample;
	IBlender* b_hdr10_lens_flare_fgen;
	IBlender* b_hdr10_lens_flare_blur;
	IBlender* b_hdr10_lens_flare_upsample;

	IBlender* b_blur;
	IBlender* b_dof;
	IBlender* b_pp_bloom;
	IBlender* b_gasmask_drops;
	IBlender* b_gasmask_dudv;
	IBlender* b_nightvision;
	IBlender* b_fakescope; //crookr
	IBlender* b_heatvision; //--DSR-- HeatVision
	IBlender* b_lut;
	IBlender* b_smaa;
	// compute shader for hdao
	IBlender* b_hdao_cs;
	IBlender* b_hdao_msaa_cs;

	// [SSS Stuff]
	IBlender* b_ssfx_fog_scattering;
	IBlender* b_ssfx_motion_blur;
	IBlender* b_ssfx_taa;
	IBlender* b_ssfx_rain;
	IBlender* b_ssfx_water_blur;
	IBlender* b_ssfx_bloom_downsample;
	IBlender* b_ssfx_bloom_upsample;
	IBlender* b_ssfx_bloom;
	IBlender* b_ssfx_bloom_lens;
	IBlender* b_ssfx_sss_ext;
	IBlender* b_ssfx_sss;
	IBlender* b_ssfx_ssr;
	IBlender* b_ssfx_volumetric_blur;
	IBlender* b_ssfx_ao;

#ifdef DEBUG
	struct		dbg_line_t		{
		Fvector	P0,P1;
		u32		color;
	};
	xr_vector<std::pair<Fsphere,Fcolor> >		dbg_spheres;
	xr_vector<dbg_line_t>										dbg_lines;
	xr_vector<Fplane>												dbg_planes;
#endif

	// MRT-path
	ref_rt rt_Depth; // Z-buffer like - initial depth
	ref_rt rt_MSAADepth; // z-buffer for MSAA deferred shading
	ref_rt rt_Generic_0_r; // MRT generic 0
	ref_rt rt_Generic_1_r; // MRT generic 1
	ref_rt rt_Generic;
	ref_rt rt_Position; // 64bit,	fat	(x,y,z,?)				(eye-space)
	ref_rt rt_Color; // 64/32bit,fat	(r,g,b,specular-gloss)	(or decompressed MET-8-8-8-8)

	//--DSR-- HeatVision_start
	ref_rt rt_Heat;
	//--DSR-- HeatVision_end

	//
	ref_rt rt_Accumulator; // 64bit		(r,g,b,specular)
	ref_rt rt_Accumulator_temp; // only for HW which doesn't feature fp16 blend
	ref_rt rt_sunshafts_0; // ss0
	ref_rt rt_sunshafts_1; // ss1
	ref_rt rt_Generic_0; // 32bit		(r,g,b,a)				// post-process, intermidiate results, etc.
	ref_rt rt_Generic_1; // 32bit		(r,g,b,a)				// post-process, intermidiate results, etc.

	resptr_core<CRT, resptrcode_crt> rt_Generic_temp;

	ref_rt rt_secondVP;	// 32bit		(r,g,b,a) --//#SM+#-- +SecondVP+


	ref_rt rt_fakescope;	// crookr fakescope

	ref_rt rt_dof;
	ref_rt rt_ui_pda;

	ref_rt rt_blur_h_2;
	ref_rt rt_blur_2;
	ref_rt rt_blur_2_zb;

	ref_rt rt_blur_h_4;
	ref_rt rt_blur_4;
	ref_rt rt_blur_4_zb;

	ref_rt rt_blur_h_8;
	ref_rt rt_blur_8;
	ref_rt rt_blur_8_zb;

	ref_rt rt_pp_bloom;

	ref_rt rt_smaa_edgetex;
	ref_rt rt_smaa_blendtex;

	//	Igor: for volumetric lights
	ref_rt rt_Generic_2; // 32bit		(r,g,b,a)				// post-process, intermidiate results, etc.
	ref_rt rt_Bloom_1; // 32bit, dim/4	(r,g,b,?)
	ref_rt rt_Bloom_2; // 32bit, dim/4	(r,g,b,?)
	ref_rt rt_LUM_64; // 64bit, 64x64,	log-average in all components
	ref_rt rt_LUM_8; // 64bit, 8x8,		log-average in all components

	ref_rt rt_LUM_pool[CHWCaps::MAX_GPUS * 2]; // 1xfp32,1x1,		exp-result -> scaler
	ref_texture t_LUM_src; // source
	ref_texture t_LUM_dest; // destination & usage for current frame

	// HDR10
	ref_rt rt_HDR10_HalfRes[2];

	// env
	ref_texture t_envmap_0; // env-0
	ref_texture t_envmap_1; // env-1

	// smap
	ref_rt rt_smap_surf; // 32bit,		color
	ref_rt rt_smap_depth; // 24(32) bit,	depth
	ref_rt rt_smap_depth_minmax; //	is used for min/max sm
	//	TODO: DX10: CHeck if we need old-style SMAP
	//	IDirect3DSurface9*			rt_smap_ZB;		//

	// Screen Space Shaders Stuff
	ref_rt rt_ssfx;
	ref_rt rt_ssfx_temp;
	ref_rt rt_ssfx_temp2;
	ref_rt rt_ssfx_temp3;

	ref_rt rt_ssfx_accum;
	//ref_rt rt_ssfx_hud; // DEPRECATED
	ref_rt rt_ssfx_ssr;
	ref_rt rt_ssfx_water;
	ref_rt rt_ssfx_water_waves;
	ref_rt rt_ssfx_ao;
	ref_rt rt_ssfx_il;

	ref_rt rt_ssfx_sss;
	ref_rt rt_ssfx_sss_ext;
	ref_rt rt_ssfx_sss_ext2;
	ref_rt rt_ssfx_sss_tmp;
	ref_rt rt_ssfx_bloom1;
	ref_rt rt_ssfx_bloom_emissive;
	ref_rt rt_ssfx_bloom_lens;
	ref_rt rt_ssfx_rain;
	ref_rt rt_ssfx_volumetric;
	ref_rt rt_ssfx_volumetric_tmp;

	ref_rt rt_ssfx_bloom_tmp2;
	ref_rt rt_ssfx_bloom_tmp4;
	ref_rt rt_ssfx_bloom_tmp8;
	ref_rt rt_ssfx_bloom_tmp16;
	ref_rt rt_ssfx_bloom_tmp32;
	ref_rt rt_ssfx_bloom_tmp64;

	ref_rt rt_ssfx_bloom_tmp32_2;
	ref_rt rt_ssfx_bloom_tmp16_2;
	ref_rt rt_ssfx_bloom_tmp8_2;
	ref_rt rt_ssfx_bloom_tmp4_2;

	ref_rt rt_ssfx_taa;
	ref_rt rt_ssfx_prev_frame;
	ref_rt rt_ssfx_motion_vectors;

	ref_rt rt_ssfx_prevPos;

	ref_shader s_ssfx_water;
	ref_shader s_ssfx_water_blur;
	ref_shader s_ssfx_water_ssr;
	ref_shader s_ssfx_ao;
	//ref_shader s_ssfx_hud[5]; // SSS23: DEPRECATED

	Fmatrix Matrix_previous, Matrix_current;
	//Fmatrix Matrix_HUD_previous, Matrix_HUD_current;
	Fvector3 Position_previous;
	//bool RVelocity;

	ref_rt rt_tempzb; // Redotix99: for 3D Shader Based Scopes

	ref_shader s_ssfx_dumb;

	//	Igor: for async screenshots
	ID3DTexture2D* t_ss_async; //32bit		(r,g,b,a) is situated in the system memory

	// Textures
	ID3DTexture3D* t_material_surf;
	ref_texture t_material;

	ID3DTexture2D* t_noise_surf [TEX_jitter_count];
	ref_texture t_noise [TEX_jitter_count];
	ID3DTexture2D* t_noise_surf_mipped;
	ref_texture t_noise_mipped;
private:
	// OCCq

	ref_shader s_occq;
	ref_shader s_sunshafts;
	// SSAO
	ref_rt rt_ssao_temp;
	ref_rt rt_half_depth;
	ref_shader s_ssao;
	ref_shader s_ssao_msaa[8];
	ref_shader s_hdao_cs;
	ref_shader s_hdao_cs_msaa;



	// Accum
	ref_shader s_accum_mask;
	ref_shader s_accum_direct;
	ref_shader s_accum_direct_volumetric;
	ref_shader s_accum_direct_volumetric_minmax;
	ref_shader s_accum_point;
	ref_shader s_accum_spot;
	ref_shader s_accum_reflected;
	ref_shader s_accum_volume;
	ref_shader s_blur;
	ref_shader s_dof;
	ref_shader s_pp_bloom;
	ref_shader s_gasmask_drops;
	ref_shader s_gasmask_dudv;
	ref_shader s_nightvision;
	ref_shader s_fakescope; //crookr
	ref_shader s_heatvision; //--DSR-- HeatVision
	ref_shader s_smaa;

	ref_shader s_lut;
	//	generate min/max
	ref_shader s_create_minmax_sm;

	//	DX10 Rain
	ref_shader s_rain;

	ref_shader s_rain_msaa[8]; // up to 8 shaders for DX10.0 support
	ref_shader s_accum_direct_volumetric_msaa[8];
	ref_shader s_accum_mask_msaa[8];
	ref_shader s_accum_direct_msaa[8];
	ref_shader s_mark_msaa_edges;
	ref_shader s_accum_point_msaa[8];
	ref_shader s_accum_spot_msaa[8];
	ref_shader s_accum_reflected_msaa[8];
	ref_shader s_accum_volume_msaa[8];

	// Screen Space Shaders Stuff
	ref_shader s_ssfx_fog_scattering;
	ref_shader s_ssfx_motion_blur;
	ref_shader s_ssfx_taa;
	ref_shader s_ssfx_rain;
	ref_shader s_ssfx_bloom;
	ref_shader s_ssfx_bloom_lens;
	ref_shader s_ssfx_bloom_upsample;
	ref_shader s_ssfx_bloom_downsample;
	ref_shader s_ssfx_sss_ext;
	ref_shader s_ssfx_sss;

	ref_shader s_ssfx_ssr;
	ref_shader s_ssfx_volumetric_blur;

	ref_geom g_accum_point;
	ref_geom g_accum_spot;
	ref_geom g_accum_omnipart;
	ref_geom g_accum_volumetric;

	ID3DVertexBuffer* g_accum_point_vb;
	ID3DIndexBuffer* g_accum_point_ib;

	ID3DVertexBuffer* g_accum_omnip_vb;
	ID3DIndexBuffer* g_accum_omnip_ib;

	ID3DVertexBuffer* g_accum_spot_vb;
	ID3DIndexBuffer* g_accum_spot_ib;

	ID3DVertexBuffer* g_accum_volumetric_vb;
	ID3DIndexBuffer* g_accum_volumetric_ib;

	// Bloom
	ref_geom g_bloom_build;
	ref_geom g_bloom_filter;
	ref_shader s_bloom_dbg_1;
	ref_shader s_bloom_dbg_2;
	ref_shader s_bloom;
	ref_shader s_bloom_msaa;
	float f_bloom_factor;

	// HDR10
	ref_shader s_hdr10_bloom_downsample;
	ref_shader s_hdr10_bloom_blur;
	ref_shader s_hdr10_bloom_upsample;
	
	ref_shader s_hdr10_lens_flare_downsample;
	ref_shader s_hdr10_lens_flare_fgen;
	ref_shader s_hdr10_lens_flare_blur;
	ref_shader s_hdr10_lens_flare_upsample;

	// Luminance
	ref_shader s_luminance;
	float f_luminance_adapt;

	// Combine
	ref_geom g_KD;
	ref_geom g_combine;
	ref_geom g_combine_VP; // xy=p,zw=tc
	ref_geom g_combine_2UV;
	ref_geom g_combine_cuboid;
	ref_geom g_aa_blur;
	ref_geom g_aa_AA;
	ref_shader s_combine_dbg_0;
	ref_shader s_combine_dbg_1;
	ref_shader s_combine_dbg_Accumulator;
	ref_shader s_combine;
	ref_shader s_combine_msaa[8];
	ref_shader s_combine_volumetric;
public:
	ref_shader s_postprocess;
	ref_shader s_postprocess_msaa;
	ref_geom g_postprocess;
	ref_shader s_menu;
	ref_geom g_menu;
private:
	float im_noise_time;
	u32 im_noise_shift_w;
	u32 im_noise_shift_h;

	float param_blur;
	float param_gray;
	float param_duality_h;
	float param_duality_v;
	float param_noise;
	float param_noise_scale;
	float param_noise_fps;
	u32 param_color_base;
	u32 param_color_gray;
	Fvector param_color_add;

	//	Color mapping
	float param_color_map_influence;
	float param_color_map_interpolate;
	ColorMapManager color_map_manager;

	//	Igor: used for volumetric lights
	bool m_bHasActiveVolumetric;
	bool m_bHasActiveVolumetric_spot;
public:
	CRenderTarget();
	~CRenderTarget();
	void accum_point_geom_create();
	void accum_point_geom_destroy();
	void accum_omnip_geom_create();
	void accum_omnip_geom_destroy();
	void accum_spot_geom_create();
	void accum_spot_geom_destroy();
	//	Igor: used for volumetric lights
	void accum_volumetric_geom_create();
	void accum_volumetric_geom_destroy();

	void u_stencil_optimize(eStencilOptimizeMode eSOM = SO_Light);
	void u_compute_texgen_screen(Fmatrix& dest);
	void u_compute_texgen_jitter(Fmatrix& dest);
	void u_setrt(const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, const ref_rt& _4, ID3DDepthStencilView* zb);
	void u_setrt(const ref_rt& _1, const ref_rt& _2, const ref_rt& _3, ID3DDepthStencilView* zb);
	void u_setrt(const ref_rt& _1, const ref_rt& _2, ID3DDepthStencilView* zb);
	void u_setrt(u32 W, u32 H, ID3DRenderTargetView* _1, ID3DRenderTargetView* _2, ID3DRenderTargetView* _3,
	             ID3DDepthStencilView* zb);
	void u_calc_tc_noise(Fvector2& p0, Fvector2& p1);
	void u_calc_tc_duality_ss(Fvector2& r0, Fvector2& r1, Fvector2& l0, Fvector2& l1);
	BOOL u_need_PP();
	bool u_need_CM();
	BOOL u_DBT_enable(float zMin, float zMax);
	void u_DBT_disable();
	void phase_sunshafts();
	void phase_blur();
	void phase_pp_bloom();
	void phase_dof();
	void phase_gasmask_drops();
	void phase_gasmask_dudv();
	void phase_nightvision();
	void phase_fakescope(); //crookr
	void phase_heatvision(); //--DSR-- HeatVision
	void phase_3DSSReticle(); // Redotix99: for 3D Shader Based Scopes
	void phase_lut();
	void phase_smaa();
	void phase_scene_prepare();
	void phase_scene_begin();
	void phase_scene_end();
	void phase_occq();
	void phase_ssao();
	void phase_hdao();
	void phase_downsamp();
	void phase_wallmarks();
	void phase_smap_direct(light* L, u32 sub_phase);
	void phase_smap_direct_tsh(light* L, u32 sub_phase);
	void phase_smap_spot_clear();
	void phase_smap_spot(light* L);
	void phase_smap_spot_tsh(light* L);
	void phase_accumulator();
	void phase_vol_accumulator();
	void shadow_direct(light* L, u32 dls_phase);

	// SSS Stuff
	void phase_ssfx_taa();
	void phase_ssfx_motion_blur();
	void phase_ssfx_fog_scattering();
	void phase_ssfx_rain(); // Bloom PP
	void phase_ssfx_bloom(); // Bloom PP
	void phase_ssfx_sss(); // SSS
	void phase_ssfx_sss_ext(light_Package& LP); // SSS Spot lights

	void phase_ssfx_ssr(); // SSR Phase
	void phase_ssfx_volumetric_blur(); // Volumetric Blur
	void phase_ssfx_water_blur(); // Water Blur
	void phase_ssfx_water_waves(); // Water Waves
	void phase_ssfx_ao(); // AO
	void phase_ssfx_il(); // IL
	void set_viewport_size(ID3DDeviceContext* dev, float w, float h);

	// HDR10
	void phase_hdr10_bloom();
	void phase_hdr10_lens_flare();

	//	Generates min/max sm
	void create_minmax_SM();

	void phase_rain();
	void draw_rain(light& RainSetup);

	void mark_msaa_edges();

	bool need_to_render_sunshafts();
	bool use_minmax_sm_this_frame();

	BOOL enable_scissor(light* L); // true if intersects near plane
	void enable_dbt_bounds(light* L);

	void disable_aniso();

	void draw_volume(light* L);
	void accum_direct(u32 sub_phase);
	void accum_direct_cascade(u32 sub_phase, Fmatrix& xform, Fmatrix& xform_prev, float fBias);
	void accum_direct_f(u32 sub_phase);
	void accum_direct_lum();
	void accum_direct_blend();
	void accum_direct_volumetric(u32 sub_phase, const u32 Offset, const Fmatrix& mShadow);
	void accum_point(light* L);
	void accum_spot(light* L);
	void accum_reflected(light* L);
	//	Igor: for volumetric lights
	void accum_volumetric(light* L);
	void phase_bloom();
	void phase_luminance();
	void phase_combine();
	void phase_combine_volumetric();
	void phase_pp();

	virtual void set_blur(float f) { param_blur = f; }
	virtual void set_gray(float f) { param_gray = f; }
	virtual void set_duality_h(float f) { param_duality_h = _abs(f); }
	virtual void set_duality_v(float f) { param_duality_v = _abs(f); }
	virtual void set_noise(float f) { param_noise = f; }
	virtual void set_noise_scale(float f) { param_noise_scale = f; }
	virtual void set_noise_fps(float f) { param_noise_fps = _abs(f) + EPS_S; }
	virtual void set_color_base(u32 f) { param_color_base = f; }
	virtual void set_color_gray(u32 f) { param_color_gray = f; }
	virtual void set_color_add(const Fvector& f) { param_color_add = f; }

	virtual u32 get_width() { return dwWidth; }
	virtual u32 get_height() { return dwHeight; }

	virtual void set_cm_imfluence(float f) { param_color_map_influence = f; }
	virtual void set_cm_interpolate(float f) { param_color_map_interpolate = f; }

	virtual void set_cm_textures(const shared_str& tex0, const shared_str& tex1)
	{
		color_map_manager.SetTextures(tex0, tex1);
	}

	//	Need to reset stencil only when marker overflows.
	//	Don't clear when render for the first time
	void reset_light_marker(bool bResetStencil = false);
	void increment_light_marker();

	void DoAsyncScreenshot();

#ifdef DEBUG
	IC void						dbg_addline				(Fvector& P0, Fvector& P1, u32 c)					{
		dbg_lines.push_back		(dbg_line_t());
		dbg_lines.back().P0		= P0;
		dbg_lines.back().P1		= P1;
		dbg_lines.back().color	= c;
	}
	IC void						dbg_addplane			(Fplane& P0,  u32 c)								{
		dbg_planes.push_back(P0);
	}
#else
	IC void dbg_addline(Fvector& P0, Fvector& P1, u32 c)
	{
	}

	IC void dbg_addplane(Fplane& P0, u32 c)
	{
	}
#endif
};

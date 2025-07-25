#ifndef xrRender_consoleH
#define xrRender_consoleH
#pragma once

// Common
extern ECORE_API float ps_ssfx_fog_scattering;
extern ECORE_API Fvector4 ps_ssfx_motionblur;
extern ECORE_API Fvector4 ps_ssfx_taa;

extern ECORE_API Fvector4 ps_ssfx_rain_drops_setup;
extern ECORE_API int ps_ssfx_terrain_grass_align;
extern ECORE_API int ps_ssfx_terrain_pom_refine;
extern ECORE_API int ps_ssfx_pom_refine;
extern ECORE_API int ps_ssfx_terrain_grass_align;
extern ECORE_API float ps_ssfx_terrain_grass_slope;
extern ECORE_API int ps_ssfx_bloom_use_presets;
extern ECORE_API Fvector4 ps_ssfx_bloom_2;
extern ECORE_API Fvector4 ps_ssfx_sss_quality;
extern ECORE_API Fvector4 ps_ssfx_sss;

extern ECORE_API int ps_ssfx_il_quality;
extern ECORE_API int ps_ssfx_ao_quality;
extern ECORE_API Fvector3 ps_ssfx_water_quality;
extern ECORE_API Fvector4 ps_ssfx_il;
extern ECORE_API Fvector4 ps_ssfx_ao;
extern ECORE_API Fvector4 ps_ssfx_water;

extern ECORE_API int ps_ssfx_ssr_quality;
extern ECORE_API Fvector4 ps_ssfx_ssr;
extern ECORE_API Fvector4 ps_ssfx_ssr_2;
extern ECORE_API Fvector4 ps_ssfx_terrain_quality;
extern ECORE_API Fvector3 ps_ssfx_shadows;
extern ECORE_API Fvector4 ps_ssfx_volumetric;

extern ECORE_API Fvector4 ps_ssfx_wind_grass;
extern ECORE_API Fvector4 ps_ssfx_wind_trees;

extern ECORE_API Fvector4 ps_ssfx_rain_1;
extern ECORE_API Fvector4 ps_ssfx_rain_2;
extern ECORE_API Fvector4 ps_ssfx_rain_3;
extern ECORE_API Fvector4 ps_ssfx_grass_shadows;
extern ECORE_API Fvector3 ps_ssfx_shadow_cascades;
extern ECORE_API Fvector4 ps_ssfx_grass_interactive;
extern ECORE_API Fvector4 ps_ssfx_int_grass_params_1;
extern ECORE_API Fvector4 ps_ssfx_int_grass_params_2;

extern ECORE_API u32 ps_r_sun_shafts; //=	0;
extern ECORE_API xr_token qsun_shafts_token[];

extern ECORE_API u32 ps_r_ssao; //	=	0;
extern ECORE_API xr_token qssao_token[];

extern ECORE_API u32 ps_r_ssao_mode;
extern ECORE_API xr_token qssao_mode_token[];

extern ECORE_API u32 ps_r_sun_quality; //	=	0;
extern ECORE_API xr_token qsun_quality_token[];

extern ECORE_API u32 ps_smaa_quality;
extern ECORE_API xr_token smaa_quality_token[];

extern ECORE_API u32 ps_r3_msaa; //	=	0;
extern ECORE_API xr_token qmsaa_token[];

extern ECORE_API u32 ps_r3_msaa_atest; //=	0;
extern ECORE_API xr_token qmsaa__atest_token[];

extern ECORE_API u32 ps_r3_minmax_sm; //	=	0;
extern ECORE_API xr_token qminmax_sm_token[];

extern ENGINE_API int ps_r__Supersample;
extern ECORE_API int ps_r__LightSleepFrames;

extern ECORE_API float ps_r__Detail_l_ambient;
extern ECORE_API float ps_r__Detail_l_aniso;
extern ECORE_API float ps_r__Detail_density;
extern ECORE_API float ps_r__Detail_height;

extern ECORE_API float ps_r__Tree_w_rot;
extern ECORE_API float ps_r__Tree_w_speed;
extern ECORE_API float ps_r__Tree_w_amp;
extern ECORE_API float ps_r__Tree_SBC; // scale bias correct
extern ECORE_API Fvector ps_r__Tree_Wave;

extern ECORE_API float ps_r__WallmarkTTL;
extern ECORE_API float ps_r__WallmarkSHIFT;
extern ECORE_API float ps_r__WallmarkSHIFT_V;

extern ECORE_API float ps_r__GLOD_ssa_start;
extern ECORE_API float ps_r__GLOD_ssa_end;
extern ECORE_API float ps_r__LOD;
//.extern ECORE_API	float		ps_r__LOD_Power		;
extern ECORE_API float ps_r__ssaDISCARD;
extern ECORE_API float ps_r__ssaDONTSORT;
extern ECORE_API float ps_r__ssaHZBvsTEX;
extern ECORE_API int ps_r__tf_Anisotropic;
extern ECORE_API float ps_r__tf_Mipbias;

enum
{
	RFLAG_NO_RAM_TEXTURES = (1 << 0),
};

extern ECORE_API Flags32 ps_r__common_flags;

// R1
extern ECORE_API float ps_r1_ssaLOD_A;
extern ECORE_API float ps_r1_ssaLOD_B;
extern ECORE_API float ps_r1_lmodel_lerp;
extern ECORE_API float ps_r1_dlights_clip;
extern ECORE_API float ps_r1_pps_u;
extern ECORE_API float ps_r1_pps_v;

// R1-specific
extern ECORE_API int ps_r1_GlowsPerFrame; // r1-only
extern ECORE_API Flags32 ps_r1_flags; // r1-only

extern ECORE_API float ps_r1_fog_luminance; //1.f r1-only
extern ECORE_API int ps_r1_SoftwareSkinning; // r1-only

enum
{
	R1FLAG_DLIGHTS = (1 << 0),
};

// R2
extern ECORE_API float ps_r2_ssaLOD_A;
extern ECORE_API float ps_r2_ssaLOD_B;

// R2-specific
extern ECORE_API Flags32 ps_r2_ls_flags; // r2-only
extern ECORE_API Flags32 ps_r2_ls_flags_ext;
extern ECORE_API Flags32 ps_r2_anomaly_flags;
extern ECORE_API float ps_r2_df_parallax_h; // r2-only
extern ECORE_API float ps_r2_df_parallax_range; // r2-only
extern ECORE_API float ps_r2_gmaterial; // r2-only
extern ECORE_API float ps_r2_tonemap_middlegray; // r2-only
extern ECORE_API float ps_r2_tonemap_adaptation; // r2-only
extern ECORE_API float ps_r2_tonemap_low_lum; // r2-only
extern ECORE_API float ps_r2_tonemap_amount; // r2-only
extern ECORE_API float ps_r2_ls_bloom_kernel_scale; // r2-only	// gauss
extern ECORE_API float ps_r2_ls_bloom_kernel_g; // r2-only	// gauss
extern ECORE_API float ps_r2_ls_bloom_kernel_b; // r2-only	// bilinear
extern ECORE_API float ps_r2_ls_bloom_threshold; // r2-only
extern ECORE_API float ps_r2_ls_bloom_speed; // r2-only
extern ECORE_API float ps_r2_ls_dsm_kernel; // r2-only
extern ECORE_API float ps_r2_ls_psm_kernel; // r2-only
extern ECORE_API float ps_r2_ls_ssm_kernel; // r2-only
extern ECORE_API Fvector ps_r2_aa_barier; // r2-only
extern ECORE_API Fvector ps_r2_aa_weight; // r2-only
extern ECORE_API float ps_r2_aa_kernel; // r2-only
extern ECORE_API float ps_r2_mblur; // .5f
extern ECORE_API int ps_r2_GI_depth; // 1..5
extern ECORE_API int ps_r2_GI_photons; // 8..256
extern ECORE_API float ps_r2_GI_clip; // EPS
extern ECORE_API float ps_r2_GI_refl; // .9f
extern ECORE_API float ps_r2_ls_depth_scale; // 1.0f
extern ECORE_API float ps_r2_ls_depth_bias; // -0.0001f
extern ECORE_API float ps_r2_ls_squality; // 1.0f
extern ECORE_API float ps_r2_sun_near; // 10.0f
extern ECORE_API float ps_r2_sun_near_border; // 1.0f
extern ECORE_API float ps_r2_sun_tsm_projection; // 0.2f
extern ECORE_API float ps_r2_sun_tsm_bias; // 0.0001f
extern ECORE_API float ps_r2_sun_depth_far_scale; // 1.00001f
extern ECORE_API float ps_r2_sun_depth_far_bias; // -0.0001f
extern ECORE_API float ps_r2_sun_depth_near_scale; // 1.00001f
extern ECORE_API float ps_r2_sun_depth_near_bias; // -0.0001f
extern ECORE_API float ps_r2_sun_lumscale; // 0.5f
extern ECORE_API float ps_r2_sun_lumscale_hemi; // 1.0f
extern ECORE_API float ps_r2_sun_lumscale_amb; // 1.0f
extern ECORE_API float ps_r2_zfill; // .1f

extern ECORE_API float ps_r2_dhemi_sky_scale; // 1.5f
extern ECORE_API float ps_r2_dhemi_light_scale; // 1.f
extern ECORE_API float ps_r2_dhemi_light_flow; // .1f
extern ECORE_API int ps_r2_dhemi_count; // 5
extern ECORE_API float ps_r2_slight_fade; // 1.f
extern ECORE_API int ps_r2_wait_sleep;
extern ECORE_API int ps_r2_qsync;
////lvutner
extern ECORE_API Fvector4 ps_r2_mask_control; // r2-only
extern ECORE_API Fvector ps_r2_drops_control; // r2-only
extern ECORE_API int ps_r2_nightvision;
extern ECORE_API int scope_fake_enabled; //crookr
extern ECORE_API int scope_3D_fake_enabled; // Redotix99: for 3D Shader Based Scopes
extern ECORE_API int ps_r2_heatvision;			//--DSR-- HeatVision
extern ECORE_API int heat_vision_cooldown;		//--DSR-- HeatVision
extern ECORE_API float heat_vision_cooldown_time;	//--DSR-- HeatVision
extern ECORE_API int heat_vision_zombie_cold;	//--DSR-- HeatVision
extern ECORE_API float ps_r2_ss_sunshafts_length;
extern ECORE_API float ps_r2_ss_sunshafts_radius;
extern u32 ps_sunshafts_mode;

extern ECORE_API float hud_fov_aim_factor;

//--DSR-- SilencerOverheat_start
extern ECORE_API float sil_glow_max_temp;
extern ECORE_API float sil_glow_shot_temp;
extern ECORE_API float sil_glow_cool_temp_rate;
extern ECORE_API Fvector sil_glow_color;
//--DSR-- SilencerOverheat_end

extern ECORE_API Fvector dsr_test;
extern ECORE_API Fvector dsr_test1;
extern ECORE_API Fvector dsr_test2;

extern ECORE_API float ps_r2_tnmp_a; // r2-only
extern ECORE_API float ps_r2_tnmp_b; // r2-only
extern ECORE_API float ps_r2_tnmp_c; // r2-only
extern ECORE_API float ps_r2_tnmp_d; // r2-only
extern ECORE_API float ps_r2_tnmp_e; // r2-only
extern ECORE_API float ps_r2_tnmp_f; // r2-only
extern ECORE_API float ps_r2_tnmp_w; // r2-only
extern ECORE_API float ps_r2_tnmp_exposure; // r2-only
extern ECORE_API float ps_r2_tnmp_gamma; // r2-only
extern ECORE_API float ps_r2_tnmp_onoff; // r2-only

/* --- HDR10 parameters --- */
extern ECORE_API float ps_r4_hdr10_whitepoint_nits; // r4-only
extern ECORE_API float ps_r4_hdr10_ui_nits; 		// r4-only
extern ECORE_API int   ps_r4_hdr10_pda;  			// r4-only (NOTE: this is a hack to not double HDR tonemap the 3D PDA)
extern ECORE_API int   ps_r4_hdr10_on; 			  	// r4-only
extern ECORE_API float ps_r4_hdr10_pda_intensity; 	// r4-only

extern ECORE_API int   ps_r4_hdr10_tonemapper;    		 // r4-only
extern ECORE_API int   ps_r4_hdr10_tonemap_mode;  		 // r4-only
extern ECORE_API float ps_r4_hdr10_exposure;      		 // r4-only
extern ECORE_API float ps_r4_hdr10_contrast;      		 // r4-only
extern ECORE_API float ps_r4_hdr10_contrast_middle_gray; // r4-only
extern ECORE_API float ps_r4_hdr10_saturation;    		 // r4-only
extern ECORE_API float ps_r4_hdr10_brightness;			 // r4-only
extern ECORE_API float ps_r4_hdr10_gamma;				 // r4-only
extern ECORE_API float ps_r4_hdr10_ui_saturation;		 // r4-only

extern ECORE_API int   ps_r4_hdr10_bloom_on;		  // r4-only
extern ECORE_API int   ps_r4_hdr10_bloom_blur_passes; // r4-only
extern ECORE_API float ps_r4_hdr10_bloom_blur_scale;       // r4-only
extern ECORE_API float ps_r4_hdr10_bloom_intensity;   // r4-only

extern ECORE_API int      ps_r4_hdr10_flare_on; // r4-only
extern ECORE_API float    ps_r4_hdr10_flare_threshold; // r4-only
extern ECORE_API float    ps_r4_hdr10_flare_power; // r4-only
extern ECORE_API int      ps_r4_hdr10_flare_ghosts; // r4-only
extern ECORE_API float    ps_r4_hdr10_flare_ghost_dispersal; // r4-only
extern ECORE_API float    ps_r4_hdr10_flare_center_falloff; // r4-only
extern ECORE_API float    ps_r4_hdr10_flare_halo_scale; // r4-only
extern ECORE_API float    ps_r4_hdr10_flare_halo_ca; // r4-only
extern ECORE_API float    ps_r4_hdr10_flare_ghost_ca; // r4-only
extern ECORE_API int      ps_r4_hdr10_flare_blur_passes; // r4-only
extern ECORE_API float    ps_r4_hdr10_flare_blur_scale; // r4-only
extern ECORE_API float    ps_r4_hdr10_flare_ghost_intensity; // r4-only
extern ECORE_API float    ps_r4_hdr10_flare_halo_intensity; // r4-only
extern ECORE_API Fvector3 ps_r4_hdr10_flare_lens_color; // r4-only

extern ECORE_API int   ps_r4_hdr10_sun_on; // r4-only
extern ECORE_API float ps_r4_hdr10_sun_intensity;  // r4-only
extern ECORE_API float ps_r4_hdr10_sun_inner_radius; // r4-only
extern ECORE_API float ps_r4_hdr10_sun_outer_radius; // r4-only
extern ECORE_API float ps_r4_hdr10_sun_dawn_begin; // r4-only
extern ECORE_API float ps_r4_hdr10_sun_dawn_end;   // r4-only
extern ECORE_API float ps_r4_hdr10_sun_dusk_begin; // r4-only
extern ECORE_API float ps_r4_hdr10_sun_dusk_end;   // r4-only
/* --- HDR10 parameters --- */

extern ECORE_API float ps_r2_img_exposure; // r2-only
extern ECORE_API float ps_r2_img_gamma; // r2-only
extern ECORE_API float ps_r2_img_saturation; // r2-only
extern ECORE_API Fvector ps_r2_img_cg; // r2-only

////

//	x - min (0), y - focus (1.4), z - max (100)
extern ECORE_API Fvector3 ps_r2_dof;
extern ECORE_API float ps_r2_dof_sky; //	distance to sky
extern ECORE_API float ps_r2_dof_kernel_size;

extern ECORE_API float ps_r3_dyn_wet_surf_near; // 10.0f
extern ECORE_API float ps_r3_dyn_wet_surf_far; // 30.0f
extern ECORE_API int ps_r3_dyn_wet_surf_sm_res; // 256

extern ECORE_API int opt_static;
extern ECORE_API int opt_dynamic;

enum
{
	R2FLAG_SUN = (1 << 0),
	R2FLAG_SUN_FOCUS = (1 << 1),
	R2FLAG_SUN_TSM = (1 << 2),
	R2FLAG_SUN_DETAILS = (1 << 3),
	R2FLAG_TONEMAP = (1 << 4),
	R2FLAG_AA = (1 << 5),
	R2FLAG_GI = (1 << 6),
	R2FLAG_FASTBLOOM = (1 << 7),
	R2FLAG_GLOBALMATERIAL = (1 << 8),
	R2FLAG_ZFILL = (1 << 9),
	R2FLAG_R1LIGHTS = (1 << 10),
	R2FLAG_SUN_IGNORE_PORTALS = (1 << 11),

	R2FLAG_EXP_SPLIT_SCENE = (1 << 12),
	R2FLAG_EXP_DONT_TEST_UNSHADOWED = (1 << 13),
	R2FLAG_EXP_DONT_TEST_SHADOWED = (1 << 14),

	R2FLAG_USE_NVDBT = (1 << 15),
	R2FLAG_USE_NVSTENCIL = (1 << 16),

	R2FLAG_EXP_MT_CALC = (1 << 17),

	R2FLAG_SOFT_WATER = (1 << 18),
	//	Igor: need restart
	R2FLAG_SOFT_PARTICLES = (1 << 19),
	//	Igor: need restart
	R2FLAG_VOLUMETRIC_LIGHTS = (1 << 20),
	R2FLAG_STEEP_PARALLAX = (1 << 21),
	R2FLAG_DOF = (1 << 22),

	R1FLAG_DETAIL_TEXTURES = (1 << 23),

	R2FLAG_DETAIL_BUMP = (1 << 24),

	R3FLAG_DYN_WET_SURF = (1 << 25),
	R3FLAG_VOLUMETRIC_SMOKE = (1 << 26),

	//R3FLAG_MSAA					= (1<<28),
	R3FLAG_MSAA_HYBRID = (1 << 27),
	R3FLAG_MSAA_OPT = (1 << 28),
	R2FLAG_TERRAIN_PREPASS = (1 << 29),
	R3FLAG_USE_DX10_1 = (1 << 30),
	//R3FLAG_MSAA_ALPHATEST		= (1<<31),
};

enum
{
	R2FLAGEXT_SSAO_BLUR = (1 << 0),
	R2FLAGEXT_SSAO_OPT_DATA = (1 << 1),
	R2FLAGEXT_SSAO_HALF_DATA = (1 << 2),
	R2FLAGEXT_SSAO_HBAO = (1 << 3),
	R2FLAGEXT_SSAO_HDAO = (1 << 4),
	R2FLAGEXT_ENABLE_TESSELLATION = (1 << 5),
	R2FLAGEXT_WIREFRAME = (1 << 6),
	R_FLAGEXT_HOM_DEPTH_DRAW = (1 << 7),
	R2FLAGEXT_SUN_ZCULLING = (1 << 8),
	R2FLAGEXT_SUN_OLD = (1 << 9),
};

enum
{
	R2SS_VOLUMETRIC = 1,
	R2SS_SCREEN_SPACE,
	R2SS_COMBINE_SUNSHAFTS,
};

enum
{
	R2_AN_FLAG_WATER_REFLECTIONS = (1 << 0),
	R2_AN_FLAG_MBLUR = (1 << 1),
	R2_AN_FLAG_FLARES = (1 << 2),
};

//Rezy: cleanup flags
extern Flags32 psDeviceFlags2;

//Swartz: actor shadow
extern ECORE_API Flags32 ps_actor_shadow_flags;

enum
{
	RFLAG_ACTOR_SHADOW = (1 << 0),
};

//-Swartz

extern void xrRender_initconsole();
extern BOOL xrRender_test_hw();

#endif

#include "stdafx.h"
#pragma hdrstop

#include	"../../xrEngine/Render.h"
#include	"../xrRender/ResourceManager.h"
#include	"../xrRender/tss.h"
#include	"../xrRender/blenders/blender.h"
#include	"../xrRender/blenders/blender_recorder.h"
//	adopt_compiler don't have = operator And it can't have = operator
#pragma warning( push )
#pragma warning( disable : 4512)
#include	"../../xrEngine/ai_script_space.h"
#pragma warning( pop )
#include	"../../xrEngine/ai_script_lua_extension.h"
#include	"luabind/return_reference_to_policy.hpp"

#include	"../xrRender/dxRenderDeviceRender.h"

using namespace luabind;

#ifdef	DEBUG
#define MDB	Memory.dbg_check()
#else
#define MDB
#endif

class adopt_dx10options
{
public:
	bool _dx10_msaa_alphatest_atoc()
	{
		return (RImplementation.o.dx10_msaa_alphatest == CRender::MSAA_ATEST_DX10_0_ATOC);
	}

	LPCSTR _get_level()
	{
		const shared_str level_name = g_pGameLevel->name();
		return level_name.c_str();
	}
};

// wrapper
class adopt_dx10sampler
{
	CBlender_Compile* m_pC;
	u32 m_SI; //	Sampler index
public:
	adopt_dx10sampler(CBlender_Compile* C, u32 SamplerIndex) : m_pC(C), m_SI(SamplerIndex)
	{
		if (u32(-1) == m_SI) m_pC = 0;
	}

	adopt_dx10sampler(const adopt_dx10sampler& _C) : m_pC(_C.m_pC), m_SI(_C.m_SI) { if (u32(-1) == m_SI) m_pC = 0; }

	// TODO: why are these commented out?
	//	adopt_sampler&			_texture		(LPCSTR texture)		{ if (C) C->i_Texture	(stage,texture);											return *this;	}
	//	adopt_sampler&			_projective		(bool _b)				{ if (C) C->i_Projective(stage,_b);													return *this;	}
		adopt_dx10sampler&	    _clamp			()						{ if (m_pC) m_pC->i_dx10Address	(m_SI,D3DTADDRESS_CLAMP);							return *this;	}
	//	adopt_sampler&			_wrap			()						{ if (C) C->i_Address	(stage,D3DTADDRESS_WRAP);									return *this;	}
	//	adopt_sampler&			_mirror			()						{ if (C) C->i_Address	(stage,D3DTADDRESS_MIRROR);									return *this;	}
	//	adopt_sampler&			_f_anisotropic	()						{ if (C) C->i_Filter	(stage,D3DTEXF_ANISOTROPIC,D3DTEXF_LINEAR,D3DTEXF_ANISOTROPIC);	return *this;	}
	//	adopt_sampler&			_f_trilinear	()						{ if (C) C->i_Filter	(stage,D3DTEXF_LINEAR,D3DTEXF_LINEAR,D3DTEXF_LINEAR);		return *this;	}
	//	adopt_sampler&			_f_bilinear		()						{ if (C) C->i_Filter	(stage,D3DTEXF_LINEAR,D3DTEXF_POINT, D3DTEXF_LINEAR);		return *this;	}
	//	adopt_sampler&			_f_linear		()						{ if (C) C->i_Filter	(stage,D3DTEXF_LINEAR,D3DTEXF_NONE,  D3DTEXF_LINEAR);		return *this;	}
	//	adopt_sampler&			_f_none			()						{ if (C) C->i_Filter	(stage,D3DTEXF_POINT, D3DTEXF_NONE,  D3DTEXF_POINT);		return *this;	}
	//	adopt_sampler&			_fmin_none		()						{ if (C) C->i_Filter_Min(stage,D3DTEXF_NONE);										return *this;	}
	//	adopt_sampler&			_fmin_point		()						{ if (C) C->i_Filter_Min(stage,D3DTEXF_POINT);										return *this;	}
	//	adopt_sampler&			_fmin_linear	()						{ if (C) C->i_Filter_Min(stage,D3DTEXF_LINEAR);										return *this;	}
	//	adopt_sampler&			_fmin_aniso		()						{ if (C) C->i_Filter_Min(stage,D3DTEXF_ANISOTROPIC);								return *this;	}
	//	adopt_sampler&			_fmip_none		()						{ if (C) C->i_Filter_Mip(stage,D3DTEXF_NONE);										return *this;	}
	//	adopt_sampler&			_fmip_point		()						{ if (C) C->i_Filter_Mip(stage,D3DTEXF_POINT);										return *this;	}
	//	adopt_sampler&			_fmip_linear	()						{ if (C) C->i_Filter_Mip(stage,D3DTEXF_LINEAR);										return *this;	}
	//	adopt_sampler&			_fmag_none		()						{ if (C) C->i_Filter_Mag(stage,D3DTEXF_NONE);										return *this;	}
	//	adopt_sampler&			_fmag_point		()						{ if (C) C->i_Filter_Mag(stage,D3DTEXF_POINT);										return *this;	}
	//	adopt_sampler&			_fmag_linear	()						{ if (C) C->i_Filter_Mag(stage,D3DTEXF_LINEAR);										return *this;	}
};

/*
class	adopt_dx10texture
{
	CBlender_Compile*		m_pC;
	u32						m_TI;	//	Sampler index
public:
	adopt_dx10texture	(CBlender_Compile*	C, u32 TextureIndex)	: m_pC(C), m_TI(TextureIndex)		{ if (u32(-1)==m_TI) m_pC=0;}
	adopt_dx10texture	(const adopt_dx10texture&	_C)				: m_pC(_C.m_pC), m_TI(_C.m_TI)	{ if (u32(-1)==m_TI) m_pC=0;}
};
*/

#pragma warning( push )
#pragma warning( disable : 4512)
// wrapper																																					
class adopt_compiler
{
	CBlender_Compile* C;
	bool& m_bFirstPass;

	void TryEndPass()
	{
		if (!m_bFirstPass) C->r_End();
		m_bFirstPass = false;
	}

public:
	adopt_compiler(CBlender_Compile* _C, bool& bFirstPass) : C(_C), m_bFirstPass(bFirstPass) { m_bFirstPass = true; }

	adopt_compiler(const adopt_compiler& _C) : C(_C.C), m_bFirstPass(_C.m_bFirstPass)
	{
	}

	adopt_compiler& _options(int P, bool S)
	{
		C->SetParams(P, S);
		return *this;
	}

	adopt_compiler& _o_emissive(bool E)
	{
		C->SH->flags.bEmissive = E;
		return *this;
	}

	//  Redotix99: for 3D Shader Based Scopes
	adopt_compiler& _o_scopelense(u32 lenseType)
	{
		C->SH->flags.iScopeLense = lenseType;
		return *this;
	}

	adopt_compiler& _o_distort(bool E)
	{
		C->SH->flags.bDistort = E;
		return *this;
	}

	adopt_compiler& _o_wmark(bool E)
	{
		C->SH->flags.bWmark = E;
		return *this;
	}

	adopt_compiler& _pass(LPCSTR vs, LPCSTR ps)
	{
		TryEndPass();
		C->r_Pass(vs, ps, true);
		return *this;
	}

	adopt_compiler& _passgs(LPCSTR vs, LPCSTR gs, LPCSTR ps)
	{
		TryEndPass();
		C->r_Pass(vs, gs, ps, true);
		return *this;
	}

	adopt_compiler& _fog(bool _fog)
	{
		C->PassSET_LightFog(FALSE, _fog);
		return *this;
	}

	adopt_compiler& _ZB(bool _test, bool _write)
	{
		C->PassSET_ZB(_test, _write);
		return *this;
	}

	adopt_compiler& _blend(bool _blend, u32 abSRC, u32 abDST)
	{
		C->PassSET_ablend_mode(_blend, abSRC, abDST);
		return *this;
	}

	adopt_compiler& _aref(bool _aref, u32 aref)
	{
		C->PassSET_ablend_aref(_aref, aref);
		return *this;
	}

	adopt_compiler& _dx10texture(LPCSTR _resname, LPCSTR _texname)
	{
		C->r_dx10Texture(_resname, _texname);
		return *this;
	}

	adopt_dx10sampler _dx10sampler(LPCSTR _name)
	{
		u32 s = C->r_dx10Sampler(_name);
		return adopt_dx10sampler(C, s);
	}

	//	DX10 specific
	adopt_compiler& _dx10color_write_enable(bool cR, bool cG, bool cB, bool cA)
	{
		C->r_ColorWriteEnable(cR, cG, cB, cA);
		return *this;
	}

	adopt_compiler& _dx10Stencil(bool Enable, u32 Func, u32 Mask, u32 WriteMask, u32 Fail, u32 Pass, u32 ZFail)
	{
		C->r_Stencil(Enable, Func, Mask, WriteMask, Fail, Pass, ZFail);
		return *this;
	}

	adopt_compiler& _dx10StencilRef(u32 Ref)
	{
		C->r_StencilRef(Ref);
		return *this;
	}

	adopt_compiler& _dx10CullMode(u32 Ref)
	{
		C->r_CullMode((D3DCULL)Ref);
		return *this;
	}

	adopt_compiler& _dx10ATOC(bool Enable)
	{
		C->RS.SetRS(XRDX10RS_ALPHATOCOVERAGE, Enable);
		return *this;
	}

	adopt_compiler& _dx10ZFunc(u32 Func)
	{
		C->RS.SetRS(D3DRS_ZFUNC, Func);
		return *this;
	}

	//adopt_dx10texture		_dx10texture	(LPCSTR _name)							{	u32 s = C->r_dx10Texture(_name,0);			return	adopt_dx10sampler(C,s);	}

	adopt_dx10options _dx10Options() { return adopt_dx10options(); };
};
#pragma warning( pop )

class adopt_blend
{
public:
};

class adopt_cmp_func
{
public:
};

class adopt_stencil_op
{
public:
};


void LuaLog3(LPCSTR caMessage)
{
	MDB;
	Lua::LuaOut(Lua::eLuaMessageTypeMessage, "%s", caMessage);
}

void LuaError(lua_State* L)
{
	Debug.fatal(DEBUG_INFO, "LUA error: %s",lua_tostring(L, -1));
}

#ifndef PURE_ALLOC
//#	ifndef USE_MEMORY_MONITOR
//#		define USE_DL_ALLOCATOR
//#	endif // USE_MEMORY_MONITOR
#endif // PURE_ALLOC

#ifndef USE_DL_ALLOCATOR
static void* lua_alloc(void* ud, void* ptr, size_t osize, size_t nsize)
{
	(void)ud;
	(void)osize;
	if (nsize == 0)
	{
		xr_free(ptr);
		return NULL;
	}
	else
#ifdef DEBUG_MEMORY_NAME
		return Memory.mem_realloc		(ptr, nsize, "LUA");
#else // DEBUG_MEMORY_MANAGER
		return Memory.mem_realloc(ptr, nsize);
#endif // DEBUG_MEMORY_MANAGER
}
#else // USE_DL_ALLOCATOR

#ifdef USE_ARENA_ALLOCATOR
	static const u32	s_arena_size = 8*1024*1024;
	static char			s_fake_array[s_arena_size];
//	doug_lea_allocator	g_render_lua_allocator( s_fake_array, s_arena_size, "render:lua" );
#else // #ifdef USE_ARENA_ALLOCATOR
//	doug_lea_allocator	g_render_lua_allocator( 0, 0, "render:lua" );
#endif // #ifdef USE_ARENA_ALLOCATOR

static void *lua_alloc		(void *ud, void *ptr, size_t osize, size_t nsize) {
#ifndef USE_MEMORY_MONITOR
	(void)ud;
	(void)osize;
	if ( !nsize )	{
		g_render_lua_allocator.free_impl	(ptr);
		return					0;
	}

	if ( !ptr )
		return					g_render_lua_allocator.malloc_impl((u32)nsize);

	return g_render_lua_allocator.realloc_impl(ptr, (u32)nsize);
#else // #ifndef USE_MEMORY_MONITOR
	if ( !nsize )	{
		memory_monitor::monitor_free(ptr);
		g_render_lua_allocator.free_impl		(ptr);
		return						NULL;
	}

	if ( !ptr ) {
		void* const result			= 
			g_render_lua_allocator.malloc_impl((u32)nsize);
		memory_monitor::monitor_alloc (result,nsize,"render:LUA");
		return						result;
	}

	memory_monitor::monitor_free	(ptr);
	void* const result				= g_render_lua_allocator.realloc_impl(ptr, (u32)nsize);
	memory_monitor::monitor_alloc	(result,nsize,"render:LUA");
	return							result;
#endif // #ifndef USE_MEMORY_MONITOR
}
#endif // USE_DL_ALLOCATOR

// export
extern int luaopen_lua_extensions(lua_State* L, bool IsDebug = false);

void CResourceManager::LS_Load()
{
#ifdef USE_GSC_MEM_ALLOC
	LSVM = lua_newstate(lua_alloc, NULL);
#else
	LSVM = luaL_newstate();
#endif //-USE_GSC_MEM_ALLOC

	if (!LSVM)
	{
		Msg("! ERROR : Cannot initialize LUA VM!");
		return;
	}

	// initialize lua standard library functions 
	luaopen_base(LSVM);
	luaopen_table(LSVM);
	luaopen_string(LSVM);
	luaopen_math(LSVM);
	luaopen_jit(LSVM);

	luaopen_lua_extensions(LSVM);

	::luabind::open(LSVM);
#if !XRAY_EXCEPTIONS
	if (0 == ::luabind::get_error_callback())
		::luabind::set_error_callback(LuaError);
#endif

	function(LSVM, "log", LuaLog3);

	module(LSVM)
	[
		class_<adopt_dx10options>("_dx10options")
		.def("dx10_msaa_alphatest_atoc", &adopt_dx10options::_dx10_msaa_alphatest_atoc)
		.def("getLevel", &adopt_dx10options::_get_level)
		//.def("",					&adopt_dx10options::_dx10Options		),	// returns options-object
		,


		class_<adopt_dx10sampler>("_dx10sampler")
		// TODO: why are these commented out?
		//.def("texture",						&adopt_sampler::_texture		,return_reference_to(_1))
		//.def("project",						&adopt_sampler::_projective		,return_reference_to(_1))
		.def("clamp",						&adopt_dx10sampler::_clamp			,return_reference_to(_1))
		//.def("wrap",						&adopt_sampler::_wrap			,return_reference_to(_1))
		//.def("mirror",						&adopt_sampler::_mirror			,return_reference_to(_1))
		//.def("f_anisotropic",				&adopt_sampler::_f_anisotropic	,return_reference_to(_1))
		//.def("f_trilinear",					&adopt_sampler::_f_trilinear	,return_reference_to(_1))
		//.def("f_bilinear",					&adopt_sampler::_f_bilinear		,return_reference_to(_1))
		//.def("f_linear",					&adopt_sampler::_f_linear		,return_reference_to(_1))
		//.def("f_none",						&adopt_sampler::_f_none			,return_reference_to(_1))
		//.def("fmin_none",					&adopt_sampler::_fmin_none		,return_reference_to(_1))
		//.def("fmin_point",					&adopt_sampler::_fmin_point		,return_reference_to(_1))
		//.def("fmin_linear",					&adopt_sampler::_fmin_linear	,return_reference_to(_1))
		//.def("fmin_aniso",					&adopt_sampler::_fmin_aniso		,return_reference_to(_1))
		//.def("fmip_none",					&adopt_sampler::_fmip_none		,return_reference_to(_1))
		//.def("fmip_point",					&adopt_sampler::_fmip_point		,return_reference_to(_1))
		//.def("fmip_linear",					&adopt_sampler::_fmip_linear	,return_reference_to(_1))
		//.def("fmag_none",					&adopt_sampler::_fmag_none		,return_reference_to(_1))
		//.def("fmag_point",					&adopt_sampler::_fmag_point		,return_reference_to(_1))
		//.def("fmag_linear",					&adopt_sampler::_fmag_linear	,return_reference_to(_1))
		,

		class_<adopt_compiler>("_compiler")
		.def(constructor<const adopt_compiler&>())
		.def("begin", &adopt_compiler::_pass, return_reference_to(_1))
		.def("begin", &adopt_compiler::_passgs, return_reference_to(_1))
		.def("sorting", &adopt_compiler::_options, return_reference_to(_1))
		.def("emissive", &adopt_compiler::_o_emissive, return_reference_to(_1))
		.def("distort", &adopt_compiler::_o_distort, return_reference_to(_1))
		.def("wmark", &adopt_compiler::_o_wmark, return_reference_to(_1))
		.def("fog", &adopt_compiler::_fog, return_reference_to(_1))
		.def("zb", &adopt_compiler::_ZB, return_reference_to(_1))
		.def("blend", &adopt_compiler::_blend, return_reference_to(_1))
		.def("aref", &adopt_compiler::_aref, return_reference_to(_1))

		.def("scopelense", &adopt_compiler::_o_scopelense, return_reference_to(_1)) //  Redotix99: for 3D Shader Based Scopes

		//	For compatibility only
		.def("dx10color_write_enable", &adopt_compiler::_dx10color_write_enable, return_reference_to(_1))
		.def("color_write_enable", &adopt_compiler::_dx10color_write_enable, return_reference_to(_1))
		.def("dx10texture", &adopt_compiler::_dx10texture, return_reference_to(_1))
		.def("dx10stencil", &adopt_compiler::_dx10Stencil, return_reference_to(_1))
		.def("dx10stencil_ref", &adopt_compiler::_dx10StencilRef, return_reference_to(_1))
		.def("dx10cullmode", &adopt_compiler::_dx10CullMode, return_reference_to(_1))
		.def("dx10atoc", &adopt_compiler::_dx10ATOC, return_reference_to(_1))
		.def("dx10zfunc", &adopt_compiler::_dx10ZFunc, return_reference_to(_1))

		.def("dx10sampler", &adopt_compiler::_dx10sampler) // returns sampler-object
		.def("dx10Options", &adopt_compiler::_dx10Options), // returns options-object	


		class_<adopt_blend>("blend")
		.enum_("blend")
		[
			value("zero", int(D3DBLEND_ZERO)),
			value("one", int(D3DBLEND_ONE)),
			value("srccolor", int(D3DBLEND_SRCCOLOR)),
			value("invsrccolor", int(D3DBLEND_INVSRCCOLOR)),
			value("srcalpha", int(D3DBLEND_SRCALPHA)),
			value("invsrcalpha", int(D3DBLEND_INVSRCALPHA)),
			value("destalpha", int(D3DBLEND_DESTALPHA)),
			value("invdestalpha", int(D3DBLEND_INVDESTALPHA)),
			value("destcolor", int(D3DBLEND_DESTCOLOR)),
			value("invdestcolor", int(D3DBLEND_INVDESTCOLOR)),
			value("srcalphasat", int(D3DBLEND_SRCALPHASAT))
		],

		class_<adopt_cmp_func>("cmp_func")
		.enum_("cmp_func")
		[
			value("never", int(D3DCMP_NEVER)),
			value("less", int(D3DCMP_LESS)),
			value("equal", int(D3DCMP_EQUAL)),
			value("lessequal", int(D3DCMP_LESSEQUAL)),
			value("greater", int(D3DCMP_GREATER)),
			value("notequal", int(D3DCMP_NOTEQUAL)),
			value("greaterequal", int(D3DCMP_GREATEREQUAL)),
			value("always", int(D3DCMP_ALWAYS))
		],

		class_<adopt_stencil_op>("stencil_op")
		.enum_("stencil_op")
		[
			value("keep", int(D3DSTENCILOP_KEEP)),
			value("zero", int(D3DSTENCILOP_ZERO)),
			value("replace", int(D3DSTENCILOP_REPLACE)),
			value("incrsat", int(D3DSTENCILOP_INCRSAT)),
			value("decrsat", int(D3DSTENCILOP_DECRSAT)),
			value("invert", int(D3DSTENCILOP_INVERT)),
			value("incr", int(D3DSTENCILOP_INCR)),
			value("decr", int(D3DSTENCILOP_DECR))
		]
	];

	// load shaders
	xr_vector<char*>* folder = FS.file_list_open("$game_shaders$", ::Render->getShaderPath(),
	                                             FS_ListFiles | FS_RootOnly);
	VERIFY(folder);
	for (u32 it = 0; it < folder->size(); it++)
	{
		string_path namesp, fn;
		xr_strcpy(namesp, (*folder)[it]);
		if (0 == strext(namesp) || 0 != xr_strcmp(strext(namesp), ".s")) continue;
		*strext(namesp) = 0;
		if (0 == namesp[0]) xr_strcpy(namesp, "_G");
		strconcat(sizeof(fn), fn, ::Render->getShaderPath(), (*folder)[it]);
		FS.update_path(fn, "$game_shaders$", fn);
		try
		{
			Script::bfLoadFileIntoNamespace(LSVM, fn, namesp, true);
		}
		catch (...)
		{
			Log(lua_tostring(LSVM, -1));
		}
	}
	FS.file_list_close(folder);
}

void CResourceManager::LS_Unload()
{
	lua_close(LSVM);
	LSVM = NULL;
}

BOOL CResourceManager::_lua_HasShader(LPCSTR s_shader)
{
	string256 undercorated;
	for (int i = 0, l = xr_strlen(s_shader) + 1; i < l; i++)
		undercorated[i] = ('\\' == s_shader[i]) ? '_' : s_shader[i];

#ifdef _EDITOR
	return Script::bfIsObjectPresent(LSVM,undercorated,"editor",LUA_TFUNCTION);
#else
	return Script::bfIsObjectPresent(LSVM, undercorated, "normal",LUA_TFUNCTION) ||
		Script::bfIsObjectPresent(LSVM, undercorated, "l_special",LUA_TFUNCTION);
#endif
}

Shader* CResourceManager::_lua_Create(LPCSTR d_shader, LPCSTR s_textures)
{
	CBlender_Compile C;
	Shader S;

	// undecorate
	string256 undercorated;
	for (int i = 0, l = xr_strlen(d_shader) + 1; i < l; i++)
		undercorated[i] = ('\\' == d_shader[i]) ? '_' : d_shader[i];
	LPCSTR s_shader = undercorated;

	// Access to template
	C.BT = NULL;
	C.bEditor = FALSE;
	C.bDetail = FALSE;

	// Prepare
	_ParseList(C.L_textures, s_textures);
	C.detail_texture = NULL;
	C.detail_scaler = NULL;

	// Compile element	(LOD0 - HQ)
	if (Script::bfIsObjectPresent(LSVM, s_shader, "normal_hq",LUA_TFUNCTION))
	{
		// Analyze possibility to detail this shader
		C.iElement = 0;
		//.		C.bDetail			= dxRenderDeviceRender::Instance().Resources->_GetDetailTexture(*C.L_textures[0],C.detail_texture,C.detail_scaler);
		//C.bDetail			= dxRenderDeviceRender::Instance().Resources->m_textures_description.GetDetailTexture(C.L_textures[0],C.detail_texture,C.detail_scaler);
		C.bDetail = dxRenderDeviceRender::Instance().Resources->m_textures_description.GetDetailTexture(
			C.L_textures[0], C.detail_texture, C.detail_scaler);

		if (C.bDetail) S.E[0] = C._lua_Compile(s_shader, "normal_hq");
		else S.E[0] = C._lua_Compile(s_shader, "normal");
	}
	else
	{
		if (Script::bfIsObjectPresent(LSVM, s_shader, "normal",LUA_TFUNCTION))
		{
			C.iElement = 0;
			//.			C.bDetail			= dxRenderDeviceRender::Instance().Resources->_GetDetailTexture(*C.L_textures[0],C.detail_texture,C.detail_scaler);
			//C.bDetail			= dxRenderDeviceRender::Instance().Resources->m_textures_description.GetDetailTexture(C.L_textures[0],C.detail_texture,C.detail_scaler);
			C.bDetail = dxRenderDeviceRender::Instance().Resources->m_textures_description.GetDetailTexture(
				C.L_textures[0], C.detail_texture, C.detail_scaler);
			S.E[0] = C._lua_Compile(s_shader, "normal");

/// SSS fix water for DX10
#if RENDER == R_R4
			// Water Flag
			if (S.E[0]->flags.bDistort)
			{
				if (strstr(s_shader, "effects_water"))
					S.E[0]->flags.isWater = TRUE;
			}
#endif
		}
	}

	// Compile element	(LOD1)
	if (Script::bfIsObjectPresent(LSVM, s_shader, "normal",LUA_TFUNCTION))
	{
		C.iElement = 1;
		//.		C.bDetail			= dxRenderDeviceRender::Instance().Resources->_GetDetailTexture(*C.L_textures[0],C.detail_texture,C.detail_scaler);
		//C.bDetail			= dxRenderDeviceRender::Instance().Resources->m_textures_description.GetDetailTexture(C.L_textures[0],C.detail_texture,C.detail_scaler);
		C.bDetail = dxRenderDeviceRender::Instance().Resources->m_textures_description.GetDetailTexture(
			C.L_textures[0], C.detail_texture, C.detail_scaler);
		S.E[1] = C._lua_Compile(s_shader, "normal");
	}

	// Compile element
	if (Script::bfIsObjectPresent(LSVM, s_shader, "l_point",LUA_TFUNCTION))
	{
		C.iElement = 2;
		C.bDetail = FALSE;
		S.E[2] = C._lua_Compile(s_shader, "l_point");;
	}

	// Compile element
	if (Script::bfIsObjectPresent(LSVM, s_shader, "l_spot",LUA_TFUNCTION))
	{
		C.iElement = 3;
		C.bDetail = FALSE;
		S.E[3] = C._lua_Compile(s_shader, "l_spot");;
	}

	// Compile element
	if (Script::bfIsObjectPresent(LSVM, s_shader, "l_special",LUA_TFUNCTION))
	{
		C.iElement = 4;
		C.bDetail = FALSE;
		S.E[4] = C._lua_Compile(s_shader, "l_special");
	}

	// Search equal in shaders array
	for (u32 it = 0; it < v_shaders.size(); it++)
		if (S.equal(v_shaders[it])) return v_shaders[it];

	// Create _new_ entry
	Shader* N = xr_new<Shader>(S);
	N->dwFlags |= xr_resource_flagged::RF_REGISTERED;
	v_shaders.push_back(N);
	return N;
}

ShaderElement* CBlender_Compile::_lua_Compile(LPCSTR namesp, LPCSTR name)
{
	ShaderElement E;
	SH = &E;
	RS.Invalidate();

	// Compile
	LPCSTR t_0 = *L_textures[0] ? *L_textures[0] : "null";
	LPCSTR t_1 = (L_textures.size() > 1) ? *L_textures[1] : "null";
	LPCSTR t_d = detail_texture ? detail_texture : "null";
	lua_State* LSVM = dxRenderDeviceRender::Instance().Resources->LSVM;
	object shader = get_globals(LSVM)[namesp];
	functor<void> element = object_cast<functor<void>>(shader[name]);
	bool bFirstPass = false;
	adopt_compiler ac = adopt_compiler(this, bFirstPass);
	element(ac, t_0, t_1, t_d);
	r_End();
	ShaderElement* _r = dxRenderDeviceRender::Instance().Resources->_CreateElement(E);
	return _r;
}

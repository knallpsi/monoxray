////////////////////////////////////////////////////////////////////////////
//	Module 		: script_storage.cpp
//	Created 	: 01.04.2004
//  Modified 	: [1/14/2015 Andrey]
//	Author		: Dmitriy Iassenev
//	Description : XRay Script Storage
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_storage.h"
#include "script_thread.h"
#include "../xrCore/mezz_stringbuffer.h"
#include <stdarg.h>
#include <unordered_map>
#include <set>
#include <sstream>
#include <regex>

#if !defined(DEBUG) && defined(USE_LUAJIT_ONE)
#	include "opt.lua.h"
#	include "opt_inline.lua.h"
#endif //!DEBUG && USE_LUAJIT_ONE
#ifndef USE_LUAJIT_ONE
#include "lua.hpp"
#endif

extern "C"
{
#include <lua.h>
	int luaopen_marshal(lua_State* L);

}
struct luajit
{
	static void open_lib(lua_State* L, pcstr module_name, lua_CFunction function)
	{
		lua_pushcfunction(L, function);
		lua_pushstring(L, module_name);
		lua_call(L, 1, 0);
	}
};

LPCSTR file_header_old =
	"\
                          local function script_name() \
                          return \"%s\" \
                          end \
                          local this = {} \
                          %s this %s \
                          setmetatable(this, {__index = _G}) \
                          setfenv(1, this) \
                                ";

LPCSTR file_header_new =
	"\
                          local function script_name() \
                          return \"%s\" \
                          end \
                          local this = {} \
                          this._G = _G \
                          %s this %s \
                          setfenv(1, this) \
                                ";

LPCSTR file_header = 0;

#ifndef ENGINE_BUILD
#	include "script_engine.h"
#	include "ai_space.h"
#else //ENGINE_BUILD
#	define NO_XRGAME_SCRIPT_ENGINE
#endif //!ENGINE_BUILD

#ifndef XRGAME_EXPORTS
#	define NO_XRGAME_SCRIPT_ENGINE
#endif //!XRGAME_EXPORTS

#ifndef NO_XRGAME_SCRIPT_ENGINE
#	include "ai_debug.h"
#endif //!NO_XRGAME_SCRIPT_ENGINE

#ifdef USE_DEBUGGER
#	include "script_debugger.h"
#endif

#ifndef PURE_ALLOC
//#	ifndef USE_MEMORY_MONITOR
//#		define USE_DL_ALLOCATOR
//#	endif //!USE_MEMORY_MONITOR
#endif //!PURE_ALLOC

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

u32 game_lua_memory_usage()
{
	return (0);
}
#else //USE_DL_ALLOCATOR

#   ifdef USE_ARENA_ALLOCATOR
        static const u32			s_arena_size = 96*1024*1024;
        static char					s_fake_array[s_arena_size];
//        static doug_lea_allocator	s_allocator( s_fake_array, s_arena_size, "lua" );
#   else //-USE_ARENA_ALLOCATOR
//        static doug_lea_allocator	s_allocator(0, 0, "lua");
#   endif //-USE_ARENA_ALLOCATOR

static void *lua_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
#ifndef USE_MEMORY_MONITOR
    (void)ud;
    (void) osize;
    if (!nsize)
    {
        s_allocator.free_impl(ptr);
        return 0;
    }
    if (!ptr)
        return s_allocator.malloc_impl((u32) nsize);

    return s_allocator.realloc_impl(ptr, (u32) nsize);
#else //USE_MEMORY_MONITOR
    if ( !nsize )	{
        memory_monitor::monitor_free(ptr);
        s_allocator.free_impl		(ptr);
        return						NULL;
    }

    if ( !ptr ) {
        void* const result			= s_allocator.malloc_impl((u32)nsize);
        memory_monitor::monitor_alloc (result,nsize,"LUA");
        return						result;
    }

    memory_monitor::monitor_free	(ptr);
    void* const result				= s_allocator.realloc_impl(ptr, (u32)nsize);
    memory_monitor::monitor_alloc	(result,nsize,"LUA");
    return							result;
#endif //!USE_MEMORY_MONITOR
}

u32 game_lua_memory_usage()
{
    return (s_allocator.get_allocated_size());
}
#endif //!USE_DL_ALLOCATOR

static LPVOID __cdecl luabind_allocator(
	::luabind::memory_allocation_function_parameter const,
	void const* const pointer,
	size_t const size
)
{
	if (!size)
	{
		LPVOID non_const_pointer = const_cast<LPVOID>(pointer);
		xr_free(non_const_pointer);
		return (0);
	}

	if (!pointer)
	{
#ifdef DEBUG
        return	( Memory.mem_alloc(size, "luabind") );
#else //!DEBUG
		return (Memory.mem_alloc(size));
#endif //-DEBUG
	}

	LPVOID non_const_pointer = const_cast<LPVOID>(pointer);
#ifdef DEBUG
    return		( Memory.mem_realloc(non_const_pointer, size, "luabind") );
#else //!DEBUG
	return (Memory.mem_realloc(non_const_pointer, size));
#endif //-DEBUG
}

void setup_luabind_allocator()
{
	::luabind::allocator = &luabind_allocator;
	::luabind::allocator_parameter = 0;
}


#ifdef USE_LUAJIT_ONE //  [1/14/2015 Andrey]

/* ---- start of LuaJIT extensions */
static void l_message(lua_State* state, const char *msg)
{
    Msg("! [LUA_JIT] %s", msg);
}


static int report(lua_State *L, int status)
{
    if (status && !lua_isnil(L, -1))
    {
        const char *msg = lua_tostring(L, -1);
        if (msg == NULL) msg = "(error object is not a string)";
        l_message(L, msg);
        lua_pop(L, 1);
    }
    return status;
}

static int loadjitmodule(lua_State *L, const char *notfound)
{
    lua_getglobal(L, "require");
    lua_pushliteral(L, "jit.");
    lua_pushvalue(L, -3);
    lua_concat(L, 2);
    if (lua_pcall(L, 1, 1, 0))
    {
        const char *msg = lua_tostring(L, -1);
        if (msg && !strncmp(msg, "module ", 7))
        {
            l_message(L, notfound);
            return 1;
        }
        else
            return report(L, 1);
    }
    lua_getfield(L, -1, "start");
    lua_remove(L, -2);  /* drop module table */
    return 0;
}

/* JIT engine control command: try jit library first or load add-on module */
static int dojitcmd(lua_State *L, const char *cmd)
{
    const char *val = strchr(cmd, '=');
    lua_pushlstring(L, cmd, val ? val - cmd : xr_strlen(cmd));
    lua_getglobal(L, "jit");  /* get jit.* table */
    lua_pushvalue(L, -2);
    lua_gettable(L, -2);  /* lookup library function */
    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 2);  /* drop non-function and jit.* table, keep module name */
        if (loadjitmodule(L, "unknown luaJIT command"))
            return 1;
    }
    else
    {
        lua_remove(L, -2);  /* drop jit.* table */
    }
    lua_remove(L, -2);  /* drop module name */
    if (val) lua_pushstring(L, val + 1);
    return report(L, lua_pcall(L, val ? 1 : 0, 0, 0));
}

void jit_command(lua_State* state, LPCSTR command)
{
    dojitcmd(state, command);
}

#ifndef DEBUG
/* start optimizer */
static int dojitopt(lua_State *L, const char *opt)
{
    lua_pushliteral(L, "opt");
    if (loadjitmodule(L, "LuaJIT optimizer module not installed"))
        return 1;
    lua_remove(L, -2);  /* drop module name */
    if (*opt) lua_pushstring(L, opt);
    return report(L, lua_pcall(L, *opt ? 1 : 0, 0, 0));
}

static void put_function(lua_State* state, u8 const* buffer, u32 const buffer_size, LPCSTR package_id)
{
    lua_getglobal(state, "package");
    lua_pushstring(state, "preload");
    lua_gettable(state, -2);

    lua_pushstring(state, package_id);
    luaL_loadbuffer(state, (char*) buffer, buffer_size, package_id);
    lua_settable(state, -3);
}

/* ---- end of LuaJIT extensions */
#endif //!DEBUG
#endif //-USE_LUAJIT_ONE


CScriptStorage::CScriptStorage()
{
	m_current_thread = 0;

#ifdef DEBUG
    m_stack_is_ready		= false;
#endif //-DEBUG

	m_virtual_machine = 0;

#ifdef USE_LUA_STUDIO
#	ifndef USE_DEBUGGER
    STATIC_CHECK( false, Do_Not_Define_USE_LUA_STUDIO_macro_without_USE_DEBUGGER_macro );
#	endif //!USE_DEBUGGER
#endif //-USE_LUA_STUDIO
}

CScriptStorage::~CScriptStorage()
{
	if (m_virtual_machine)
		lua_close(m_virtual_machine);
}

extern int luaopen_lua_extensions(lua_State* L, bool IsDebug = false);
extern lua_CFunction luaopen_socket_core_init();
extern void pdebug_init_init(lua_State* L);

void disable_os_funcs(lua_State* L)
{
	lua_getglobal(L, "os");
	lua_pushnil(L);
	lua_setfield(L, -2, "execute");
	lua_pushnil(L);
	lua_setfield(L, -2, "rename");
	lua_pushnil(L);
	lua_setfield(L, -2, "remove");
	lua_pushnil(L);
	lua_setfield(L, -2, "exit");
	lua_pop(L, 1);

	lua_getglobal(L, "io");
	lua_pushnil(L);
	lua_setfield(L, -2, "popen");
	lua_pop(L, 1);
}

bool LoadKernelScriptToGlobal(lua_State* L, const char* name)
{
	string_path FileName;
	xr_string FixedFileName = name;
	//FixedFileName = "kernel\\" + FixedFileName; //When this line is enabled all lua files are not being loaded after loading a game

	if (FS.exist(FileName, "$game_scripts$", FixedFileName.data()))
	{
		int	start = lua_gettop(L);
		IReader* l_tpFileReader = FS.r_open(FileName);

		string_path NameSpace;
		xr_strcpy(NameSpace, name);

		if (strext(NameSpace))
			*strext(NameSpace) = 0;

		if (luaL_loadbuffer(L, (const char*)l_tpFileReader->pointer(), l_tpFileReader->length(), NameSpace))
		{
			lua_settop(L, start);
			return false;
		}
		else
		{
			int errFuncId = -1;
			int	l_iErrorCode = lua_pcall(L, 0, 0, (-1 == errFuncId) ? 0 : errFuncId);
			if (l_iErrorCode)
			{
#ifdef DEBUG
				g_pScriptEngine->print_output(L, name, l_iErrorCode);
#endif
				lua_settop(L, start);
				return false;
			}
		}


		FS.r_close(l_tpFileReader);
	}
	else
	{
		return false;
	}

	return true;
};

BOOL lua_debug = FALSE;
void CScriptStorage::reinit()
{
	if (m_virtual_machine)
		lua_close(m_virtual_machine);

#ifdef USE_GSC_MEM_ALLOC
    m_virtual_machine = lua_newstate(lua_alloc, NULL);
#else
	m_virtual_machine = luaL_newstate();
#endif //-USE_GSC_MEM_ALLOC

	if (!m_virtual_machine)
	{
		Msg("! ERROR : Cannot initialize script virtual machine!");
		return;
	}


#ifndef USE_LUAJIT_ONE
	luaL_openlibs(lua());
	if (strstr(Core.Params, "-nojit"))
		luaJIT_setmode(lua(), 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_OFF);
#else // USE_LUAJIT_ONE
    // initialize lua standard library functions    

    luajit::open_lib(lua(), "", luaopen_base);
    luajit::open_lib(lua(), LUA_LOADLIBNAME, luaopen_package);
    luajit::open_lib(lua(), LUA_TABLIBNAME, luaopen_table);
    luajit::open_lib(lua(), LUA_IOLIBNAME, luaopen_io);
    luajit::open_lib(lua(), LUA_OSLIBNAME, luaopen_os);
    luajit::open_lib(lua(), LUA_MATHLIBNAME, luaopen_math);
    luajit::open_lib(lua(), LUA_STRLIBNAME, luaopen_string);
    luajit::open_lib(lua(), LUA_JITLIBNAME, luaopen_jit);
	luajit::open_lib(lua(), LUA_BITLIBNAME, luaopen_bit);
	luajit::open_lib(lua(), LUA_FFILIBNAME, luaopen_ffi);

#ifdef DEBUG
    luajit::open_lib(lua(), LUA_DBLIBNAME, luaopen_debug);
#else //!DEBUG

    if (strstr(Core.Params, "-dbg"))
        luajit::open_lib(lua(), LUA_DBLIBNAME, luaopen_debug);
#endif //-DEBUG

	if (!strstr(Core.Params, "-nojit"))
	{
		luajit::open_lib(lua(), LUA_JITLIBNAME, luaopen_jit);
#ifndef DEBUG
		put_function(lua(), opt_lua_binary, sizeof(opt_lua_binary), "jit.opt");
		put_function(lua(), opt_inline_lua_binary, sizeof(opt_lua_binary), "jit.opt_inline");
		dojitopt(lua(), "2");
#endif //!DEBUG
	}

#endif //!USE_LUAJIT_ONE
	bool isDebugEnabled = lua_debug;
	luaopen_lua_extensions(lua(), isDebugEnabled);
	disable_os_funcs(lua());

	if (isDebugEnabled)
	{
		Msg("!lua_debug 1, opening socket and initializing LuaPanda");
		LoadKernelScriptToGlobal(lua(), "global.lua");
		LoadKernelScriptToGlobal(lua(), "dynamic_callbacks.lua");

		// Sockets
		luajit::open_lib(lua(), "socket.core", luaopen_socket_core_init());
		bool SocketTest = LoadKernelScriptToGlobal(lua(), "socket.lua");

		// Panda
		if (SocketTest)
		{
			pdebug_init_init(lua());
			LoadKernelScriptToGlobal(lua(), "LuaPanda.lua");
		}
	}
	
	if (strstr(Core.Params, "-_g"))
		file_header = file_header_new; //AVO: I get fatal crash at the start if this is used
	else
		file_header = file_header_old;
}

int CScriptStorage::vscript_log(ScriptStorage::ELuaMessageType tLuaMessageType, LPCSTR caFormat, va_list marker)
{
#ifndef NO_XRGAME_SCRIPT_ENGINE
#   ifdef DEBUG
        if (!psAI_Flags.test(aiLua) && (tLuaMessageType != ScriptStorage::eLuaMessageTypeError))
            return(0);
#   endif //-DEBUG
#endif //!NO_XRGAME_SCRIPT_ENGINE

	//#ifndef PRINT_CALL_STACK
	//return		(0);
	//#else //PRINT_CALL_STACK
#   ifndef NO_XRGAME_SCRIPT_ENGINE
	//AVO: allow LUA debug prints (i.e.: ai().script_engine().script_log(ScriptStorage::eLuaMessageTypeError, "CWeapon : cannot access class member Weapon_IsScopeAttached!");)
#       ifndef DEBUG

	if (!strstr(Core.Params, "-dbg"))
		return (0);
#       endif //!DEBUG
#       ifndef LUA_DEBUG_PRINT
#           ifdef DEBUG
                if (!psAI_Flags.test(aiLua) && (tLuaMessageType != ScriptStorage::eLuaMessageTypeError))
                    return(0);
#           endif //-DEBUG
#       else //!LUA_DEBUG_PRINT
            if (!psAI_Flags.test(aiLua) && (tLuaMessageType != ScriptStorage::eLuaMessageTypeError))
                return(0);
#       endif //-LUA_DEBUG_PRINT
#endif //-NO_XRGAME_SCRIPT_ENGINE

	LPCSTR S = "", SS = "";
	LPSTR S1;
	string4096 S2;
	switch (tLuaMessageType)
	{
	case ScriptStorage::eLuaMessageTypeInfo:
		{
			S = "* [LUA] ";
			SS = "[INFO]        ";
			break;
		}
	case ScriptStorage::eLuaMessageTypeError:
		{
			S = "! [LUA] ";
			SS = "[ERROR]       ";
			break;
		}
	case ScriptStorage::eLuaMessageTypeMessage:
		{
			S = "~ [LUA] ";
			SS = "[MESSAGE]     ";
			break;
		}
	case ScriptStorage::eLuaMessageTypeHookCall:
		{
			S = "[LUA][HOOK_CALL] ";
			SS = "[CALL]        ";
			break;
		}
	case ScriptStorage::eLuaMessageTypeHookReturn:
		{
			S = "[LUA][HOOK_RETURN] ";
			SS = "[RETURN]      ";
			break;
		}
	case ScriptStorage::eLuaMessageTypeHookLine:
		{
			S = "[LUA][HOOK_LINE] ";
			SS = "[LINE]        ";
			break;
		}
	case ScriptStorage::eLuaMessageTypeHookCount:
		{
			S = "[LUA][HOOK_COUNT] ";
			SS = "[COUNT]       ";
			break;
		}
	case ScriptStorage::eLuaMessageTypeHookTailReturn:
		{
			S = "[LUA][HOOK_TAIL_RETURN] ";
			SS = "[TAIL_RETURN] ";
			break;
		}
	default: NODEFAULT;
	}

	xr_strcpy(S2, S);
	S1 = S2 + xr_strlen(S);
	int l_iResult = vsprintf(S1, caFormat, marker);
	Msg("%s", S2);

	xr_strcpy(S2, SS);
	S1 = S2 + xr_strlen(SS);
	vsprintf(S1, caFormat, marker);
	xr_strcat(S2, "\r\n");

#ifdef LUA_DEBUG_PRINT //DEBUG
#	ifndef ENGINE_BUILD
        ai().script_engine().m_output.w(S2,xr_strlen(S2)*sizeof(char));
#	endif //!ENGINE_BUILD
#endif //-LUA_DEBUG_PRINT DEBUG

	return (l_iResult);
	//#endif //-PRINT_CALL_STACK
}

//#ifdef PRINT_CALL_STACK
void CScriptStorage::print_stack()
{
#ifdef DEBUG
    if (!m_stack_is_ready)
        return;

    m_stack_is_ready = false;
#endif //-DEBUG

	lua_State* L = lua();
	lua_Debug l_tDebugInfo;
	for (int i = 0; lua_getstack(L, i, &l_tDebugInfo); ++i)
	{
		lua_getinfo(L, "nSlu", &l_tDebugInfo);
		if (!l_tDebugInfo.name)
		{
			script_log_no_stack(ScriptStorage::eLuaMessageTypeError, "%2d : [%s] %s(%d) : %s", i, l_tDebugInfo.what,
			                    l_tDebugInfo.short_src, l_tDebugInfo.currentline, "");
			//script_log(ScriptStorage::eLuaMessageTypeError, "%2d : [%s] %s(%d) : %s", i, l_tDebugInfo.what, l_tDebugInfo.short_src, l_tDebugInfo.currentline, "");
		}
		else
		{
			if (!xr_strcmp(l_tDebugInfo.what, "C"))
			{
				script_log_no_stack(ScriptStorage::eLuaMessageTypeError, "%2d : [C  ] %s", i, l_tDebugInfo.name);
				//script_log(ScriptStorage::eLuaMessageTypeError, "%2d : [C  ] %s", i, l_tDebugInfo.name);  
			}
			else
			{
				script_log_no_stack(ScriptStorage::eLuaMessageTypeError, "%2d : [%s] %s(%d) : %s", i, l_tDebugInfo.what,
				                    l_tDebugInfo.short_src, l_tDebugInfo.currentline, l_tDebugInfo.name);
				//script_log(ScriptStorage::eLuaMessageTypeError, "%2d : [%s] %s(%d) : %s", i, l_tDebugInfo.what, l_tDebugInfo.short_src, l_tDebugInfo.currentline, l_tDebugInfo.name);
			}
		}
	}
}

//#endif //-PRINT_CALL_STACK

//AVO: added to stop duplicate stack output prints in log
int __cdecl CScriptStorage::script_log_no_stack(ScriptStorage::ELuaMessageType tLuaMessageType, LPCSTR caFormat, ...)
{
	va_list marker;
	va_start(marker, caFormat);
	int result = vscript_log(tLuaMessageType, caFormat, marker);
	va_end(marker);
	return result;
}

//-AVO

int __cdecl CScriptStorage::script_log(ScriptStorage::ELuaMessageType tLuaMessageType, LPCSTR caFormat, ...)
{
	va_list marker;
	va_start(marker, caFormat);
	int result = vscript_log(tLuaMessageType, caFormat, marker);
	va_end(marker);

	static bool reenterability = false;
	if (!reenterability)
	{
		reenterability = true;
		if (tLuaMessageType == ScriptStorage::eLuaMessageTypeError)
			ai().script_engine().print_stack();
		reenterability = false;
	}

	// #ifdef PRINT_CALL_STACK
	// #	ifndef ENGINE_BUILD
	//     static bool	reenterability = false;
	//     if (!reenterability)
	//     {
	//         reenterability = true;
	//         if (eLuaMessageTypeError == tLuaMessageType)
	//             ai().script_engine().print_stack();
	//         reenterability = false;
	//     }
	// #	endif //!ENGINE_BUILD
	// #endif //-PRINT_CALL_STACK

	return (result);
}

bool CScriptStorage::parse_namespace(LPCSTR caNamespaceName, LPSTR b, u32 const b_size, LPSTR c, u32 const c_size)
{
	*b = 0;
	*c = 0;
	LPSTR S2;
	STRCONCAT(S2, caNamespaceName);
	LPSTR S = S2;
	for (int i = 0;; ++i)
	{
		if (!xr_strlen(S))
		{
			script_log(ScriptStorage::eLuaMessageTypeError, "the namespace name %s is incorrect!", caNamespaceName);
			return (false);
		}
		LPSTR S1 = strchr(S, '.');
		if (S1)
			*S1 = 0;

		if (i)
			xr_strcat(b, b_size, "{");
		xr_strcat(b, b_size, S);
		xr_strcat(b, b_size, "=");
		if (i)
			xr_strcat(c, c_size, "}");
		if (S1)
			S = ++S1;
		else
			break;
	}

	return (true);
}

bool CScriptStorage::load_buffer(lua_State* L, LPCSTR caBuffer, size_t tSize, LPCSTR caScriptName,
                                 LPCSTR caNameSpaceName)
{
	int l_iErrorCode;
	if (caNameSpaceName && xr_strcmp("_G", caNameSpaceName))
	{
		string512 insert, a, b;

		LPCSTR header = file_header;

		if (!parse_namespace(caNameSpaceName, a, sizeof(a), b, sizeof(b)))
			return (false);

		xr_sprintf(insert, header, caNameSpaceName, a, b);
		u32 str_len = xr_strlen(insert);
		u32 const total_size = str_len + tSize;
		LPSTR script = 0;
		bool dynamic_allocation = false;

		__try
		{
			if (total_size < 768 * 1024)
				script = (LPSTR)_alloca(total_size);
			else
			{
#ifdef DEBUG
                script = (LPSTR)Memory.mem_alloc(total_size, "lua script file");
#else //!DEBUG
				script = (LPSTR)Memory.mem_alloc(total_size);
#endif //-DEBUG
				dynamic_allocation = true;
			}
		}
		__except (GetExceptionCode() == STATUS_STACK_OVERFLOW)
		{
			int errcode = _resetstkoflw();
			R_ASSERT2(errcode, "Could not reset the stack after \"Stack overflow\" exception!");
#ifdef DEBUG
            script					= (LPSTR)Memory.mem_alloc(total_size, "lua script file (after exception)");
#else //#ifdef DEBUG
			script = (LPSTR)Memory.mem_alloc(total_size);
#endif //#ifdef DEBUG
			dynamic_allocation = true;
		};

		xr_strcpy(script, total_size, insert);
		CopyMemory(script + str_len, caBuffer, u32(tSize));

		l_iErrorCode = luaL_loadbuffer(L, script, tSize + str_len, caScriptName);

		if (dynamic_allocation)
			xr_free(script);
	}
	else
	{
		//		try
		{
			l_iErrorCode = luaL_loadbuffer(L, caBuffer, tSize, caScriptName);
		}
		//		catch(...) {
		//			l_iErrorCode= LUA_ERRSYNTAX;
		//		}
	}

	if (l_iErrorCode)
	{
//#ifdef DEBUG
		if (strstr(Core.Params, "-dbg")) print_output(L,caScriptName,l_iErrorCode);
//#endif //-DEBUG
		on_error(L);
		return (false);
	}
	return (true);
}

xr_unordered_map<std::string, std::set<std::string>> unlocalizers;
bool unlocalizerPassed = false;

static std::string join_list(const std::vector<std::string>& items_vec, std::string delim = "\n") {
	std::string ret;
	for (const auto& i : items_vec) {
		if (!ret.empty()) {
			ret += delim;
		}
		ret += i;
	}
	return ret;
};

static bool unlocalRegex(std::set<std::string>& unlocals, std::string& s, const std::regex& pattern, const int group, const std::string& replacement) {
	if (std::regex_match(s, pattern)) {
		//Msg("matching local function pattern");
		std::smatch match;
		std::regex_search(s, match, pattern);
		std::string variable = match[group];
		if (unlocals.find(variable) != unlocals.end()) {
			Msg("[unlocalRegex] found variable %s to unlocal", variable.c_str());
			s = std::regex_replace(s, pattern, replacement);
			return true;
		}
	} else {
		return false;
	}
	return false;
};

bool CScriptStorage::do_file(LPCSTR caScriptName, LPCSTR caNameSpaceName)
{
	if (!unlocalizerPassed) {
		auto file_list = FS.file_list_open("$game_config$", "unlocalizers\\", FS_RootOnly | FS_ListFiles);
		if (!file_list) {
			unlocalizerPassed = true;
		} else {
			xr_string id;
			auto i = file_list->begin();
			auto e = file_list->end();
			for (; i != e; ++i)
			{
				u32 length = xr_strlen(*i);

				if (!((length >= 4) &&
					((*i)[length - 4] == '.') &&
					((*i)[length - 3] == 'l') &&
					((*i)[length - 2] == 't') &&
					((*i)[length - 1] == 'x')))
					continue;

				id.assign(*i, length - 4);

				string_path file_name;
				FS.update_path(file_name, "$game_config$", (xr_string("unlocalizers\\") + id).c_str());
				xr_strcat(file_name, ".ltx");

				Msg("opening file %s", file_name);
				auto config = xr_new<CInifile>(file_name);

				typedef CInifile::Root sections_type;
				sections_type& sections = config->sections();

				sections_type::const_iterator i = sections.begin();
				sections_type::const_iterator e = sections.end();
				for (; i != e; ++i)
				{
					auto sectionName = std::string((*i)->Name.c_str());
					toLowerCase(sectionName);
					if (unlocalizers.find(sectionName) == unlocalizers.end()) {

						// construct set that contains top level variables to delocalize by section name
						unlocalizers[sectionName].clear();
						Msg("creating unlocalizer for script %s", sectionName.c_str());
					}
					auto& data = (*i)->Data;
					for (auto& item : data) {
						unlocalizers[sectionName].insert(std::string(item.first.c_str()));
						Msg("adding variable %s for unlocalizer for script %s", item.first.c_str(), sectionName.c_str());
					}
				}
				xr_delete(config);
			}
			FS.file_list_close(file_list);
			unlocalizerPassed = true;
		}
	}
	int start = lua_gettop(lua());
	string_path l_caLuaFileName;
	IReader* l_tpFileReader = FS.r_open(caScriptName);

	if (!l_tpFileReader)
	{
		script_log(eLuaMessageTypeError, "Cannot open file \"%s\"", caScriptName);
		return (false);
	}

	// Unlocalize variables in the script defined by unlocalizers map
	auto scriptContents = static_cast<LPCSTR>(l_tpFileReader->pointer());
	auto scriptLength = (size_t)l_tpFileReader->length();
	bool unlocalPerformed = false;
	std::string unlocalizerResult;
	std::string loweredNameSpaceName = caNameSpaceName;
	toLowerCase(loweredNameSpaceName);
	if (unlocalizers.find(loweredNameSpaceName) != unlocalizers.end()) {
		Msg("found script %s in unlocalizers data", caNameSpaceName);

		// Get contents of the script file and split by lines
		std::vector<std::string> tokens;
		std::string temp;
		while (!l_tpFileReader->eof())
		{
			char c = l_tpFileReader->r_u8();
			temp += c;
		}

		std::stringstream stringStream(temp);
		std::string line;
		tokens.clear();
		while (std::getline(stringStream, line)) {
			tokens.push_back(line);
		}

		// Iterate lines and unlocalize variables
		auto& unlocals = unlocalizers[loweredNameSpaceName];

		/*for (auto& u : unlocals) {
			Msg("%s", u);
		}*/

		for (auto& s : tokens) {

			//Msg("%s", s.c_str());

			trim(s, "\n\r");
			if (s.empty()) {
				continue;
			}

			std::regex pattern;

			//local function x(a,b,c)
			pattern = std::regex(R"((^local)([\t ]+)(function)([\t ]+)([_a-zA-Z].*)([\t ]*)(\(.*$))");
			if (unlocalRegex(unlocals, s, pattern, 5, "$3$4$5$6$7")) {
				unlocalPerformed = true;
				continue;
			}

			//local a = ...
			//local a
			//local a,b,c = ... (if one of a,b,c is in unlocalizers list - all of them will be unlocalized)
			//local x; local y; - unsupported yet
			pattern = std::regex(R"((^local)([\t ]+)(.*))");
			if (std::regex_match(s, pattern)) {
				std::smatch match;
				std::regex_search(s, match, pattern);
				std::string m = match[3];

				// strip comments
				std::regex r = std::regex(R"((.*)--.*)");
				if (std::regex_match(m, r)) {
					//Msg("found comments\n");
					std::smatch noncomments;
					std::regex_search(m, noncomments, r);
					m = noncomments[1];
				}

				auto variablesAndValues = splitStringLimit(m, "=", 1);
				bool hasValue = variablesAndValues.size() > 1;
				auto variables = splitStringMulti(variablesAndValues[0], ",");
				for (auto v : variables) {
					trim(v);
					//Msg("%s\n", v.c_str());
					if (unlocals.find(v) != unlocals.end()) {
						unlocalPerformed = true;
						Msg("found variable %s to unlocal", v.c_str());
						s = std::regex_replace(s, pattern, "$3");
						if (!hasValue) {

							// strip comments
							std::regex r = std::regex(R"((.*)(--.*))");
							if (std::regex_match(s, r)) {
								//Msg("found comments\n");
								std::smatch noncomments;
								std::regex_search(s, noncomments, r);
								s = std::string(noncomments[1]) + "= nil " + std::string(noncomments[2]);
							} else {
								s += " = nil";
							}
						}
						break;
					}
				}
			}
		}

		// Store result back
		/*for (auto& s : tokens) {
			Msg("%s", s.c_str());
		}*/

		unlocalizerResult = join_list(tokens);
		scriptContents = unlocalizerResult.c_str();
		scriptLength = strlen(scriptContents);
	}

	strconcat(sizeof(l_caLuaFileName), l_caLuaFileName, "@", caScriptName);

	bool bufferLoaded = false;
	if (unlocalPerformed) {
		bufferLoaded = load_buffer(lua(), scriptContents, scriptLength, l_caLuaFileName, caNameSpaceName);
	} else {
		l_tpFileReader->rewind();
		bufferLoaded = load_buffer(lua(), static_cast<LPCSTR>(l_tpFileReader->pointer()), (size_t)l_tpFileReader->length(), l_caLuaFileName, caNameSpaceName);
	}

	if (!bufferLoaded)
	{
		//		VERIFY		(lua_gettop(lua()) >= 4);
		//		lua_pop		(lua(),4);
		//		VERIFY		(lua_gettop(lua()) == start - 3);
		lua_settop(lua(), start);
		FS.r_close(l_tpFileReader);
		return (false);
	}
	FS.r_close(l_tpFileReader);

	int errFuncId = -1;
#ifdef USE_DEBUGGER
#	ifndef USE_LUA_STUDIO
    if( ai().script_engine().debugger() )
        errFuncId = ai().script_engine().debugger()->PrepareLua(lua());
#	endif // #ifndef USE_LUA_STUDIO
#endif // #ifdef USE_DEBUGGER
	if (0) //.
	{
		for (int i = 0; lua_type(lua(), -i - 1); i++)
			Msg("%2d : %s", -i - 1, lua_typename(lua(), lua_type(lua(), -i - 1)));
	}

	// because that's the first and the only call of the main chunk - there is no point to compile it
	//	luaJIT_setmode	(lua(),0,LUAJIT_MODE_ENGINE|LUAJIT_MODE_OFF);						// Oles
	int l_iErrorCode = lua_pcall(lua(), 0, 0, (-1 == errFuncId) ? 0 : errFuncId); // new_Andy
	//	luaJIT_setmode	(lua(),0,LUAJIT_MODE_ENGINE|LUAJIT_MODE_ON);						// Oles

#ifdef USE_DEBUGGER
#	ifndef USE_LUA_STUDIO
    if( ai().script_engine().debugger() )
        ai().script_engine().debugger()->UnPrepareLua(lua(),errFuncId);
#	endif // #ifndef USE_LUA_STUDIO
#endif // #ifdef USE_DEBUGGER
	if (l_iErrorCode)
	{
//#ifdef DEBUG
		if (strstr(Core.Params, "-dbg")) print_output(lua(),caScriptName,l_iErrorCode);
//#endif
		on_error(lua());
		lua_settop(lua(), start);
		return (false);
	}

	return (true);
}

bool CScriptStorage::load_file_into_namespace(LPCSTR caScriptName, LPCSTR caNamespaceName)
{
	int start = lua_gettop(lua());
	if (!do_file(caScriptName, caNamespaceName))
	{
		Msg("! [ERROR] --- Failed to load script %s", caNamespaceName);
		lua_settop(lua(), start);
		return (false);
	}
	VERIFY(lua_gettop(lua()) == start);
	return (true);
}

bool CScriptStorage::namespace_loaded(LPCSTR N, bool remove_from_stack)
{
	int start = lua_gettop(lua());
	lua_pushstring(lua(), "_G");
	lua_rawget(lua(), LUA_GLOBALSINDEX);
	string256 S2;
	xr_strcpy(S2, N);
	LPSTR S = S2;
	for (;;)
	{
		if (!xr_strlen(S))
		{
			VERIFY(lua_gettop(lua()) >= 1);
			lua_pop(lua(), 1);
			VERIFY(start == lua_gettop(lua()));
			return (false);
		}
		LPSTR S1 = strchr(S, '.');
		if (S1)
			*S1 = 0;
		lua_pushstring(lua(), S);
		lua_rawget(lua(), -2);
		if (lua_isnil(lua(), -1))
		{
			//			lua_settop		(lua(),0);
			VERIFY(lua_gettop(lua()) >= 2);
			lua_pop(lua(), 2);
			VERIFY(start == lua_gettop(lua()));
			return (false); //	there is no namespace!
		}
		else if (!lua_istable(lua(), -1))
		{
			//				lua_settop	(lua(),0);
			VERIFY(lua_gettop(lua()) >= 1);
			lua_pop(lua(), 1);
			VERIFY(start == lua_gettop(lua()));
			FATAL(" Error : the namespace name is already being used by the non-table object!\n");
			return (false);
		}
		lua_remove(lua(), -2);
		if (S1)
			S = ++S1;
		else
			break;
	}
	if (!remove_from_stack)
	{
		VERIFY(lua_gettop(lua()) == start + 1);
	}
	else
	{
		VERIFY(lua_gettop(lua()) >= 1);
		lua_pop(lua(), 1);
		VERIFY(lua_gettop(lua()) == start);
	}
	return (true);
}

bool CScriptStorage::object(LPCSTR identifier, int type)
{
	int start = lua_gettop(lua());
	lua_pushnil(lua());
	while (lua_next(lua(), -2))
	{
		if ((lua_type(lua(), -1) == type) && !xr_strcmp(identifier, lua_tostring(lua(), -2)))
		{
			VERIFY(lua_gettop(lua()) >= 3);
			lua_pop(lua(), 3);
			VERIFY(lua_gettop(lua()) == start - 1);
			return (true);
		}
		lua_pop(lua(), 1);
	}
	VERIFY(lua_gettop(lua()) >= 1);
	lua_pop(lua(), 1);
	VERIFY(lua_gettop(lua()) == start - 1);
	return (false);
}

bool CScriptStorage::object(LPCSTR namespace_name, LPCSTR identifier, int type)
{
	int start = lua_gettop(lua());
	if (xr_strlen(namespace_name) && !namespace_loaded(namespace_name, false))
	{
		VERIFY(lua_gettop(lua()) == start);
		return (false);
	}
	bool result = object(identifier, type);
	VERIFY(lua_gettop(lua()) == start);
	return (result);
}

::luabind::object CScriptStorage::name_space(LPCSTR namespace_name)
{
	string256 S1;
	xr_strcpy(S1, namespace_name);
	LPSTR S = S1;
	::luabind::object lua_namespace = ::luabind::get_globals(lua());
	for (;;)
	{
		if (!xr_strlen(S))
			return (lua_namespace);
		LPSTR I = strchr(S, '.');
		if (!I)
			return (lua_namespace[S]);
		*I = 0;
		lua_namespace = lua_namespace[S];
		S = I + 1;
	}
}

#include <boost/noncopyable.hpp>

struct raii_guard : private boost::noncopyable
{
	int m_error_code;
	LPCSTR const& m_error_description;

	raii_guard(int error_code, LPCSTR const& m_description) : m_error_code(error_code),
	                                                          m_error_description(m_description)
	{
	}

	~raii_guard()
	{
#ifdef DEBUG
        bool lua_studio_connected = !!ai().script_engine().debugger();
        if (!lua_studio_connected)
#endif //-DEBUG
		{
#ifdef DEBUG
            static bool const break_on_assert	= !!strstr(Core.Params,"-break_on_assert");
#else //!DEBUG
			static bool const break_on_assert = false; //Alundaio: Can't get a proper stack trace with this enabled
#endif //-DEBUG
			if (!m_error_code)
				return;

			if (break_on_assert)
				R_ASSERT2(!m_error_code, m_error_description);
			else
				Msg("! [SCRIPT ERROR]: %s", m_error_description);
		}
	}
}; //-struct raii_guard

bool CScriptStorage::print_output(lua_State* L, LPCSTR caScriptFileName, int iErorCode)
{
	if (iErorCode)
		print_error(L, iErorCode);

	LPCSTR S = "see call_stack for details!";

	raii_guard guard(iErorCode, S);

	if (!lua_isstring(L, -1))
		return (false);

	S = lua_tostring(L, -1);
	if (!xr_strcmp(S, "cannot resume dead coroutine"))
	{
		VERIFY2("Please do not return any values from main!!!", caScriptFileName);
#ifdef USE_DEBUGGER
#	ifndef USE_LUA_STUDIO
        if(ai().script_engine().debugger() && ai().script_engine().debugger()->Active() ){
            ai().script_engine().debugger()->Write(S);
            ai().script_engine().debugger()->ErrorBreak();
        }
#	endif //!USE_LUA_STUDIO
#endif //-USE_DEBUGGER
	}
	else
	{
		if (!iErorCode)
			script_log(ScriptStorage::eLuaMessageTypeInfo, "Output from %s", caScriptFileName);
		script_log(iErorCode ? ScriptStorage::eLuaMessageTypeError : ScriptStorage::eLuaMessageTypeMessage, "%s", S);
#ifdef USE_DEBUGGER
#	ifndef USE_LUA_STUDIO
        if (ai().script_engine().debugger() && ai().script_engine().debugger()->Active()) {
            ai().script_engine().debugger()->Write		(S);
            ai().script_engine().debugger()->ErrorBreak	();
        }
#	endif //!USE_LUA_STUDIO
#endif //-USE_DEBUGGER
	}
	return (true);
}

void CScriptStorage::print_error(lua_State* L, int iErrorCode)
{
	switch (iErrorCode)
	{
	case LUA_ERRRUN:
		{
			script_log(ScriptStorage::eLuaMessageTypeError, "SCRIPT RUNTIME ERROR");
			break;
		}
	case LUA_ERRMEM:
		{
			script_log(ScriptStorage::eLuaMessageTypeError, "SCRIPT ERROR (memory allocation)");
			break;
		}
	case LUA_ERRERR:
		{
			script_log(ScriptStorage::eLuaMessageTypeError, "SCRIPT ERROR (while running the error handler function)");
			break;
		}
	case LUA_ERRFILE:
		{
			script_log(ScriptStorage::eLuaMessageTypeError, "SCRIPT ERROR (while running file)");
			break;
		}
	case LUA_ERRSYNTAX:
		{
			script_log(ScriptStorage::eLuaMessageTypeError, "SCRIPT SYNTAX ERROR");
			break;
		}
	case LUA_YIELD:
		{
			script_log(ScriptStorage::eLuaMessageTypeInfo, "Thread is yielded");
			break;
		}
	default: NODEFAULT;
	}
}

#ifdef LUA_DEBUG_PRINT //DEBUG
void CScriptStorage::flush_log()
{
    string_path			log_file_name;
    strconcat           (sizeof(log_file_name),log_file_name,Core.ApplicationName,"_",Core.UserName,"_lua.log");
    FS.update_path      (log_file_name,"$logs$",log_file_name);
    m_output.save_to	(log_file_name);
}
#endif //-LUA_DEBUG_PRINT DEBUG

int CScriptStorage::error_log(LPCSTR format, ...)
{
	va_list marker;
	va_start(marker, format);

	LPCSTR S = "! [LUA][ERROR] ";
	LPSTR S1;
	string4096 S2;
	xr_strcpy(S2, S);
	S1 = S2 + xr_strlen(S);

	int result = vsprintf(S1, format, marker);
	va_end(marker);

	Msg("%s", S2);

	return (result);
}

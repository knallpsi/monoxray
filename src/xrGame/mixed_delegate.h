#ifndef MIXED_DELEGATE_INCLUDED
#define MIXED_DELEGATE_INCLUDED

#include "../xrCore/fastdelegate.h"
#include "pch_script.h"
#include "script_export_space.h"
#include "script_callback_ex.h"
#include "mixed_delegate_unique_tags.h"

template <typename Signature, int UniqueTag = mdut_no_unique_tag>
class mixed_delegate;

template <typename R, class Param1, class Param2, int UniqueTag>
class mixed_delegate<R (Param1, Param2), UniqueTag>
{
public:
	typedef R return_type;
	typedef Param1 param1_type;
	typedef Param2 param2_type;

	typedef fastdelegate::FastDelegate<R (Param1, Param2)> fastdelegate_type;
	typedef CScriptCallbackEx<R> lua_delegate_type;
	typedef ::luabind::object lua_object_type;
	typedef ::luabind::functor<R> lua_function_type;

	mixed_delegate()
	{
	};

	~mixed_delegate()
	{
	}

	template <class ThisRef, class ClassType>
	mixed_delegate(ThisRef* ptr_this, R (xr_stdcall ClassType::*func_ptr)(Param1, Param2)) :
		m_cpp_delegate(ptr_this, func_ptr)
	{
	};

	mixed_delegate(lua_object_type ptr_this, lua_function_type func_ptr)
	{
		m_lua_delegate.set(func_ptr, ptr_this);
	}

	mixed_delegate(mixed_delegate const& copy) :
		m_cpp_delegate(copy.m_cpp_delegate),
		m_lua_delegate(copy.m_lua_delegate)

	{
	};

	template <class ThisRef, class ClassType>
	void bind(ThisRef* ptr_this,
	          R (xr_stdcall ClassType::*func_ptr)(Param1, Param2))
	{
		m_cpp_delegate.bind(ptr_this, func_ptr);
	}

	void bind(lua_object_type ptr_this, lua_function_type func_ptr)
	{
		m_lua_delegate.set(func_ptr, ptr_this);
	}

	void clear()
	{
		if (m_cpp_delegate)
		{
			m_cpp_delegate.clear();
		}
		if (m_lua_delegate)
		{
			m_lua_delegate.clear();
		}
	}

	R operator ()(Param1 arg1, Param2 arg2)
	{
		if (m_cpp_delegate)
		{
			return m_cpp_delegate.operator()(arg1, arg2);
		}
		if (m_lua_delegate)
		{
			return m_lua_delegate.operator()(arg1, arg2);
		}
		FATAL("mixed delegate is not bound");
		return R();
	}

	bool operator !() const
	{
		return ! operator bool();
	}

	operator bool() const
	{
		if (m_cpp_delegate)
			return true;
		if (m_lua_delegate)
			return true;
		return false;
	}

private:
	fastdelegate_type m_cpp_delegate;
	lua_delegate_type m_lua_delegate;
DECLARE_SCRIPT_REGISTER_FUNCTION
}; //class mixed_delegate


#define DEFINE_MIXED_DELEGATE_SCRIPT(type, name_str) \
	void type::script_register(lua_State *L)\
	{\
		module(L)\
		[\
			class_<type>(name_str)\
				.def(						constructor<>())\
				.def(						constructor<type::lua_object_type, type::lua_function_type>())\
				.def("bind",				&type::bind)\
				.def("clear",				&type::clear)\
		];\
	};

#endif //#ifndef MIXED_DELEGATE_INCLUDED

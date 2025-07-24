////////////////////////////////////////////////////////////////////////////
//	Module 		: script_fvector_script.cpp
//	Created 	: 28.06.2004
//  Modified 	: 28.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Script float vector script export
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "script_fvector.h"

using namespace luabind;

#pragma optimize("s",on)
void CScriptFvector::script_register(lua_State* L)
{
	module(L)
	[
		class_<Fvector>("vector")
		// demonized: new exports of static functions, in Lua use like this: vector.generate_orthonormal_basis(a, b, c)
		.scope[
			def("generate_orthonormal_basis", &Fvector::generate_orthonormal_basis),
			def("generate_orthonormal_basis_normalized", &Fvector::generate_orthonormal_basis_normalized)
		]
		.def_readwrite("x", &Fvector::x)
		.def_readwrite("y", &Fvector::y)
		.def_readwrite("z", &Fvector::z)
		.def(constructor<>())
		.def("set", (Fvector & (Fvector::*)(float, float, float))(&Fvector::set), return_reference_to(_1))
		.def("set", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::set), return_reference_to(_1))
		.def("add", (Fvector & (Fvector::*)(float))(&Fvector::add), return_reference_to(_1))
		.def("add", (Fvector & (Fvector::*)(float, float, float))(&Fvector::add), return_reference_to(_1))
		.def("add", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::add), return_reference_to(_1))
		.def("add", (Fvector & (Fvector::*)(const Fvector&, const Fvector&))(&Fvector::add), return_reference_to(_1))
		.def("add", (Fvector & (Fvector::*)(const Fvector&, float))(&Fvector::add), return_reference_to(_1))
		.def("sub", (Fvector & (Fvector::*)(float))(&Fvector::sub), return_reference_to(_1))
		.def("sub", (Fvector & (Fvector::*)(float, float, float))(&Fvector::sub), return_reference_to(_1))
		.def("sub", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::sub), return_reference_to(_1))
		.def("sub", (Fvector & (Fvector::*)(const Fvector&, const Fvector&))(&Fvector::sub), return_reference_to(_1))
		.def("sub", (Fvector & (Fvector::*)(const Fvector&, float))(&Fvector::sub), return_reference_to(_1))
		.def("mul", (Fvector & (Fvector::*)(float))(&Fvector::mul), return_reference_to(_1))
		.def("mul", (Fvector & (Fvector::*)(float, float, float))(&Fvector::mul), return_reference_to(_1))
		.def("mul", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::mul), return_reference_to(_1))
		.def("mul", (Fvector & (Fvector::*)(const Fvector&, const Fvector&))(&Fvector::mul), return_reference_to(_1))
		.def("mul", (Fvector & (Fvector::*)(const Fvector&, float))(&Fvector::mul), return_reference_to(_1))
		.def("div", (Fvector & (Fvector::*)(float))(&Fvector::div), return_reference_to(_1))
		.def("div", (Fvector & (Fvector::*)(float, float, float))(&Fvector::div), return_reference_to(_1))
		.def("div", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::div), return_reference_to(_1))
		.def("div", (Fvector & (Fvector::*)(const Fvector&, const Fvector&))(&Fvector::div), return_reference_to(_1))
		.def("div", (Fvector & (Fvector::*)(const Fvector&, float))(&Fvector::div), return_reference_to(_1))
		.def("invert", (Fvector & (Fvector::*)())(&Fvector::invert), return_reference_to(_1))
		.def("invert", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::invert), return_reference_to(_1))
		.def("min", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::min), return_reference_to(_1))
		.def("min", (Fvector & (Fvector::*)(const Fvector&, const Fvector&))(&Fvector::min), return_reference_to(_1))
		.def("max", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::max), return_reference_to(_1))
		.def("max", (Fvector & (Fvector::*)(const Fvector&, const Fvector&))(&Fvector::max), return_reference_to(_1))
		.def("abs", &Fvector::abs, return_reference_to(_1))
		.def("similar", &Fvector::similar)
		.def("set_length", &Fvector::set_length, return_reference_to(_1))
		.def("align", &Fvector::align, return_reference_to(_1))
		//			.def("squeeze",						&Fvector::squeeze,																										return_reference_to(_1))
		.def("clamp", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::clamp), return_reference_to(_1))
		.def("clamp", (Fvector & (Fvector::*)(const Fvector&, const Fvector&))(&Fvector::clamp),
		     return_reference_to(_1))
		.def("inertion", &Fvector::inertion, return_reference_to(_1))
		.def("average", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::average), return_reference_to(_1))
		.def("average", (Fvector & (Fvector::*)(const Fvector&, const Fvector&))(&Fvector::average),
		     return_reference_to(_1))
		.def("lerp", &Fvector::lerp, return_reference_to(_1))
		.def("mad", (Fvector & (Fvector::*)(const Fvector&, float))(&Fvector::mad), return_reference_to(_1))
		.def("mad", (Fvector & (Fvector::*)(const Fvector&, const Fvector&, float))(&Fvector::mad),
		     return_reference_to(_1))
		.def("mad", (Fvector & (Fvector::*)(const Fvector&, const Fvector&))(&Fvector::mad), return_reference_to(_1))
		.def("mad", (Fvector & (Fvector::*)(const Fvector&, const Fvector&, const Fvector&))(&Fvector::mad),
		     return_reference_to(_1))
		//			.def("square_magnitude",			&Fvector::square_magnitude)
		.def("magnitude", &Fvector::magnitude)
		//			.def("normalize_magnitude",			&Fvector::normalize_magn)
		.def("normalize", (Fvector & (Fvector::*)())(&Fvector::normalize_safe), return_reference_to(_1))
		.def("normalize", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::normalize_safe), return_reference_to(_1))
		.def("normalize_safe", (Fvector & (Fvector::*)())(&Fvector::normalize_safe), return_reference_to(_1))
		.def("normalize_safe", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::normalize_safe),
		     return_reference_to(_1))
		//			.def("random_dir",					(Fvector & (Fvector::*)())(&Fvector::random_dir),																		return_reference_to(_1))
		//			.def("random_dir",					(Fvector & (Fvector::*)(const Fvector &, float))(&Fvector::random_dir),													return_reference_to(_1))
		//			.def("random_point",				(Fvector & (Fvector::*)(const Fvector &))(&Fvector::random_point),														return_reference_to(_1))
		//			.def("random_point",				(Fvector & (Fvector::*)(float))(&Fvector::random_point),																return_reference_to(_1))
		.def("dotproduct", &Fvector::dotproduct)
		.def("crossproduct", &Fvector::crossproduct, return_reference_to(_1))
		.def("distance_to_xz", &Fvector::distance_to_xz)
		.def("distance_to_xz_sqr", &Fvector::distance_to_xz_sqr)
		.def("distance_to_sqr", &Fvector::distance_to_sqr)
		.def("distance_to", &Fvector::distance_to)
		//			.def("from_bary",					(Fvector & (Fvector::*)(const Fvector &, const Fvector &, const Fvector &, float, float, float))(&Fvector::from_bary),	return_reference_to(_1))
		//			.def("from_bary",					(Fvector & (Fvector::*)(const Fvector &, const Fvector &, const Fvector &, const Fvector &))(&Fvector::from_bary),		return_reference_to(_1))
		//			.def("from_bary4",					&Fvector::from_bary4,																									return_reference_to(_1))
		//			.def("mknormal_non_normalized",		&Fvector::mknormal_non_normalized,																						return_reference_to(_1))
		//			.def("mknormal",					&Fvector::mknormal,																										return_reference_to(_1))
		.def("setHP", &Fvector::setHP, return_reference_to(_1))
		//			.def("getHP",						&Fvector::getHP,																																	out_value(_2) + out_value(_3))
		.def("getH", &Fvector::getH)
		.def("getP", &Fvector::getP)

		.def("reflect", &Fvector::reflect, return_reference_to(_1))
		.def("slide", &Fvector::slide, return_reference_to(_1))

		// demonized: new exports
		.def("project", (Fvector & (Fvector::*)(const Fvector&, const Fvector&))(&Fvector::project), return_reference_to(_1))
		.def("project", (Fvector & (Fvector::*)(const Fvector&))(&Fvector::project), return_reference_to(_1))
		.def("hud_to_world", &Fvector::hud_to_world, return_reference_to(_1))
		.def("world_to_hud", &Fvector::world_to_hud, return_reference_to(_1))
		.def("hud_to_world_dir", &Fvector::hud_to_world_dir, return_reference_to(_1))
		.def("world_to_hud_dir", &Fvector::world_to_hud_dir, return_reference_to(_1)),

		class_<Fvector2>("vector2")
		.def_readwrite("x", &Fvector2::x)
		.def_readwrite("y", &Fvector2::y)
		.def(constructor<>())

		// demonized: new exports
		.def("normalize", (Fvector2 & (Fvector2::*)(void))(&Fvector2::normalize), return_reference_to(_1))

		.def("set", (Fvector2 & (Fvector2::*)(float, float))(&Fvector2::set), return_reference_to(_1))
		.def("set", (Fvector2 & (Fvector2::*)(const Fvector2&))(&Fvector2::set), return_reference_to(_1)),

		class_<Fvector4>("vector4")
		.def_readwrite("x", &Fvector4::x)
		.def_readwrite("y", &Fvector4::y)
		.def_readwrite("z", &Fvector4::z)
		.def_readwrite("w", &Fvector4::w)
		.def(constructor<>())
		.def("set", (Fvector4& (Fvector4::*)(float, float, float, float))(&Fvector4::set), return_reference_to(_1))
		.def("set", (Fvector4& (Fvector4::*)(const Fvector4&))(&Fvector4::set), return_reference_to(_1))
		.def("add", (Fvector4& (Fvector4::*)(float, float, float, float))(&Fvector4::add), return_reference_to(_1))
		.def("add", (Fvector4& (Fvector4::*)(const Fvector4&))(&Fvector4::add), return_reference_to(_1))
		.def("add", (Fvector4& (Fvector4::*)(float))(&Fvector4::add), return_reference_to(_1))
		.def("add", (Fvector4& (Fvector4::*)(const Fvector4&, const Fvector4&))(&Fvector4::add), return_reference_to(_1))
		.def("add", (Fvector4& (Fvector4::*)(const Fvector4&, float))(&Fvector4::add), return_reference_to(_1))
		.def("sub", (Fvector4& (Fvector4::*)(float, float, float, float))(&Fvector4::sub), return_reference_to(_1))
		.def("sub", (Fvector4& (Fvector4::*)(const Fvector4&))(&Fvector4::sub), return_reference_to(_1))
		.def("sub", (Fvector4& (Fvector4::*)(float))(&Fvector4::sub), return_reference_to(_1))
		.def("sub", (Fvector4& (Fvector4::*)(const Fvector4&, const Fvector4&))(&Fvector4::sub), return_reference_to(_1))
		.def("sub", (Fvector4& (Fvector4::*)(const Fvector4&, float))(&Fvector4::sub), return_reference_to(_1))
		.def("mul", (Fvector4& (Fvector4::*)(float, float, float, float))(&Fvector4::mul), return_reference_to(_1))
		.def("mul", (Fvector4& (Fvector4::*)(const Fvector4&))(&Fvector4::mul), return_reference_to(_1))
		.def("mul", (Fvector4& (Fvector4::*)(float))(&Fvector4::mul), return_reference_to(_1))
		.def("mul", (Fvector4& (Fvector4::*)(const Fvector4&, const Fvector4&))(&Fvector4::mul), return_reference_to(_1))
		.def("mul", (Fvector4& (Fvector4::*)(const Fvector4&, float))(&Fvector4::mul), return_reference_to(_1))
		.def("div", (Fvector4& (Fvector4::*)(float, float, float, float))(&Fvector4::div), return_reference_to(_1))
		.def("div", (Fvector4& (Fvector4::*)(const Fvector4&))(&Fvector4::div), return_reference_to(_1))
		.def("div", (Fvector4& (Fvector4::*)(float))(&Fvector4::div), return_reference_to(_1))
		.def("div", (Fvector4& (Fvector4::*)(const Fvector4&, const Fvector4&))(&Fvector4::div), return_reference_to(_1))
		.def("div", (Fvector4& (Fvector4::*)(const Fvector4&, float))(&Fvector4::div), return_reference_to(_1))
		.def("clamp", (Fvector4& (Fvector4::*)(const Fvector4&, const Fvector4&))(&Fvector4::clamp), return_reference_to(_1))
		.def("clamp", (Fvector4& (Fvector4::*)(const Fvector4&))(&Fvector4::clamp), return_reference_to(_1))
		.def("similar", &Fvector4::similar)
		.def("magnitude", &Fvector4::magnitude)
		.def("normalize", &Fvector4::normalize, return_reference_to(_1))
		.def("normalize_as_plane", &Fvector4::normalize_as_plane, return_reference_to(_1))
		.def("lerp", &Fvector4::lerp, return_reference_to(_1)),

		class_<Fbox>("Fbox")
		.def_readwrite("min", &Fbox::min)
		.def_readwrite("max", &Fbox::max)
		.def(constructor<>()),

		class_<Frect>("Frect")
		.def(constructor<>())
		.def("set", (Frect & (Frect::*)(float, float, float, float))(&Frect::set), return_reference_to(_1))
		.def_readwrite("lt", &Frect::lt)
		.def_readwrite("rb", &Frect::rb)
		.def_readwrite("x1", &Frect::x1)
		.def_readwrite("x2", &Frect::x2)
		.def_readwrite("y1", &Frect::y1)
		.def_readwrite("y2", &Frect::y2)

	];
}

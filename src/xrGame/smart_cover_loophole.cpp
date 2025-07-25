////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_loophole.cpp
//	Created 	: 16.08.2007
//	Author		: Alexander Dudin
//	Description : Loophole class for smart cover
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "smart_cover_loophole.h"
#include "object_broker.h"
#include "smart_cover_object.h"

using smart_cover::loophole;
using smart_cover::action;
using smart_cover::detail::parse_string;
using smart_cover::detail::parse_float;
using smart_cover::detail::parse_table;
using smart_cover::detail::parse_fvector;
using smart_cover::detail::parse_bool;

namespace smart_cover
{
	shared_str transform_vertex(shared_str const& vertex_id, bool const& in);
	shared_str parse_vertex(::luabind::object const& table, LPCSTR identifier, bool const& in);
} // namespace smart_cover

class id_predicate
{
	shared_str m_id;

public:
	IC id_predicate(shared_str const& id) :
		m_id(id)
	{
	}

	IC bool operator()(std::pair<shared_str, action*> const& other) const
	{
		return (m_id._get() == other.first._get());
	}
};


loophole::loophole(::luabind::object const& description) :
	m_fov(0.f),
	m_range(0.f)
{
	VERIFY2(description.type() == LUA_TTABLE, "invalid loophole description passed");

	m_id = parse_string(description, "id");

	m_usable = parse_bool(description, "usable");

	m_fov_position = parse_fvector(description, "fov_position");

	m_fov_direction = parse_fvector(description, "fov_direction");
	if (m_fov_direction.square_magnitude() < EPS_L)
	{
		Msg("! fov direction for loophole %s is setup incorrectly", m_id.c_str());
		m_fov_direction.set(0.f, 0.f, 1.f);
	}
	else
		m_fov_direction.normalize();

	m_danger_fov_direction = parse_fvector(description, "danger_fov_direction");
	if (m_danger_fov_direction.square_magnitude() < EPS_L)
	{
		Msg("! danger fov direction for loophole %s is setup incorrectly", m_id.c_str());
		m_danger_fov_direction.set(0.f, 0.f, 1.f);
	}
	else
		m_danger_fov_direction.normalize();

	m_enter_direction = parse_fvector(description, "enter_direction");

	if (m_enter_direction.square_magnitude() < EPS_L)
	{
		Msg("! enter direction for loophole %s is setup incorrectly", m_id.c_str());
		m_enter_direction.set(0.f, 0.f, 1.f);
	}
	else
		m_enter_direction.normalize();

	::luabind::object actions;
	parse_table(description, "actions", actions);

	typedef ::luabind::object::iterator iterator;
	iterator I = actions.begin();
	iterator E = actions.end();
	for (; I != E; ++I)
	{
		VERIFY(I.key().type() == LUA_TSTRING);
		LPCSTR action_type = ::luabind::object_cast<LPCSTR>(I.key());
		::luabind::object table = *I;
		if (table.type() != LUA_TTABLE)
		{
			VERIFY(table.type() != LUA_TNIL);
			continue;
		}
		add_action(action_type, *I);
	}

	m_usable = m_actions.empty() ? false : true;

	if (!m_usable)
		return;

	::luabind::object transitions;
	parse_table(description, "transitions", transitions);

	fill_transitions(transitions);

	m_fov = deg2rad(parse_float(description, "fov", 0.f, 360.f));
	m_danger_fov = deg2rad(parse_float(description, "danger_fov", 0.f, 360.f));
	m_range = parse_float(description, "range", 0.f);
}

void loophole::add_action(LPCSTR type, ::luabind::object const& table)
{
	VERIFY(table.type() == LUA_TTABLE);
	smart_cover::action* action = xr_new<smart_cover::action>(table);
	VERIFY(
		std::find_if(
			m_actions.begin(),
			m_actions.end(),
			id_predicate(type)
		) ==
		m_actions.end()
	);
	m_actions.insert(std::make_pair(type, action));
}

loophole::~loophole()
{
	delete_data(m_actions);
	delete_data(m_transitions);
}

void loophole::fill_transitions(::luabind::object const& transitions_table)
{
	VERIFY2(transitions_table.type() == LUA_TTABLE, "invalid loophole description passed");
	::luabind::object::iterator I = transitions_table.begin();
	::luabind::object::iterator E = transitions_table.end();
	for (; I != E; ++I)
	{
		::luabind::object table = *I;
		VERIFY2(table.type() == LUA_TTABLE, "invalid loophole description passed");
		shared_str action_from = parse_vertex(table, "action_from", true);
		shared_str action_to = parse_vertex(table, "action_to", false);
		::luabind::object result;
		parse_table(table, "animations", result);
		TransitionData tmp;
		::luabind::object::iterator i = result.begin();
		::luabind::object::iterator e = result.end();
		for (; i != e; ++i)
		{
			::luabind::object string = *i;
			if (string.type() != LUA_TSTRING)
			{
				VERIFY(string.type() != LUA_TNIL);
				continue;
			}

			shared_str animation = ::luabind::object_cast<LPCSTR>(string);
			VERIFY2(
				std::find(
					tmp.begin(),
					tmp.end(),
					animation
				) ==
				tmp.end(),
				make_string(
					"duplicated_animation found: %s",
					animation.c_str()
				)
			);
			tmp.push_back(animation);
		}
		float weight = parse_float(table, "weight");

		if (!m_transitions.vertex(action_from))
			m_transitions.add_vertex(Loki::EmptyType(), action_from);

		if (!m_transitions.vertex(action_to))
			m_transitions.add_vertex(Loki::EmptyType(), action_to);

		m_transitions.add_edge(action_from, action_to, weight);
		TransitionGraph::CEdge* edge = m_transitions.edge(action_from, action_to);
		VERIFY(!tmp.empty());
		edge->data() = tmp;
	}
}

smart_cover::action::Animations const& loophole::action_animations(shared_str const& action_id,
                                                                   shared_str const& animation_id) const
{
	typedef loophole::ActionList::const_iterator const_iterator;
	const_iterator found = m_actions.find(action_id);
	VERIFY2(found != m_actions.end(),
	        make_string("action [%s] not present in loophole [%s]", action_id.c_str(), m_id.c_str()));

	return found->second->animations(m_id, animation_id);
}

loophole::TransitionData const& loophole::transition_animations(shared_str const& action_from,
                                                                shared_str const& action_to) const
{
	TransitionGraph::CEdge const* edge = m_transitions.edge(transform_vertex(action_from, true),
	                                                        transform_vertex(action_to, false));
	VERIFY2(edge, make_string("transition [%s]->[%s] absent", action_from.c_str(), action_to.c_str()));
	return (edge->data());
}

void loophole::exit_position(Fvector& position) const
{
	ActionList::const_iterator found = m_actions.find("exit");

	if (found != m_actions.end())
		position = found->second->target_position();
}

#include "pch_script.h"
#include "xrEngine/FDemoRecord.h"
#include "xrEngine/FDemoPlay.h"
#include "xrEngine/Environment.h"
#include "xrEngine/IGame_Persistent.h"
#include "ParticlesObject.h"
#include "Level.h"
#include "HUDManager.h"
#include "xrServer.h"
#include "NET_Queue.h"
#include "game_cl_base.h"
#include "entity_alive.h"
#include "ai_space.h"
#include "ai_debug.h"
#include "ShootingObject.h"
#include "GameTaskManager.h"
#include "Level_Bullet_Manager.h"
#include "script_process.h"
#include "script_engine.h"
#include "script_engine_space.h"
#include "team_base_zone.h"
#include "infoportion.h"
#include "patrol_path_storage.h"
#include "date_time.h"
#include "space_restriction_manager.h"
#include "seniority_hierarchy_holder.h"
#include "space_restrictor.h"
#include "client_spawn_manager.h"
#include "autosave_manager.h"
#include "ClimableObject.h"
#include "level_graph.h"
#include "mt_config.h"
#include "phcommander.h"
#include "map_manager.h"
#include "xrEngine/CameraManager.h"
#include "level_sounds.h"
#include "car.h"
#include "trade_parameters.h"
#include "game_cl_base_weapon_usage_statistic.h"
#include "MainMenu.h"
#include "xrEngine/XR_IOConsole.h"
#include "actor.h"
#include "player_hud.h"
#include "UI/UIGameTutorial.h"
#include "file_transfer.h"
#include "message_filter.h"
#include "demoplay_control.h"
#include "demoinfo.h"
#include "CustomDetector.h"
#include "xrPhysics/IPHWorld.h"
#include "xrPhysics/console_vars.h"
#include "../xrEngine/device.h"

#include "UIGameCustom.h"
#include "ui/UIPdaWnd.h"
#include "UICursor.h"
#include "debug_renderer.h"
#include "LevelDebugScript.h"

#include "alife_simulator.h"
#include "alife_object_registry.h"

#ifdef DEBUG
#include "level_debug.h"
#include "ai/stalker/ai_stalker.h"
#include "PhysicObject.h"
#include "PHDebug.h"
#include "debug_text_tree.h"
#endif
extern ENGINE_API bool g_dedicated_server;
extern ENGINE_API BOOL	g_bootComplete;
extern CUISequencer* g_tutorial;
extern CUISequencer* g_tutorial2;

float g_cl_lvInterp = 0.1;
u32 lvInterpSteps = 0;

#ifdef SPAWN_ANTIFREEZE
BOOL spawn_antifreeze = TRUE;
BOOL spawn_antifreeze_debug = FALSE;
static xrCriticalSection prefetch_cs;
static HANDLE prefetch_thread_signal;

static void unpausePrefetchThreadSignal()
{
	//if (spawn_antifreeze_debug) Msg("prefetch_thread_signal Set");
	SetEvent(prefetch_thread_signal);
}

static void pausePrefetchThreadSignal()
{
	//if (spawn_antifreeze_debug) Msg("prefetch_thread_signal Reset");
	ResetEvent(prefetch_thread_signal);
}

static void closePrefetchThreadSignal()
{
	if (spawn_antifreeze_debug) Msg("prefetch_thread_signal Close");
	CloseHandle(prefetch_thread_signal);
}

static void createPrefetchThreadSignal()
{
	if (spawn_antifreeze_debug) Msg("prefetch_thread_signal CreateEvent");
	prefetch_thread_signal = CreateEvent(nullptr, TRUE, FALSE, nullptr);
}

struct spawn_and_prefetch_events
{
	NET_Queue_Event* spawn_events = nullptr;
	prefetch_event_queue* prefetch_events = nullptr;
	models_set* prefetched_models = nullptr;
	bool* closeSignal = nullptr;
};

u16	GetSpawnInfo(NET_Packet &P, u16 &parent_id, shared_str& section)
{
    u16 dummy16, id;
    P.r_begin(dummy16);

    shared_str s_name;
    P.r_stringZ(s_name);
	section = s_name;

	string256 temp;
	P.r_stringZ(temp);

	u8 temp_gt, s_RP;
	Fvector o_Position, o_Angle;
	u16 RespawnTime;
	P.r_u8(temp_gt/*s_gameid*/);
	P.r_u8(s_RP);
	P.r_vec3(o_Position);
	P.r_vec3(o_Angle);
	P.r_u16(RespawnTime);
	P.r_u16(id);
	P.r_u16(parent_id);

    P.r_pos = 0;
    return id;
}
#endif
//-AVO

namespace crash_saving {
	extern void(*save_impl)();
	static bool g_isSaving = false;
	int saveCountMax = 10;

	void _save_impl()
	{
		if (g_isSaving) return;
		if (saveCountMax <= 0) return;

		int saveCount = -1;
		g_isSaving = true;
		NET_Packet net_packet;
		net_packet.w_begin(M_SAVE_GAME);

		std::string path = "fatal_ctd_save_";
		std::string path_mask(path);
		std::string path_ext = ".scop";
		path_mask.append("*").append(path_ext);

		FS_FileSet fset_temp;
		FS.file_list(fset_temp, "$game_saves$", FS_ListFiles | FS_RootOnly, path_mask.c_str());

		std::vector<FS_File> fset(fset_temp.begin(), fset_temp.end());
		struct {
			bool operator()(FS_File& a, FS_File& b) {
				return a.time_write > b.time_write;
			}
		} sortFilesDesc;
		std::sort(fset.begin(), fset.end(), sortFilesDesc);

		//Msg("save mask %s", path_mask.c_str());

		for (auto &file : fset)
		{
			string128 name;
			xr_strcpy(name, sizeof(name), file.name.c_str());
			std::string name_string(name);
			name_string.erase(name_string.length() - path_ext.length());

			//Msg("found save file %s, save_name %s", name, name_string.c_str());

			try {
				//Msg("save number %s", name_string.substr(path.length()).c_str());
				int name_count = std::stoi(name_string.substr(path.length()));
				saveCount = name_count;
				break;
			} catch (...) {
				Msg("!error getting save number from %s", name);
			}
		}

		saveCount++;
		if (saveCount >= saveCountMax) {
			saveCount = 0;
		}

		path.append(std::to_string(saveCount));
		net_packet.w_stringZ(path.c_str());
		net_packet.w_u8(1);
		CLevel& level = Level();
		if (&level != nullptr)
		{
			level.Send(net_packet, net_flags(1));
		}

	}
}

CLevel::CLevel() :
	IPureClient(Device.GetTimerGlobal())
#ifdef PROFILE_CRITICAL_SECTIONS
, DemoCS(MUTEX_PROFILE_ID(DemoCS))
#endif
{
	g_bDebugEvents = strstr(Core.Params, "-debug_ge") != nullptr;
	game_events = xr_new<NET_Queue_Event>();

	eChangeRP = Engine.Event.Handler_Attach("LEVEL:ChangeRP", this);
	eDemoPlay = Engine.Event.Handler_Attach("LEVEL:PlayDEMO", this);
	eChangeTrack = Engine.Event.Handler_Attach("LEVEL:PlayMusic", this);
	eEnvironment = Engine.Event.Handler_Attach("LEVEL:Environment", this);
	eEntitySpawn = Engine.Event.Handler_Attach("LEVEL:spawn", this);
	m_pBulletManager = xr_new<CBulletManager>();
	if (!g_dedicated_server)
	{
		m_map_manager = xr_new<CMapManager>();
		m_game_task_manager = xr_new<CGameTaskManager>();
	}
	m_dwDeltaUpdate = u32(fixed_step * 1000);
	m_seniority_hierarchy_holder = xr_new<CSeniorityHierarchyHolder>();
	if (!g_dedicated_server)
	{
		m_level_sound_manager = xr_new<CLevelSoundManager>();
		m_space_restriction_manager = xr_new<CSpaceRestrictionManager>();
		m_client_spawn_manager = xr_new<CClientSpawnManager>();
		m_autosave_manager = xr_new<CAutosaveManager>();
        m_debug_renderer = xr_new<CDebugRenderer>();
#ifdef DEBUG
        m_level_debug = xr_new<CLevelDebug>();
#endif
	}
	m_ph_commander = xr_new<CPHCommander>();
	m_ph_commander_scripts = xr_new<CPHCommander>();
	pObjects4CrPr.clear();
	pActors4CrPr.clear();
	g_player_hud = xr_new<player_hud>();
	g_player_hud->load_default();

#ifdef SPAWN_ANTIFREEZE
	spawn_events = xr_new<NET_Queue_Event>();
	prefetch_events = xr_new<prefetch_event_queue>();
	prefetched_models = xr_new<models_set>();
	auto events = new spawn_and_prefetch_events({ spawn_events, prefetch_events, prefetched_models, &closeSignal });
	createPrefetchThreadSignal();
	thread_spawn(ProcessPrefetchEvents, "Pre-Spawn Prefetcher Thread", 0, events);
	Msg("CLevel::CLevel() Spawn Antifreeze initialized");
#endif

	Msg("%s", Core.Params);
	//crash_saving::save_impl = crash_saving::_save_impl; // CLevel ready, we can save now
}

extern CAI_Space* g_ai_space;

CLevel::~CLevel()
{
	//crash_saving::save_impl = nullptr; // CLevel not available, disable crash save
	xr_delete(g_player_hud);
	delete_data(hud_zones_list);
	hud_zones_list = nullptr;
	Msg("- Destroying level");
	Engine.Event.Handler_Detach(eEntitySpawn, this);
	Engine.Event.Handler_Detach(eEnvironment, this);
	Engine.Event.Handler_Detach(eChangeTrack, this);
	Engine.Event.Handler_Detach(eDemoPlay, this);
	Engine.Event.Handler_Detach(eChangeRP, this);
	if (physics_world())
	{
		destroy_physics_world();
		xr_delete(m_ph_commander_physics_worldstep);
	}
	// destroy PSs
	for (POIt p_it = m_StaticParticles.begin(); m_StaticParticles.end() != p_it; ++p_it)
		CParticlesObject::Destroy(*p_it);
	m_StaticParticles.clear();
	// Unload sounds
	// unload prefetched sounds
	sound_registry.clear();
	// unload static sounds
	for (u32 i = 0; i < static_Sounds.size(); ++i)
	{
		static_Sounds[i]->destroy();
		xr_delete(static_Sounds[i]);
	}
	static_Sounds.clear();
	xr_delete(m_level_sound_manager);
	xr_delete(m_space_restriction_manager);
	xr_delete(m_seniority_hierarchy_holder);
	xr_delete(m_client_spawn_manager);
	xr_delete(m_autosave_manager);
    xr_delete(m_debug_renderer);
	delete_data(m_debug_render_queue);
	if (!g_dedicated_server)
		ai().script_engine().remove_script_process(ScriptEngine::eScriptProcessorLevel);
	xr_delete(game);
	xr_delete(game_events);

#ifdef SPAWN_ANTIFREEZE
	xr_delete(spawn_events);
	xr_delete(prefetch_events);
	xr_delete(prefetched_models);
	closeSignal = true; // signal ProcessPrefetchEvents thread to exit
	unpausePrefetchThreadSignal();
#endif

	xr_delete(m_pBulletManager);
	xr_delete(pStatGraphR);
	xr_delete(pStatGraphS);
	xr_delete(m_ph_commander);
	xr_delete(m_ph_commander_scripts);
	pObjects4CrPr.clear();
	pActors4CrPr.clear();
	ai().unload();
#ifdef DEBUG
    xr_delete(m_level_debug);
#endif
	xr_delete(m_map_manager);
	delete_data(m_game_task_manager);
	// here we clean default trade params
	// because they should be new for each saved/loaded game
	// and I didn't find better place to put this code in
	// XXX nitrocaster: find better place for this clean()
	CTradeParameters::clean();
	if (g_tutorial && g_tutorial->m_pStoredInputReceiver == this)
		g_tutorial->m_pStoredInputReceiver = nullptr;
	if (g_tutorial2 && g_tutorial2->m_pStoredInputReceiver == this)
		g_tutorial2->m_pStoredInputReceiver = nullptr;
	if (IsDemoPlay())
	{
		StopPlayDemo();
		if (m_reader)
		{
			FS.r_close(m_reader);
			m_reader = nullptr;
		}
	}
	xr_delete(m_msg_filter);
	xr_delete(m_demoplay_control);
	xr_delete(m_demo_info);
	if (IsDemoSave())
	{
		StopSaveDemo();
	}
	deinit_compression();
}

shared_str CLevel::name() const
{
	return map_data.m_name;
}

void CLevel::GetLevelInfo(CServerInfo* si)
{
	if (Server && game)
	{
		Server->GetServerInfo(si);
	}
}

void CLevel::PrefetchSound(LPCSTR name)
{
	// preprocess sound name
	string_path tmp;
	xr_strcpy(tmp, name);
	xr_strlwr(tmp);
	if (strext(tmp))
		*strext(tmp) = 0;
	shared_str snd_name = tmp;
	// find in registry
	SoundRegistryMapIt it = sound_registry.find(snd_name);
	// if find failed - preload sound
	if (it == sound_registry.end())
		sound_registry[snd_name].create(snd_name.c_str(), st_Effect, sg_SourceType);
}

// Game interface ////////////////////////////////////////////////////
int CLevel::get_RPID(LPCSTR /**name/**/)
{
	/*
	// Gain access to string
	LPCSTR	params = pLevel->r_string("respawn_point",name);
	if (0==params)	return -1;

	// Read data
	Fvector4	pos;
	int			team;
	sscanf		(params,"%f,%f,%f,%d,%f",&pos.x,&pos.y,&pos.z,&team,&pos.w); pos.y += 0.1f;

	// Search respawn point
	svector<Fvector4,maxRP>	&rp = Level().get_team(team).RespawnPoints;
	for (int i=0; i<(int)(rp.size()); ++i)
	if (pos.similar(rp[i],EPS_L))	return i;
	*/
	return -1;
}

bool g_bDebugEvents = false;

void CLevel::cl_Process_Event(u16 dest, u16 type, NET_Packet& P)
{
	// Msg("--- event[%d] for [%d]",type,dest);
	CObject* O = Objects.net_Find(dest);
	if (0 == O)
	{
#ifdef DEBUG
        Msg("* WARNING: c_EVENT[%d] to [%d]: unknown dest", type, dest);
#endif
		return;
	}
	CGameObject* GO = smart_cast<CGameObject*>(O);
	if (!GO)
	{
#ifndef MASTER_GOLD
        Msg("! ERROR: c_EVENT[%d] : non-game-object", dest);
#endif
		return;
	}
	if (type != GE_DESTROY_REJECT)
	{
		if (type == GE_DESTROY)
		{
			Game().OnDestroy(GO);
		}
		GO->OnEvent(P, type);
	}
	else
	{
		// handle GE_DESTROY_REJECT here
		u32 pos = P.r_tell();
		u16 id = P.r_u16();
		P.r_seek(pos);
		bool ok = true;
		CObject* D = Objects.net_Find(id);
		if (0 == D)
		{
#ifndef MASTER_GOLD
            Msg("! ERROR: c_EVENT[%d] : unknown dest", id);
#endif
			ok = false;
		}
		CGameObject* GD = smart_cast<CGameObject*>(D);
		if (!GD)
		{
#ifndef MASTER_GOLD
            Msg("! ERROR: c_EVENT[%d] : non-game-object", id);
#endif
			ok = false;
		}
		GO->OnEvent(P, GE_OWNERSHIP_REJECT);
		if (ok)
		{
			Game().OnDestroy(GD);
			GD->OnEvent(P, GE_DESTROY);
		}
	}
}

//AVO: used by SPAWN_ANTIFREEZE (by alpet, edited by demonized)
#ifdef SPAWN_ANTIFREEZE
bool CLevel::PostponedSpawnFind(u16 id, const NET_Event& E) const
{
	NET_Packet P;
	E.implication(P);
	return PostponedSpawnFind(id, P);
}

bool CLevel::PostponedSpawnFind(u16 id, NET_Packet& P) const
{
	u16 parent_id;
	shared_str section;
	return id == GetSpawnInfo(P, parent_id, section);
}

bool CLevel::PostponedSpawn(u16 id)
{
	PROF_EVENT("ProcessGameEvents PostponedSpawn");

	xrCriticalSectionGuard g(prefetch_cs);
	auto queue = prefetch_events;
	auto it = std::find_if(queue->begin(), queue->end(), [id, this](prefetch_event& E) { return PostponedSpawnFind(id, E.p); });

	auto& queue2 = spawn_events->queue;
	auto it2 = std::find_if(queue2.begin(), queue2.end(), [id, this](const NET_Event& E) { return PostponedSpawnFind(id, E); });
	return it != queue->end() || it2 != queue2.end();
}

int CLevel::GetSpawnEventPriority(const NET_Event& e) const
{
	if (e.ID == M_EVENT)
		return 0;

	if (e.ID == M_SPAWN) {
		NET_Packet P;
		e.implication(P);

		u16 parent_id = 0;
		shared_str section;
		GetSpawnInfo(P, parent_id, section);
		if (parent_id < 0xFFFF)
			return 1;

		return 2;
	}

	return 0;
}

bool CLevel::SpawnEventCompare(const NET_Event& a, const NET_Event& b) const
{
	return GetSpawnEventPriority(a) > GetSpawnEventPriority(b);
}

// demonized: If called manually, be aware of ProcessPrefetchEvents thread, which may modify spawn_events queue at the same time, maybe fix later
void CLevel::SortSpawnEventsQueue()
{
	auto& queue = spawn_events->queue;
	std::stable_sort(queue.begin(), queue.end(), [this](const NET_Event& a, const NET_Event& b) { return SpawnEventCompare(a, b); });
}

void CLevel::ProcessPrefetchEvents(void* args)
{
	auto events = reinterpret_cast<spawn_and_prefetch_events*>(args);
	auto spawn_events = events->spawn_events;
	auto prefetch_events = events->prefetch_events;
	auto prefetched_models = events->prefetched_models;
	auto closeSignal = events->closeSignal;	

	while (true)
	{
		WaitForSingleObject(prefetch_thread_signal, INFINITE); // wait for prefetch queue event to be signaled

		if (*closeSignal == true)
		{
			if (spawn_antifreeze_debug) Msg("[ProcessPrefetchEvents] closeSignal received, destroying thread");
			closePrefetchThreadSignal();
			delete events;
			return;
		}

		if (prefetch_events->empty())
		{
			if (spawn_antifreeze_debug) Msg("[ProcessPrefetchEvents] called, but prefetch_events queue is empty");
			pausePrefetchThreadSignal();
			continue;
		}

		PROF_EVENT("ProcessPrefetchEvents");

		if (spawn_antifreeze_debug) Msg("[ProcessPrefetchEvents] started, queue size %d", prefetch_events->size());

		prefetch_event_queue saved_prefetch_events;

		xrCriticalSectionGuard g(prefetch_cs);
		saved_prefetch_events = std::move(*prefetch_events); // move the events to temp queue, so we can continue processing prefetch_events in the main thread
		pausePrefetchThreadSignal();
		g.Leave();

		for (const auto& E : saved_prefetch_events)
		{
			for (const auto& model : E.models)
			{
				if (prefetched_models->find(model) == prefetched_models->end())
				{
					if (spawn_antifreeze_debug) Msg("[ProcessPrefetchEvents] Prefetching model '%s' for spawn event", model.c_str());
					::Render->models_PrefetchOne(model.c_str(), false);
					prefetched_models->insert(model); // add model to prefetched models set to avoid double prefetching
				}
			}
		}

		g.Enter();
		for (auto& E : saved_prefetch_events)
		{
			spawn_events->insert(E.p); // reinsert the event to spawn_events queue for further processing
		}

		if (spawn_antifreeze_debug) Msg("[ProcessPrefetchEvents] finished, spawn_events queue size %d", spawn_events->queue.size());
	}
}

// demonized: If called manually, be aware of ProcessPrefetchEvents thread, which may modify spawn_events queue at the same time, maybe fix later
void CLevel::ProcessSpawnEvents()
{
	PROF_EVENT("ProcessSpawnEvents");
	for (auto it = spawn_events->queue.begin(); it != spawn_events->queue.end();)
	{
		const NET_Event& E = *it;
		u16 ID, dest, type;
		NET_Packet P;
		ID = E.ID;
		dest = E.destination;
		type = E.type;
		E.implication(P);

		u16 parent_id;
		shared_str section;
		u16 obj_id = GetSpawnInfo(P, parent_id, section);

		if (spawn_antifreeze_debug) Msg("[ProcessSpawnEvents] spawning section %s, obj_id %d, parent_id %d, event_id %d", section.c_str(), obj_id, parent_id, dest);

		// demonized: If item is II_BOLT class - go through anyway
		if (pSettings->line_exist(section.c_str(), "class") && strstr(pSettings->r_string(section.c_str(), "class"), "II_BOLT") != nullptr)
		{
			// demonized: this is a sin, but its an easy way
			goto spawn;
		}

		// demonized: If there is a parent of this object, check if its still in alife
		if (parent_id != 0xffff)
		{
			auto parent_obj = ai().alife().objects().object(parent_id);
			if (!parent_obj || !parent_obj->m_bOnline)
			{
				if (spawn_antifreeze_debug) Msg("![ProcessSpawnEvents] parent object is not in alife, do not spawn, section %s, obj_id %d, parent_id %d, event_id %d", section.c_str(), obj_id, parent_id, dest);
				it = spawn_events->queue.erase(it); // remove current event
				continue;
			}
		}

	spawn:
		u16 dummy16;
		P.r_begin(dummy16);
		cl_Process_Spawn(P);
		it = spawn_events->queue.erase(it);
	}
}
#endif

void CLevel::ProcessGameEvents()
{
	PROF_EVENT("ProcessGameEvents");

	// Game events
	{
		for (auto it = game_events->queue.begin(); it != game_events->queue.end(); )
		{
			PROF_EVENT("ProcessGameEvents game_events queue");
			u16 ID = it->ID;
			u16 dest = it->destination;
			u16 type = it->type;
			NET_Packet P;
			it->implication(P);

//AVO: spawn antifreeze implementation, originally by alpet, reritten by demonized
#ifdef SPAWN_ANTIFREEZE
			if (spawn_antifreeze && g_bootComplete)
			{
				// Postpone M_EVENT for postponed spawns
				//if (M_EVENT == ID && PostponedSpawn(dest))
				//{
				//	game_events->insert(P);
				//	Msg("[ProcessGameEvents] postponed M_EVENT, object in prefetch queue: obj_id %d", dest);
				//	it = game_events->queue.erase(it); // remove current event
				//	continue;
				//}

				// add to prefetch_events queue for postponed spawn
				if (M_SPAWN == ID)
				{
					PROF_EVENT("ProcessGameEvents M_SPAWN");

					u16 parent_id;
					shared_str section;
					u16 obj_id = GetSpawnInfo(P, parent_id, section);

					static auto isValidToPrefetch = [](u16 parent_id, shared_str& section, u16 obj_id, NET_Packet& P) {
						if (pSettings->line_exist("spawn_antifreeze_ignore", section))
						{
							return false;
						}

						bool valid = true;

						if (pSettings->line_exist(section.c_str(), "class"))
						{
							auto c = pSettings->r_string(section.c_str(), "class");

							// Do not prefetch fake missiles of a weapon
							valid &= strstr(c, "G_RPG7") == nullptr;
							valid &= strstr(c, "G_FAKE") == nullptr;

							// Do not prefetch helicopters
							valid &= strstr(c, "C_HLCP") == nullptr;
						}

						return valid;
					};

					if (isValidToPrefetch(parent_id, section, obj_id, P))
					{
						models_set models;

						static auto safe_insert = [](models_set& models, LPCSTR model) {
							if (model)
							{
								string_path modelWithoutExtension;
								xr_strcpy(modelWithoutExtension, model);
								xr_strlwr(modelWithoutExtension);
								if (strext(modelWithoutExtension)) *strext(modelWithoutExtension) = 0;

								if (xr_strcmp(modelWithoutExtension, "") != 0 && xr_strcmp(modelWithoutExtension, ".ogf") != 0)
									models.insert(modelWithoutExtension);
							}
						};

						// Insert visual from ltx
						if (pSettings->line_exist(section, "visual"))
						{
							safe_insert(models, pSettings->r_string(section, "visual"));
						}

						// Corpse visual
						/*if (pSettings->line_exist(section, "corpse_visual"))
						{
							safe_insert(models, pSettings->r_string(section, "corpse_visual"));
						}*/

						// Bloodsucker visual
						/*if (pSettings->line_exist(section, "Predator_Visual"))
						{
							safe_insert(models, pSettings->r_string(section, "Predator_Visual"));
						}*/

						auto obj = ai().alife().objects().object(obj_id);

						// Actual visual from alife object
						if (obj && obj->visual())
						{
							safe_insert(models, obj->visual()->get_visual());
						}

						if (!models.empty())
						{
							prefetch_event E;
							E.p = std::move(P);
							E.models = std::move(models);

							xrCriticalSectionGuard g(prefetch_cs);
							prefetch_events->push_back(E);

							if (spawn_antifreeze_debug) Msg("[ProcessGameEvents] added M_SPAWN to prefetch_events: section %s, obj_id %d, parent_id %d, event_id %d", section.c_str(), obj_id, parent_id, dest);
							it = game_events->queue.erase(it); // remove current event
							continue;
						}
					}					
				}
			}
#endif
//-AVO
			it = game_events->queue.erase(it); // remove current event
			switch (ID)
			{
			case M_SPAWN:
				{
					PROF_EVENT("ProcessGameEvents M_SPAWN");

#ifdef SPAWN_ANTIFREEZE
					if (spawn_antifreeze_debug)
					{
						u16 parent_id;
						shared_str section;
						u16 obj_id = GetSpawnInfo(P, parent_id, section);
						Msg("[ProcessGameEvents] M_SPAWN: section %s, obj_id %d, parent_id %d, event_id %d", section.c_str(), obj_id, parent_id, dest);
					}
#endif

					u16 dummy16;
					P.r_begin(dummy16);
					cl_Process_Spawn(P);
					break;
				}
			case M_EVENT:
				{
					PROF_EVENT("ProcessGameEvents M_EVENT");
					cl_Process_Event(dest, type, P);
					break;
				}
			case M_MOVE_PLAYERS:
				{
					PROF_EVENT("ProcessGameEvents M_MOVE_PLAYERS");
					u8 Count = P.r_u8();
					for (u8 i = 0; i < Count; i++)
					{
						u16 ID = P.r_u16();
						Fvector NewPos, NewDir;
						P.r_vec3(NewPos);
						P.r_vec3(NewDir);
						CActor* OActor = smart_cast<CActor*>(Objects.net_Find(ID));
						if (0 == OActor)
							break;
						OActor->MoveActor(NewPos, NewDir);
					}
					NET_Packet PRespond;
					PRespond.w_begin(M_MOVE_PLAYERS_RESPOND);
					Send(PRespond, net_flags(TRUE, TRUE));
					break;
				}
			case M_STATISTIC_UPDATE:
				{
					PROF_EVENT("ProcessGameEvents M_STATISTIC_UPDATE");
					if (GameID() != eGameIDSingle)
						Game().m_WeaponUsageStatistic->OnUpdateRequest(&P);
					break;
				}
			case M_FILE_TRANSFER:
				{
					PROF_EVENT("ProcessGameEvents M_FILE_TRANSFER");
					if (m_file_transfer) // in case of net_Stop
						m_file_transfer->on_message(&P);
					break;
				}
			case M_GAMEMESSAGE:
				{
					PROF_EVENT("ProcessGameEvents M_GAMEMESSAGE");
					Game().OnGameMessage(P);
					break;
				}
			default:
				{
					VERIFY(0);
					break;
				}
			}
		}
	}

#ifdef SPAWN_ANTIFREEZE
	if (!prefetch_events->empty())
	{
		unpausePrefetchThreadSignal(); // signal ProcessPrefetchEvents thread to process prefetch_events queue
	}
#endif

	if (OnServer() && GameID() != eGameIDSingle)
		Game().m_WeaponUsageStatistic->Send_Check_Respond();
}

#ifdef DEBUG_MEMORY_MANAGER
extern Flags32 psAI_Flags;
extern float debug_on_frame_gather_stats_frequency;

struct debug_memory_guard
{
    inline debug_memory_guard()
    {
        mem_alloc_gather_stats(!!psAI_Flags.test(aiDebugOnFrameAllocs));
        mem_alloc_gather_stats_frequency(debug_on_frame_gather_stats_frequency);
    }
};
#endif

void CLevel::MakeReconnect()
{
	if (!Engine.Event.Peek("KERNEL:disconnect"))
	{
		Engine.Event.Defer("KERNEL:disconnect");
		char const* server_options = nullptr;
		char const* client_options = nullptr;
		if (m_caServerOptions.c_str())
		{
			server_options = xr_strdup(*m_caServerOptions);
		}
		else
		{
			server_options = xr_strdup("");
		}
		if (m_caClientOptions.c_str())
		{
			client_options = xr_strdup(*m_caClientOptions);
		}
		else
		{
			client_options = xr_strdup("");
		}
		Engine.Event.Defer("KERNEL:start", size_t(server_options), size_t(client_options));
	}
}

void CLevel::OnFrame()
{
	PROF_EVENT("CLevel::OnFrame()");

#ifdef DEBUG_MEMORY_MANAGER
    debug_memory_guard __guard__;
#endif
#ifdef DEBUG
    DBG_RenderUpdate();
#endif
	Fvector temp_vector;
	m_feel_deny.feel_touch_update(temp_vector, 0.f);
	if (GameID() != eGameIDSingle)
		psDeviceFlags.set(rsDisableObjectsAsCrows, true);
	else
		psDeviceFlags.set(rsDisableObjectsAsCrows, false);
	// commit events from bullet manager from prev-frame
	Device.Statistic->TEST0.Begin();
	BulletManager().CommitEvents();
	Device.Statistic->TEST0.End();
	// Client receive
	if (net_isDisconnected())
	{
		if (OnClient() && GameID() != eGameIDSingle)
		{
#ifdef DEBUG
            Msg("--- I'm disconnected, so clear all objects...");
#endif
			ClearAllObjects();
		}
		Engine.Event.Defer("kernel:disconnect");
		return;
	}
	else
	{
		Device.Statistic->netClient1.Begin();
		ClientReceive();
		Device.Statistic->netClient1.End();
	}
	
	ProcessGameEvents();
#ifdef SPAWN_ANTIFREEZE
	{
		xrCriticalSectionGuard g(prefetch_cs);
		if (!spawn_events->queue.empty())
		{
			SortSpawnEventsQueue();
			ProcessSpawnEvents();
		}
	}
#endif

	if (m_bNeed_CrPr)
		make_NetCorrectionPrediction();
	if (!g_dedicated_server)
	{
		if (g_mt_config.test(mtMap))
			Device.seqParallel.push_back(fastdelegate::FastDelegate0<>(m_map_manager, &CMapManager::Update));
		else
			MapManager().Update();
		if (IsGameTypeSingle() && Device.dwPrecacheFrame == 0)
		{
			// XXX nitrocaster: was enabled in x-ray 1.5; to be restored or removed
			//if (g_mt_config.test(mtMap))
			//{
			//    Device.seqParallel.push_back(fastdelegate::FastDelegate0<>(
			//    m_game_task_manager,&CGameTaskManager::UpdateTasks));
			//}
			//else
			GameTaskManager().UpdateTasks();
		}
	}
	// Inherited update
	inherited::OnFrame();
	// Draw client/server stats
	if (!g_dedicated_server && psDeviceFlags.test(rsStatistic))
	{
		CGameFont* F = UI().Font().pFontDI;
		if (!psNET_direct_connect)
		{
			if (IsServer())
			{
				const IServerStatistic* S = Server->GetStatistic();
				F->SetHeightI(0.015f);
				F->OutSetI(0.0f, 0.5f);
				F->SetColor(D3DCOLOR_XRGB(0, 255, 0));
				F->OutNext("IN:  %4d/%4d (%2.1f%%)", S->bytes_in_real, S->bytes_in,
				           100.f * float(S->bytes_in_real) / float(S->bytes_in));
				F->OutNext("OUT: %4d/%4d (%2.1f%%)", S->bytes_out_real, S->bytes_out,
				           100.f * float(S->bytes_out_real) / float(S->bytes_out));
				F->OutNext("client_2_sever ping: %d", net_Statistic.getPing());
				F->OutNext("SPS/Sended : %4d/%4d", S->dwBytesPerSec, S->dwBytesSended);
				F->OutNext("sv_urate/cl_urate : %4d/%4d", psNET_ServerUpdate, psNET_ClientUpdate);
				F->SetColor(D3DCOLOR_XRGB(255, 255, 255));
				struct net_stats_functor
				{
					xrServer* m_server;
					CGameFont* F;

					void operator()(IClient* C)
					{
						m_server->UpdateClientStatistic(C);
						F->OutNext("0x%08x: P(%d), BPS(%2.1fK), MRR(%2d), MSR(%2d), Retried(%2d), Blocked(%2d)",
						           //Server->game->get_option_s(*C->Name,"name",*C->Name),
						           C->ID.value(),
						           C->stats.getPing(),
						           float(C->stats.getBPS()), // /1024,
						           C->stats.getMPS_Receive(),
						           C->stats.getMPS_Send(),
						           C->stats.getRetriedCount(),
						           C->stats.dwTimesBlocked);
					}
				};
				net_stats_functor tmp_functor;
				tmp_functor.m_server = Server;
				tmp_functor.F = F;
				Server->ForEachClientDo(tmp_functor);
			}
			if (IsClient())
			{
				IPureClient::UpdateStatistic();
				F->SetHeightI(0.015f);
				F->OutSetI(0.0f, 0.5f);
				F->SetColor(D3DCOLOR_XRGB(0, 255, 0));
				F->OutNext("client_2_sever ping: %d", net_Statistic.getPing());
				F->OutNext("sv_urate/cl_urate : %4d/%4d", psNET_ServerUpdate, psNET_ClientUpdate);
				F->SetColor(D3DCOLOR_XRGB(255, 255, 255));
				F->OutNext("BReceivedPs(%2d), BSendedPs(%2d), Retried(%2d), Blocked(%2d)",
				           net_Statistic.getReceivedPerSec(),
				           net_Statistic.getSendedPerSec(),
				           net_Statistic.getRetriedCount(),
				           net_Statistic.dwTimesBlocked);
#ifdef DEBUG
                if (!pStatGraphR)
                {
                    pStatGraphR = xr_new<CStatGraph>();
                    pStatGraphR->SetRect(50, 700, 300, 68, 0xff000000, 0xff000000);
                    //m_stat_graph->SetGrid(0, 0.0f, 10, 1.0f, 0xff808080, 0xffffffff);
                    pStatGraphR->SetMinMax(0.0f, 65536.0f, 1000);
                    pStatGraphR->SetStyle(CStatGraph::stBarLine);
                    pStatGraphR->AppendSubGraph(CStatGraph::stBarLine);
                }
                pStatGraphR->AppendItem(float(net_Statistic.getBPS()), 0xff00ff00, 0);
                F->OutSet(20.f, 700.f);
                F->OutNext("64 KBS");
#endif
			}
		}
	}
	else
	{
#ifdef DEBUG
        if (pStatGraphR)
            xr_delete(pStatGraphR);
#endif
	}
#ifdef DEBUG
    g_pGamePersistent->Environment().m_paused = m_bEnvPaused;
#endif
	g_pGamePersistent->Environment().SetGameTime(GetEnvironmentGameDayTimeSec(),
	                                             game->GetEnvironmentGameTimeFactor());
	if (!g_dedicated_server)
		ai().script_engine().script_process(ScriptEngine::eScriptProcessorLevel)->update();
	m_ph_commander->update();
	m_ph_commander_scripts->update();
	Device.Statistic->TEST0.Begin();
	BulletManager().CommitRenderSet();
	Device.Statistic->TEST0.End();
	// update static sounds
	if (!g_dedicated_server)
	{
		if (g_mt_config.test(mtLevelSounds))
		{
			Device.seqParallel.push_back(fastdelegate::FastDelegate0<>(
				m_level_sound_manager, &CLevelSoundManager::Update));
		}
		else
			m_level_sound_manager->Update();
	}
	// defer LUA-GC-STEP
	if (!g_dedicated_server)
	{
		if (g_mt_config.test(mtLUA_GC))
			Device.seqParallel.push_back(fastdelegate::FastDelegate0<>(this, &CLevel::script_gc));
		else
			script_gc();
	}
	if (pStatGraphR)
	{
		static float fRPC_Mult = 10.0f;
		static float fRPS_Mult = 1.0f;
		pStatGraphR->AppendItem(float(m_dwRPC) * fRPC_Mult, 0xffff0000, 1);
		pStatGraphR->AppendItem(float(m_dwRPS) * fRPS_Mult, 0xff00ff00, 0);
	}
}

int psLUA_GCSTEP = 300;

void CLevel::script_gc()
{
	PROF_EVENT();
	lua_gc(ai().script_engine().lua(), LUA_GCSTEP, psLUA_GCSTEP);
}

#ifdef DEBUG_PRECISE_PATH
void test_precise_path();
#endif

#ifdef DEBUG
extern Flags32 dbg_net_Draw_Flags;
#endif

extern void draw_wnds_rects();
extern bool use_reshade;
extern void render_reshade_effects();

extern int ps_r4_hdr10_pda; // NOTE: this is a hack to avoid double HDR tonemapping the PDA

void CLevel::OnRender()
{
	// PDA
	if (game && CurrentGameUI() && &CurrentGameUI()->GetPdaMenu() != nullptr)
	{
		CUIPdaWnd* pda = &CurrentGameUI()->GetPdaMenu();
		if (psActorFlags.test(AF_3D_PDA) && pda->IsShown())
		{
			ps_r4_hdr10_pda = 1; // !!! HACK !!!

			pda->Draw();
			CUICursor* cursor = &UI().GetUICursor();

			if (cursor)
			{
				static bool need_reset;
				bool is_top = CurrentGameUI()->TopInputReceiver() == pda;

				if (pda->IsEnabled() && is_top && !Console->bVisible)
				{
					if (need_reset)
					{
						need_reset = false;
						pda->ResetCursor();
					}

					Frect &pda_border = pda->m_cursor_box;
					Fvector2 cursor_pos = cursor->GetCursorPosition();

					if (!pda_border.in(cursor_pos))
					{
						clamp(cursor_pos.x, pda_border.left, pda_border.right);
						clamp(cursor_pos.y, pda_border.top, pda_border.bottom);
						cursor->SetUICursorPosition(cursor_pos);
					}

					Fvector2 cursor_pos_dif;
					cursor_pos_dif.set(cursor_pos);
					cursor_pos_dif.sub(pda->last_cursor_pos);
					pda->last_cursor_pos.set(cursor_pos);
					pda->MouseMovement(cursor_pos_dif.x, cursor_pos_dif.y);
				}
				else
					need_reset = true;

				if (is_top)
					cursor->OnRender();
			}
			Render->RenderToTarget(Render->rtPDA);

			ps_r4_hdr10_pda = 0;
		}

		if (Actor() && Actor()->m_bDelayDrawPickupItems)
		{
			Actor()->m_bDelayDrawPickupItems = false;
			Actor()->DrawPickupItems();
		}
	}

	inherited::OnRender();
	if (!game)
		return;
	Game().OnRender();
	BulletManager().Render();

	if (Device.m_SecondViewport.IsSVPFrame())
		Render->RenderToTarget(Render->rtSVP);

	if (use_reshade)
		render_reshade_effects();

	HUD().RenderUI();

	ScriptDebugRender();

#ifdef DEBUG
    draw_wnds_rects();
    physics_world()->OnRender();
#endif
#ifdef DEBUG
    if (ai().get_level_graph())
        ai().level_graph().render();
#ifdef DEBUG_PRECISE_PATH
    test_precise_path();
#endif
    CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(Level().CurrentEntity());
    if (stalker)
        stalker->OnRender();
    if (bDebug)
    {
        for (u32 I = 0; I < Level().Objects.o_count(); I++)
        {
            CObject* _O = Level().Objects.o_get_by_iterator(I);
            CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(_O);
            if (stalker)
                stalker->OnRender();
            CCustomMonster* monster = smart_cast<CCustomMonster*>(_O);
            if (monster)
                monster->OnRender();
            CPhysicObject* physic_object = smart_cast<CPhysicObject*>(_O);
            if (physic_object)
                physic_object->OnRender();
            CSpaceRestrictor* space_restrictor = smart_cast<CSpaceRestrictor*>(_O);
            if (space_restrictor)
                space_restrictor->OnRender();
            CClimableObject* climable = smart_cast<CClimableObject*>(_O);
            if (climable)
                climable->OnRender();
            CTeamBaseZone* team_base_zone = smart_cast<CTeamBaseZone*>(_O);
            if (team_base_zone)
                team_base_zone->OnRender();
            if (GameID() != eGameIDSingle)
            {
                CInventoryItem* pIItem = smart_cast<CInventoryItem*>(_O);
                if (pIItem)
                    pIItem->OnRender();
            }
            if (dbg_net_Draw_Flags.test(dbg_draw_skeleton)) //draw skeleton
            {
                CGameObject* pGO = smart_cast<CGameObject*>	(_O);
                if (pGO && pGO != Level().CurrentViewEntity() && !pGO->H_Parent())
                {
                    if (pGO->Position().distance_to_sqr(Device.vCameraPosition) < 400.0f)
                    {
                        pGO->dbg_DrawSkeleton();
                    }
                }
            }
        }
        //  [7/5/2005]
        if (Server && Server->game) Server->game->OnRender();
        //  [7/5/2005]
        ObjectSpace.dbgRender();
        UI().Font().pFontStat->OutSet(170, 630);
        UI().Font().pFontStat->SetHeight(16.0f);
        UI().Font().pFontStat->SetColor(0xffff0000);
        if (Server)
            UI().Font().pFontStat->OutNext("Client Objects:      [%d]", Server->GetEntitiesNum());
        UI().Font().pFontStat->OutNext("Server Objects:      [%d]", Objects.o_count());
        UI().Font().pFontStat->OutNext("Interpolation Steps: [%d]", Level().GetInterpolationSteps());
        if (Server)
        {
            UI().Font().pFontStat->OutNext("Server updates size: [%d]", Server->GetLastUpdatesSize());
        }
        UI().Font().pFontStat->SetHeight(8.0f);
    }
#endif
	debug_renderer().render();
#ifdef DEBUG
    if (bDebug)
    {
        DBG().draw_object_info();
        DBG().draw_text();
        DBG().draw_level_info();
    }
    DBG().draw_debug_text();
    if (psAI_Flags.is(aiVision))
    {
        for (u32 I = 0; I < Level().Objects.o_count(); I++)
        {
            CObject* object = Objects.o_get_by_iterator(I);
            CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(object);
            if (!stalker)
                continue;
            stalker->dbg_draw_vision();
        }
    }

    if (psAI_Flags.test(aiDrawVisibilityRays))
    {
        for (u32 I = 0; I < Level().Objects.o_count(); I++)
        {
            CObject* object = Objects.o_get_by_iterator(I);
            CAI_Stalker* stalker = smart_cast<CAI_Stalker*>(object);
            if (!stalker)
                continue;
            stalker->dbg_draw_visibility_rays();
        }
    }
#endif
}

void CLevel::ScriptDebugRender()
{
	if (!m_debug_render_queue.size())
		return;

	bool hasVisibleObj = false;
	xr_map<u16, DBG_ScriptObject*>::iterator it = m_debug_render_queue.begin();
	xr_map<u16, DBG_ScriptObject*>::iterator it_e = m_debug_render_queue.end();
	for (; it != it_e; ++it)
	{
		DBG_ScriptObject* obj = (*it).second;
		if (obj->m_visible) {
			hasVisibleObj = true;
			obj->Render();
		}
	}

	// demonized: fix of showing console window when there are no visible gizmos 
	if (hasVisibleObj)
		DRender->OnFrameEnd();
}

void CLevel::OnEvent(EVENT E, u64 P1, u64 /**P2/**/)
{
	if (E == eEntitySpawn)
	{
		char Name[128];
		Name[0] = 0;
		sscanf(LPCSTR(P1), "%s", Name);
		Level().g_cl_Spawn(Name, 0xff, M_SPAWN_OBJECT_LOCAL, Fvector().set(0, 0, 0));
	}
	else if (E == eChangeRP && P1)
	{
	}
	else if (E == eDemoPlay && P1)
	{
		char* name = (char*)P1;
		string_path RealName;
		xr_strcpy(RealName, name);
		xr_strcat(RealName, ".xrdemo");
		Cameras().AddCamEffector(xr_new<CDemoPlay>(RealName, 1.3f, 0));
	}
	else if (E == eChangeTrack && P1)
	{
		// int id = atoi((char*)P1);
		// Environment->Music_Play(id);
	}
	else if (E == eEnvironment)
	{
		// int id=0; float s=1;
		// sscanf((char*)P1,"%d,%f",&id,&s);
		// Environment->set_EnvMode(id,s);
	}
}

void CLevel::AddObject_To_Objects4CrPr(CGameObject* pObj)
{
	if (!pObj)
		return;
	for (CGameObject* obj : pObjects4CrPr)
	{
		if (obj == pObj)
			return;
	}
	pObjects4CrPr.push_back(pObj);
}

void CLevel::AddActor_To_Actors4CrPr(CGameObject* pActor)
{
	if (!pActor)
		return;
	if (!smart_cast<CActor*>(pActor)) return;
	for (CGameObject* act : pActors4CrPr)
	{
		if (act == pActor)
			return;
	}
	pActors4CrPr.push_back(pActor);
}

void CLevel::RemoveObject_From_4CrPr(CGameObject* pObj)
{
	if (!pObj)
		return;
	auto objIt = std::find(pObjects4CrPr.begin(), pObjects4CrPr.end(), pObj);
	if (objIt != pObjects4CrPr.end())
	{
		pObjects4CrPr.erase(objIt);
	}
	auto aIt = std::find(pActors4CrPr.begin(), pActors4CrPr.end(), pObj);
	if (aIt != pActors4CrPr.end())
	{
		pActors4CrPr.erase(aIt);
	}
}

void CLevel::make_NetCorrectionPrediction()
{
	m_bNeed_CrPr = false;
	m_bIn_CrPr = true;
	u64 NumPhSteps = physics_world()->StepsNum();
	physics_world()->StepsNum() -= m_dwNumSteps;
	if (ph_console::g_bDebugDumpPhysicsStep && m_dwNumSteps > 10)
	{
		Msg("!!!TOO MANY PHYSICS STEPS FOR CORRECTION PREDICTION = %d !!!", m_dwNumSteps);
		m_dwNumSteps = 10;
	}
	physics_world()->Freeze();
	//setting UpdateData and determining number of PH steps from last received update
	for (CGameObject* obj : pObjects4CrPr)
	{
		if (!obj)
			continue;
		obj->PH_B_CrPr();
	}
	//first prediction from "delivered" to "real current" position
	//making enought PH steps to calculate current objects position based on their updated state
	for (u32 i = 0; i < m_dwNumSteps; i++)
	{
		physics_world()->Step();

		for (CGameObject* act : pActors4CrPr)
		{
			if (!act || act->CrPr_IsActivated())
				continue;
			act->PH_B_CrPr();
		}
	}
	for (CGameObject* obj : pObjects4CrPr)
	{
		if (!obj)
			continue;
		obj->PH_I_CrPr();
	}
	if (!InterpolationDisabled())
	{
		for (u32 i = 0; i < lvInterpSteps; i++) //second prediction "real current" to "future" position
		{
			physics_world()->Step();
		}
		for (CGameObject* obj : pObjects4CrPr)
		{
			if (!obj)
				continue;
			obj->PH_A_CrPr();
		}
	}
	physics_world()->UnFreeze();
	physics_world()->StepsNum() = NumPhSteps;
	m_dwNumSteps = 0;
	m_bIn_CrPr = false;
	pObjects4CrPr.clear();
	pActors4CrPr.clear();
}

u32 CLevel::GetInterpolationSteps()
{
	return lvInterpSteps;
}

void CLevel::UpdateDeltaUpd(u32 LastTime)
{
	u32 CurrentDelta = LastTime - m_dwLastNetUpdateTime;
	if (CurrentDelta < m_dwDeltaUpdate)
		CurrentDelta = iFloor(float(m_dwDeltaUpdate * 10 + CurrentDelta) / 11);
	m_dwLastNetUpdateTime = LastTime;
	m_dwDeltaUpdate = CurrentDelta;
	if (0 == g_cl_lvInterp)
		ReculcInterpolationSteps();
	else if (g_cl_lvInterp > 0)
	{
		lvInterpSteps = iCeil(g_cl_lvInterp / fixed_step);
	}
}

void CLevel::ReculcInterpolationSteps()
{
	lvInterpSteps = iFloor(float(m_dwDeltaUpdate) / (fixed_step * 1000));
	if (lvInterpSteps > 60)
		lvInterpSteps = 60;
	if (lvInterpSteps < 3)
		lvInterpSteps = 3;
}

bool CLevel::InterpolationDisabled()
{
	return g_cl_lvInterp < 0;
}

void CLevel::PhisStepsCallback(u32 Time0, u32 Time1)
{
	if (!Level().game)
		return;
	if (GameID() == eGameIDSingle)
		return;
	//#pragma todo("Oles to all: highly inefficient and slow!!!")
	//fixed (Andy)
	/*
	for (xr_vector<CObject*>::iterator O=Level().Objects.objects.begin(); O!=Level().Objects.objects.end(); ++O)
	{
	if( smart_cast<CActor*>((*O)){
	CActor* pActor = smart_cast<CActor*>(*O);
	if (!pActor || pActor->Remote()) continue;
	pActor->UpdatePosStack(Time0, Time1);
	}
	};
	*/
}

void CLevel::SetNumCrSteps(u32 NumSteps)
{
	m_bNeed_CrPr = true;
	if (m_dwNumSteps > NumSteps)
		return;
	m_dwNumSteps = NumSteps;
	if (m_dwNumSteps > 1000000)
	{
		VERIFY(0);
	}
}

ALife::_TIME_ID CLevel::GetStartGameTime()
{
	return (game->GetStartGameTime());
}

ALife::_TIME_ID CLevel::GetGameTime()
{
	return (game->GetGameTime());
}

ALife::_TIME_ID CLevel::GetEnvironmentGameTime()
{
	return (game->GetEnvironmentGameTime());
}

u8 CLevel::GetDayTime()
{
	u32 dummy32, hours;
	GetGameDateTime(dummy32, dummy32, dummy32, hours, dummy32, dummy32, dummy32);
	VERIFY(hours < 256);
	return u8(hours);
}

float CLevel::GetGameDayTimeSec()
{
	return (float(s64(GetGameTime() % (24 * 60 * 60 * 1000))) / 1000.f);
}

u32 CLevel::GetGameDayTimeMS()
{
	return (u32(s64(GetGameTime() % (24 * 60 * 60 * 1000))));
}

float CLevel::GetEnvironmentGameDayTimeSec()
{
	return (float(s64(GetEnvironmentGameTime() % (24 * 60 * 60 * 1000))) / 1000.f);
}

void CLevel::GetGameDateTime(u32& year, u32& month, u32& day, u32& hours, u32& mins, u32& secs, u32& milisecs)
{
	split_time(GetGameTime(), year, month, day, hours, mins, secs, milisecs);
}

float CLevel::GetGameTimeFactor()
{
	return (game ? game->GetGameTimeFactor() : 1.0f);
}

void CLevel::SetGameTimeFactor(const float fTimeFactor)
{
	game->SetGameTimeFactor(fTimeFactor);
}

void CLevel::SetGameTimeFactor(ALife::_TIME_ID GameTime, const float fTimeFactor)
{
	game->SetGameTimeFactor(GameTime, fTimeFactor);
}

void CLevel::SetEnvironmentGameTimeFactor(u64 const& GameTime, float const& fTimeFactor)
{
	if (!game)
		return;
	game->SetEnvironmentGameTimeFactor(GameTime, fTimeFactor);
}

bool CLevel::IsServer()
{
	if (!Server || IsDemoPlayStarted())
		return false;
	return true;
}

bool CLevel::IsClient()
{
	if (IsDemoPlayStarted())
		return true;
	if (Server)
		return false;
	return true;
}

void CLevel::OnAlifeSimulatorUnLoaded()
{
	MapManager().ResetStorage();
	GameTaskManager().ResetStorage();
	delete_data(m_debug_render_queue);
}

void CLevel::OnAlifeSimulatorLoaded()
{
	MapManager().ResetStorage();
	GameTaskManager().ResetStorage();
	delete_data(m_debug_render_queue);
}

void CLevel::OnSessionTerminate(LPCSTR reason)
{
	MainMenu()->OnSessionTerminate(reason);
}

u32 GameID()
{
	return Game().Type();
}

CZoneList* CLevel::create_hud_zones_list()
{
	hud_zones_list = xr_new<CZoneList>();
	hud_zones_list->clear();
	return hud_zones_list;
}

bool CZoneList::feel_touch_contact(CObject* O)
{
	TypesMapIt it = m_TypesMap.find(O->cNameSect());
	bool res = (it != m_TypesMap.end());
	CCustomZone* pZone = smart_cast<CCustomZone*>(O);
	if (pZone && !pZone->IsEnabled())
	{
		res = false;
	}
	return res;
}

CZoneList::CZoneList()
{
}

CZoneList::~CZoneList()
{
	clear();
	destroy();
}

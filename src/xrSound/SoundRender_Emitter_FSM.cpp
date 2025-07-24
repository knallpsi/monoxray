#include "stdafx.h"
#pragma hdrstop

#include "SoundRender_Emitter.h"
#include "SoundRender_Core.h"
#include "SoundRender_Source.h"

//#define MEASURE_PROCESSING_TIME

#ifdef MEASURE_PROCESSING_TIME
	CTimer Timer;
#endif

XRSOUND_API extern float psSoundCull;

inline u32 calc_cursor(const float& fTimeStarted, float& fTime, const float& fTimeTotal, const WAVEFORMATEX& wfx)
{
	if (fTime < fTimeStarted)
		fTime = fTimeStarted; // Андрюха посоветовал, ассерт что ниже вылетел из за паузы как то хитро
	R_ASSERT((fTime-fTimeStarted)>=0.0f);
	while ((fTime - fTimeStarted) > fTimeTotal) //looped
	{
		fTime -= fTimeTotal;
	}
	u32 curr_sample_num = iFloor((fTime - fTimeStarted) * wfx.nSamplesPerSec);
	return curr_sample_num * (wfx.wBitsPerSample / 8) * wfx.nChannels;
}

void CSoundRender_Emitter::update(float dt)
{
	float fTime = SoundRender->fTimer_Value;
	float fDeltaTime = SoundRender->fTimer_Delta;

	VERIFY2(!!(owner_data) || (!(owner_data)&&(m_current_state==stStopped)), "owner");
	VERIFY2(owner_data?*(int*)(&owner_data->feedback):1, "owner");

#ifdef MEASURE_PROCESSING_TIME
	float time1 = Timer.GetElapsed_ms_f();
#endif

	if (bRewind)
	{
		if (target) SoundRender->i_rewind(this);
		bRewind = FALSE;
	}

	if (m_current_state != stStopped && !_valid(p_source.position))
	{
		m_current_state = stStopped;
	}

	// demonized: add preplay update, calculate delay based on distance and speed of sound
	// Adaptation for user experience
	// For distances < 30m (configurable via cvar), no delay
	// Then gradually ramp up to delay calculated by math
	// Starting at 90m, full delay kicks in
	// Don't apply delay to already delayed sounds, they are most likely already handled in respective parts of code and scripts
	if (need_preplay_update && m_current_state < stPlaying && starting_delay == 0.f)
	{
		if (_valid(p_source.position) && !p_source.position.similar(Fvector().set(0.f, 0.f, 0.f)) && owner_data && source() && source()->channels_num() == 1)
		{
			//Smooth Ramp Using Hermite(Smootherstep)
			static auto CalculateSmoothSoundDelay = [](float distance, float speedOfSound, float rampStart, float rampRange)
			{
				if (distance <= rampStart)
					return 0.0f;

				float delay = distance / speedOfSound;

				// Ramp factor from 0 to 1 between rampStart and rampStart + rampRange
				float t = std::clamp((distance - rampStart) / rampRange, 0.0f, 1.0f);

				// Smootherstep for smooth transition
				float smoothT = t * t * t * (t * (t * 6 - 15) + 10);

				return delay * smoothT;
			};

			float speedOfSound = 343.f;
			float oldDelay = starting_delay;
			auto oldState = m_current_state;
			auto delay = CalculateSmoothSoundDelay(p_source.position.distance_to(SoundRender->listener_position()), speedOfSound, soundSmoothingParams::distanceBasedDelayMinDistance, 60);

			// clamp delay in case of strange result
			delay = std::clamp(delay, 0.f, 3.5f);

			// apply cvar power
			delay *= soundSmoothingParams::distanceBasedDelayPower;
			if (delay > 0.f)
			{
				if (m_current_state == stStarting || m_current_state == stStartingLooped)
				{
					starting_delay = delay;
					m_current_state = m_current_state == stStarting ? stStartingDelayed : stStartingLoopedDelayed;
				}
				else
				{
					starting_delay += delay;
				}
				need_preplay_update = false;
				/*Msg("CSoundRender_Emitter::update, file %s, state %s, need_preplay_update, distance %.2f, old delay %.2f, delay %.2f",
					owner_data && source() && source()->file_name() ? source()->file_name() : "null",
					magic_enum::enum_name(oldState).data(),
					p_source.position.distance_to(SoundRender->listener_position()),
					oldDelay,
					starting_delay
				);*/
			}
		}
	}
	else
	{
		need_preplay_update = false;
	}

	switch (m_current_state)
	{
	case stStopped:
		break;
	case stStartingDelayed:
		if (iPaused) break;
		starting_delay -= dt;
		if (starting_delay <= 0)
			m_current_state = stStarting;
		break;
	case stStarting:
		if (iPaused) break;
		fTimeStarted = fTime;
		fTimeToStop = fTime + (get_length_sec() / psSpeedOfSound); 
		fTimeToPropagade = fTime;
		fade_volume = 1.f;
		occluder_volume = SoundRender->get_occlusion(p_source.position, .2f, occluder);
		smooth_volume = p_source.base_volume * p_source.volume * (owner_data->s_type == st_Effect
			                                                          ? psSoundVEffects * psSoundVFactor
			                                                          : psSoundVMusic * psSoundVMusicFactor) * (b2D ? 1.f : occluder_volume);
		e_current = e_target = *SoundRender->get_environment(p_source.position);
		if (update_culling(dt))
		{
			m_current_state = stPlaying;
			set_cursor(0);
			SoundRender->i_start(this);
		}
		else
			m_current_state = stSimulating;
		break;
	case stStartingLoopedDelayed:
		if (iPaused) break;
		starting_delay -= dt;
		if (starting_delay <= 0)
			m_current_state = stStartingLooped;
		break;
	case stStartingLooped:
		if (iPaused) break;
		fTimeStarted = fTime;
		fTimeToStop = 0xffffffff;
		fTimeToPropagade = fTime;
		fade_volume = 1.f;
		occluder_volume = SoundRender->get_occlusion(p_source.position, .2f, occluder);
		smooth_volume = p_source.base_volume * p_source.volume * (owner_data->s_type == st_Effect
			                                                          ? psSoundVEffects * psSoundVFactor
			                                                          : psSoundVMusic * psSoundVMusicFactor) * (b2D ? 1.f : occluder_volume);
		e_current = e_target = *SoundRender->get_environment(p_source.position);
		if (update_culling(dt))
		{
			m_current_state = stPlayingLooped;
			set_cursor(0);
			SoundRender->i_start(this);
		}
		else
			m_current_state = stSimulatingLooped;
		break;
	case stPlaying:
		if (iPaused)
		{
			if (target)
			{
				SoundRender->i_stop(this);
				m_current_state = stSimulating;
			}
			fTimeStarted += fDeltaTime;
			fTimeToStop += fDeltaTime;
			fTimeToPropagade += fDeltaTime;
			break;
		}
		if (fTime >= fTimeToStop)
		{
			// STOP
			m_current_state = stStopped;
			SoundRender->i_stop(this);
		}
		else
		{
			if (!update_culling(dt))
			{
				// switch to: SIMULATE
				m_current_state = stSimulating; // switch state
				SoundRender->i_stop(this);
			}
			else
			{
				// We are still playing
				update_environment(dt);
			}
		}
		break;
	case stSimulating:
		if (iPaused)
		{
			fTimeStarted += fDeltaTime;
			fTimeToStop += fDeltaTime;
			fTimeToPropagade += fDeltaTime;
			break;
		}
		if (fTime >= fTimeToStop)
		{
			// STOP
			m_current_state = stStopped;
		}
		else
		{
			u32 ptr = calc_cursor(fTimeStarted,
			                      fTime,
			                      get_length_sec(),
			                      source()->m_wformat);
			set_cursor(ptr);

			if (update_culling(dt))
			{
				// switch to: PLAY
				m_current_state = stPlaying;
				/*
								u32 ptr						= calc_cursor(	fTimeStarted, 
																			fTime, 
																			get_length_sec(), 
																			source()->m_wformat); 
								set_cursor					(ptr);
				*/
				SoundRender->i_start(this);
			}
		}
		break;
	case stPlayingLooped:
		if (iPaused)
		{
			if (target)
			{
				SoundRender->i_stop(this);
				m_current_state = stSimulatingLooped;
			}
			fTimeStarted += fDeltaTime;
			fTimeToPropagade += fDeltaTime;
			break;
		}
		if (!update_culling(dt))
		{
			// switch to: SIMULATE
			m_current_state = stSimulatingLooped; // switch state
			SoundRender->i_stop(this);
		}
		else
		{
			// We are still playing
			update_environment(dt);
		}
		break;
	case stSimulatingLooped:
		if (iPaused)
		{
			fTimeStarted += fDeltaTime;
			fTimeToPropagade += fDeltaTime;
			break;
		}
		if (update_culling(dt))
		{
			// switch to: PLAY
			m_current_state = stPlayingLooped; // switch state
			u32 ptr = calc_cursor(fTimeStarted,
			                      fTime,
			                      get_length_sec(),
			                      source()->m_wformat);
			set_cursor(ptr);

			SoundRender->i_start(this);
		}
		break;
	}

#ifdef MEASURE_PROCESSING_TIME
	float time2 = Timer.GetElapsed_ms_f();
#endif

	// if deffered stop active and volume==0 -> physically stop sound
	if (bStopping && fis_zero(fade_volume))
		i_stop();

	VERIFY2(!!(owner_data) || (!(owner_data)&&(m_current_state==stStopped)), "owner");
	VERIFY2(owner_data?*(int*)(owner_data->feedback):1, "owner");

	// footer
	bMoved = FALSE;
	if (m_current_state != stStopped)
	{
		if (fTime >= fTimeToPropagade)
			Event_Propagade();
	}
	else if (owner_data)
	{
		VERIFY(this==owner_data->feedback);
		owner_data->feedback = 0;
		owner_data = 0;
	}
	
#ifdef MEASURE_PROCESSING_TIME
	float time3 = Timer.GetElapsed_ms_f();
	const char* name = "?";
	if (owner_data && source())
	{
		name = source()->fname.c_str();
	}
	float d1 = time2 - time1;
	float d2 = time3 - time2;
	if (d1 > 1 || d2 > 1)
	{
		Msg("|SND| %s took long: %f %f", name, d1, d2);
	}
#endif
}

IC void volume_lerp(float& c, float t, float s, float dt)
{
	float diff = t - c;
	float diff_a = _abs(diff);
	if (diff_a < EPS_S) return;
	float mot = s * dt;
	if (mot > diff_a) mot = diff_a;
	c += (diff / diff_a) * mot;
}

#include "..\xrServerEntities\ai_sounds.h"

BOOL CSoundRender_Emitter::update_culling(float dt)
{
	if (b2D)
	{
		occluder_volume = 1.f;
		fade_volume += dt * 10.f * (bStopping ? -1.f : 1.f);
	}
	else
	{
		// Check range
		float dist = SoundRender->listener_position().distance_to(p_source.position);
		if (dist > p_source.max_distance)
		{
			smooth_volume = 0;
			return FALSE;
		}

		// Calc attenuated volume
		float att = p_source.min_distance / (psSoundRolloff * dist);
		clamp(att, 0.f, 1.f);
		float fade_scale = bStopping || (att * p_source.base_volume * p_source.volume * (owner_data->s_type == st_Effect
			                                                                                 ? psSoundVEffects *
			                                                                                 psSoundVFactor
			                                                                                 : psSoundVMusic * psSoundVMusicFactor) <
			                   psSoundCull)
			                   ? -1.f
			                   : 1.f;
		fade_volume += dt * 10.f * fade_scale;

		// Update occlusion
		float occ = (owner_data->g_type == SOUND_TYPE_WORLD_AMBIENT)
			            ? 1.0f
			            : SoundRender->get_occlusion(p_source.position, .2f, occluder);
		volume_lerp(occluder_volume, occ, 1.f, dt);
		clamp(occluder_volume, 0.f, 1.f);
	}
	clamp(fade_volume, 0.f, 1.f);
	// Update smoothing
	smooth_volume = .9f * smooth_volume + .1f * (p_source.base_volume * p_source.volume * (
			owner_data->s_type == st_Effect ? psSoundVEffects * psSoundVFactor : psSoundVMusic * psSoundVMusicFactor) * occluder_volume *
		fade_volume);
	if (smooth_volume < psSoundCull) return FALSE; // allow volume to go up
	// Here we has enought "PRIORITY" to be soundable
	// If we are playing already, return OK
	// --- else check availability of resources
	if (target) return TRUE;
	else return SoundRender->i_allow_play(this);
}

float CSoundRender_Emitter::priority()
{
	float dist = SoundRender->listener_position().distance_to(p_source.position);
	float att = p_source.min_distance / (psSoundRolloff * dist);
	clamp(att, 0.f, 1.f);
	return smooth_volume * att * priority_scale;
}

void CSoundRender_Emitter::update_environment(float dt)
{
	if (bMoved) {
		e_target = *SoundRender->get_environment(p_source.position);
		p_source.update_velocity(dt);
	}
	e_current.lerp(e_current, e_target, dt);
}

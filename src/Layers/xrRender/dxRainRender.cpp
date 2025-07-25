#include "stdafx.h"
#include "dxRainRender.h"

#include "../../xrEngine/Rain.h"

//	Warning: duplicated in rain.cpp
static const int max_desired_items = 2500;
static const float source_radius = 15;//12.5f;
static const float source_offset = 20.f; // 40
static const float max_distance = source_offset * 1.5f; //1.25f;
static const float sink_offset = -(max_distance - source_offset);
static const float drop_length = 5.f;
static const float drop_width = 0.30f;
static const float drop_angle = 3.0f;
static const float drop_max_angle = deg2rad(35.f); //10
static const float drop_max_wind_vel = 20.0f;
static const float drop_speed_min = 40.f;
static const float drop_speed_max = 80.f;

const int max_particles = 1000;
const int particles_cache = 400;
const float particles_time = .3f;

int current_items;

dxRainRender::dxRainRender()
{
	current_items = 0;

	IReader* F = FS.r_open("$game_meshes$", "dm\\rain.dm");
	VERIFY3(F, "Can't open file.", "dm\\rain.dm");

	DM_Drop = ::RImplementation.model_CreateDM(F);

	//
	SH_Rain.create("effects\\rain", "fx\\fx_rain");
	hGeom_Rain.create(FVF::F_LIT, RCache.Vertex.Buffer(), RCache.QuadIB);
	hGeom_Drops.create(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1, RCache.Vertex.Buffer(), RCache.Index.Buffer());

#if defined(USE_DX11)
	if (RImplementation.o.ssfx_rain)
		SH_Splash.create("effects\\rain_splash", "fx\\fx_rain");
#endif

	FS.r_close(F);
}

dxRainRender::~dxRainRender()
{
	::RImplementation.model_Delete(DM_Drop);
}

void dxRainRender::Copy(IRainRender& _in)
{
	*this = *(dxRainRender*)&_in;
}

#include "../../xrEngine/iGame_persistent.h"

void dxRainRender::Render(CEffect_Rain& owner)
{
	float factor = g_pGamePersistent->Environment().CurrentEnv->rain_density;
	if (factor < EPS_L) return;

	float _drop_len = drop_length;
	float _drop_width = drop_width;
	float _drop_speed = 1.0f;
	ref_shader& _splash_SH = DM_Drop->shader;
	static shared_str s_shader_setup = "ssfx_rain_setup";

	int rain_max_particles = max_desired_items;
	float rain_radius = source_radius;

	// SSS Rain shader is available
#if defined(USE_DX11)
	if (RImplementation.o.ssfx_rain)
	{
		_drop_len = ps_ssfx_rain_1.x;
		_drop_width = ps_ssfx_rain_1.y;
		_drop_speed = ps_ssfx_rain_1.z;
		_splash_SH = SH_Splash;
		rain_max_particles = ps_ssfx_rain_drops_setup.x;
		rain_radius = ps_ssfx_rain_drops_setup.y;
	}
#endif

	u32 desired_items = iFloor(0.01f * (1.f + factor * 99.0f) * float(rain_max_particles));

	// Get to the desired items
	if (current_items < desired_items)
		current_items += desired_items - current_items;

	// visual
	float factor_visual = factor / 2.f + .5f;
	Fvector3 f_rain_color = g_pGamePersistent->Environment().CurrentEnv->rain_color;
	u32 u_rain_color = color_rgba_f(f_rain_color.x, f_rain_color.y, f_rain_color.z, factor_visual);

	// born _new_ if needed
	float b_radius_wrap_sqr = _sqr((rain_radius * 1.5f));
	if (owner.items.size() < current_items)
	{
		// owner.items.reserve		(desired_items);
		while (owner.items.size() < current_items)
		{
			CEffect_Rain::Item one;
			owner.Born(one, rain_radius, _drop_speed);
			owner.items.push_back(one);
		}
	}

	// build source plane
	Fplane src_plane;
	Fvector norm = {0.f, -1.f, 0.f};
	Fvector upper;
	upper.set(Device.vCameraPosition.x, Device.vCameraPosition.y + source_offset, Device.vCameraPosition.z);
	src_plane.build(upper, norm);

	// perform update
	u32 vOffset;
	FVF::LIT* verts = (FVF::LIT *)RCache.Vertex.Lock(desired_items * 4, hGeom_Rain->vb_stride, vOffset);
	FVF::LIT* start = verts;
	const Fvector& vEye = Device.vCameraPosition;
	for (u32 I = 0; I < current_items; I++)
	{
		// physics and time control
		CEffect_Rain::Item& one = owner.items[I];

		if (one.dwTime_Hit < Device.dwTimeGlobal) 
		{
			owner.Hit(one.Phit);
			if (current_items > desired_items) current_items--; // Hit something
		}
		if (one.dwTime_Life < Device.dwTimeGlobal)
		{
			owner.Born(one, rain_radius, _drop_speed);
			if (current_items > desired_items) current_items--; // Out of life ( invalidated, never hit something, etc. )
		}

		// последняя дельта ??
		//.		float xdt		= float(one.dwTime_Hit-Device.dwTimeGlobal)/1000.f;
		//.		float dt		= Device.fTimeDelta;//xdt<Device.fTimeDelta?xdt:Device.fTimeDelta;
		float dt = Device.fTimeDelta;
		one.P.mad(one.D, one.fSpeed * dt);

		Device.Statistic->TEST1.Begin();
		Fvector wdir;
		wdir.set(one.P.x - vEye.x, 0, one.P.z - vEye.z);
		float wlen = wdir.square_magnitude();
		if (wlen > b_radius_wrap_sqr)
		{
			wlen = _sqrt(wlen);
			//.			Device.Statistic->TEST3.Begin();
			if ((one.P.y - vEye.y) < sink_offset)
			{
				// need born
				one.invalidate();
			}
			else
			{
				Fvector inv_dir, src_p;
				inv_dir.invert(one.D);
				wdir.div(wlen);
				one.P.mad(one.P, wdir, -(wlen + rain_radius));
				if (src_plane.intersectRayPoint(one.P, inv_dir, src_p))
				{
					float dist_sqr = one.P.distance_to_sqr(src_p);
					float height = max_distance;
					if (owner.RayPick(src_p, one.D, height, collide::rqtBoth))
					{
						if (_sqr(height) <= dist_sqr)
						{
							one.invalidate(); // need born
							//							Log("1");
						}
						else
						{
							owner.RenewItem(one, height - _sqrt(dist_sqr),TRUE); // fly to point
							//							Log("2",height-dist);
						}
					}
					else
					{
						owner.RenewItem(one, max_distance - _sqrt(dist_sqr),FALSE); // fly ...
						//						Log("3",1.5f*b_height-dist);
					}
				}
				else
				{
					// need born
					one.invalidate();
					//					Log("4");
				}
			}
			//.			Device.Statistic->TEST3.End();
		}
		Device.Statistic->TEST1.End();

		// Build line
		Fvector& pos_head = one.P;
		Fvector pos_trail;
		pos_trail.mad(pos_head, one.D, -_drop_len * factor_visual);

		// Culling
		Fvector sC, lineD;
		float sR;
		sC.sub(pos_head, pos_trail);
		lineD.normalize(sC);
		sC.mul(.5f);
		sR = sC.magnitude();
		sC.add(pos_trail);
		if (!::Render->ViewBase.testSphere_dirty(sC, sR)) continue;

		static Fvector2 UV[2][4] = {
			{{0, 1}, {0, 0}, {1, 1}, {1, 0}},
			{{1, 0}, {1, 1}, {0, 0}, {0, 1}}
		};

		// Everything OK - build vertices
		Fvector P, lineTop, camDir;
		camDir.sub(sC, vEye);
		camDir.normalize();
		lineTop.crossproduct(camDir, lineD);
		float w = _drop_width;
		u32 s = one.uv_set;
		P.mad(pos_trail, lineTop, -w);
		verts->set(P, u_rain_color, UV[s][0].x, UV[s][0].y);
		verts++;
		P.mad(pos_trail, lineTop, w);
		verts->set(P, u_rain_color, UV[s][1].x, UV[s][1].y);
		verts++;
		P.mad(pos_head, lineTop, -w);
		verts->set(P, u_rain_color, UV[s][2].x, UV[s][2].y);
		verts++;
		P.mad(pos_head, lineTop, w);
		verts->set(P, u_rain_color, UV[s][3].x, UV[s][3].y);
		verts++;
	}
	u32 vCount = (u32)(verts - start);
	RCache.Vertex.Unlock(vCount, hGeom_Rain->vb_stride);

	// Render if needed
	if (vCount)
	{
		//HW.pDevice->SetRenderState	(D3DRS_CULLMODE,D3DCULL_NONE);
		RCache.set_CullMode(CULL_NONE);
		RCache.set_xform_world(Fidentity);
		RCache.set_Shader(SH_Rain);
		RCache.set_Geometry(hGeom_Rain);
		RCache.Render(D3DPT_TRIANGLELIST, vOffset, 0, vCount, 0, vCount / 2);
		//HW.pDevice->SetRenderState	(D3DRS_CULLMODE,D3DCULL_CCW);
		RCache.set_CullMode(CULL_CCW);
		RCache.set_c(s_shader_setup, ps_ssfx_rain_2); // Alpha, Brigthness, Refraction, Reflection
	}

	// Particles
	CEffect_Rain::Particle* P = owner.particle_active;
	if (0 == P) return;

	{
		float dt = Device.fTimeDelta;
		_IndexStream& _IS = RCache.Index;
		RCache.set_Shader(_splash_SH);
		RCache.set_c(s_shader_setup, ps_ssfx_rain_3); // Alpha, Refraction

		Fmatrix mXform, mScale;
		int pcount = 0;
		u32 v_offset, i_offset;
		u32 vCount_Lock = particles_cache * DM_Drop->number_vertices;
		u32 iCount_Lock = particles_cache * DM_Drop->number_indices;
		IRender_DetailModel::fvfVertexOut* v_ptr = (IRender_DetailModel::fvfVertexOut*)RCache.Vertex.Lock(vCount_Lock,
		                                                                                                  hGeom_Drops->
		                                                                                                  vb_stride,
		                                                                                                  v_offset);
		u16* i_ptr = _IS.Lock(iCount_Lock, i_offset);
		while (P)
		{
			CEffect_Rain::Particle* next = P->next;

			// Update
			// P can be zero sometimes and it crashes
			P->time -= dt;
			if (P->time < 0)
			{
				owner.p_free(P);
				P = next;
				continue;
			}

			// Render
			if (::Render->ViewBase.testSphere_dirty(P->bounds.P, P->bounds.R))
			{
				// Build matrix
				float scale = P->time / particles_time;
				mScale.scale(scale, scale, scale);
				mXform.mul_43(P->mXForm, mScale);

				// XForm verts
				DM_Drop->transfer(mXform, v_ptr, u_rain_color, i_ptr, pcount * DM_Drop->number_vertices);
				v_ptr += DM_Drop->number_vertices;
				i_ptr += DM_Drop->number_indices;
				pcount ++;

				if (pcount >= particles_cache)
				{
					// flush
					u32 dwNumPrimitives = iCount_Lock / 3;
					RCache.Vertex.Unlock(vCount_Lock, hGeom_Drops->vb_stride);
					_IS.Unlock(iCount_Lock);
					RCache.set_Geometry(hGeom_Drops);
					RCache.Render(D3DPT_TRIANGLELIST, v_offset, 0, vCount_Lock, i_offset, dwNumPrimitives);

					v_ptr = (IRender_DetailModel::fvfVertexOut*)RCache.Vertex.Lock(vCount_Lock, hGeom_Drops->vb_stride,
					                                                               v_offset);
					i_ptr = _IS.Lock(iCount_Lock, i_offset);

					pcount = 0;
				}
			}

			P = next;
		}

		// Flush if needed
		vCount_Lock = pcount * DM_Drop->number_vertices;
		iCount_Lock = pcount * DM_Drop->number_indices;
		u32 dwNumPrimitives = iCount_Lock / 3;
		RCache.Vertex.Unlock(vCount_Lock, hGeom_Drops->vb_stride);
		_IS.Unlock(iCount_Lock);
		if (pcount)
		{
			RCache.set_Geometry(hGeom_Drops);
			RCache.Render(D3DPT_TRIANGLELIST, v_offset, 0, vCount_Lock, i_offset, dwNumPrimitives);
		}
	}
}

const Fsphere& dxRainRender::GetDropBounds() const
{
	return DM_Drop->bv_sphere;
}

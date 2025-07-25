#include "stdafx.h"
#pragma hdrstop

#include "r_backend_xform.h"

void R_xforms::set_W(const Fmatrix& m)
{
	m_w.set(m);
	m_wv.mul_43(m_v, m_w);
	m_wvp.mul(m_p, m_wv);
	if (c_w) RCache.set_c(c_w, m_w);
	if (c_wv) RCache.set_c(c_wv, m_wv);
	if (c_wvp) RCache.set_c(c_wvp, m_wvp);
	m_bInvWValid = false;
	if (c_invw) apply_invw();
	RCache.set_xform(D3DTS_WORLD, m);
}

void R_xforms::set_V(const Fmatrix& m)
{
	m_v.set(m);
	m_wv.mul_43(m_v, m_w);
	m_vp.mul(m_p, m_v);
	m_wvp.mul(m_p, m_wv);
	if (c_v) RCache.set_c(c_v, m_v);
	if (c_vp) RCache.set_c(c_vp, m_vp);
	if (c_wv) RCache.set_c(c_wv, m_wv);
	if (c_wvp) RCache.set_c(c_wvp, m_wvp);
	RCache.set_xform(D3DTS_VIEW, m);
}

void R_xforms::set_P(const Fmatrix& m)
{
	m_p.set(m);
	m_vp.mul(m_p, m_v);
	m_wvp.mul(m_p, m_wv);
	if (c_p) RCache.set_c(c_p, m_p);
	if (c_vp) RCache.set_c(c_vp, m_vp);
	if (c_wvp) RCache.set_c(c_wvp, m_wvp);
	// always setup projection - D3D relies on it to work correctly :(
	RCache.set_xform(D3DTS_PROJECTION, m);
}

void R_xforms::set_W_prev(const Fmatrix& m)
{
	m_w_prev.set(m);
	m_wv_prev.mul_43(m_v_prev, m_w_prev);
	m_wvp_prev.mul(m_p_prev, m_wv_prev);

	if (c_w_prev)		RCache.set_c(c_w_prev, m_w_prev);
	if (c_wv_prev)		RCache.set_c(c_wv_prev, m_wv_prev);
	if (c_wvp_prev)		RCache.set_c(c_wvp_prev, m_wvp_prev);
}
void R_xforms::set_V_prev(const Fmatrix& m)
{
	m_v_prev.set(m);
	m_wv_prev.mul_43(m_v_prev, m_w_prev);
	m_vp_prev.mul(m_p_prev, m_v_prev);
	m_wvp_prev.mul(m_p_prev, m_wv_prev);

	if (c_v_prev)		RCache.set_c(c_v_prev, m_v_prev);
	if (c_vp_prev)		RCache.set_c(c_vp_prev, m_vp_prev);
	if (c_wv_prev)		RCache.set_c(c_wv_prev, m_wv_prev);
	if (c_wvp_prev)		RCache.set_c(c_wvp_prev, m_wvp_prev);
}
void R_xforms::set_P_prev(const Fmatrix& m)
{
	m_p_prev.set(m);
	m_vp_prev.mul(m_p_prev, m_v_prev);
	m_wvp_prev.mul(m_p_prev, m_wv_prev);
	if (c_p_prev)		RCache.set_c(c_p_prev, m_p_prev);
	if (c_vp_prev)		RCache.set_c(c_vp_prev, m_vp_prev);
	if (c_wvp_prev)		RCache.set_c(c_wvp_prev, m_wvp_prev);
}

void R_xforms::apply_invw()
{
	VERIFY(c_invw);

	if (!m_bInvWValid)
	{
		m_invw.invert_b(m_w);
		m_bInvWValid = true;
	}

	RCache.set_c(c_invw, m_invw);
}

void R_xforms::unmap()
{
	c_w = NULL;
	c_invw = NULL;
	c_v = NULL;
	c_p = NULL;
	c_wv = NULL;
	c_vp = NULL;
	c_wvp = NULL;

	c_w_prev = NULL;
	c_v_prev = NULL;
	c_p_prev = NULL;
	c_wv_prev = NULL;
	c_vp_prev = NULL;
	c_wvp_prev = NULL;
}

R_xforms::R_xforms()
{
	unmap();
	m_w.identity();
	m_invw.identity();
	m_v.identity();
	m_p.identity();
	m_wv.identity();
	m_vp.identity();
	m_wvp.identity();

	m_w_prev.identity();
	m_v_prev.identity();
	m_p_prev.identity();
	m_wv_prev.identity();
	m_vp_prev.identity();
	m_wvp_prev.identity();

	m_bInvWValid = true;
}

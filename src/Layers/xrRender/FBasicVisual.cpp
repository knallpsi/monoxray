// dxRender_Visual.cpp: implementation of the dxRender_Visual class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#ifndef _EDITOR
#	include "../../xrEngine/render.h"
#endif // #ifndef _EDITOR

#include "fbasicvisual.h"
#include "../../xrEngine/fmesh.h"
#include "dxRenderDeviceRender.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IRender_Mesh::~IRender_Mesh()
{
	_RELEASE(p_rm_Vertices);
	_RELEASE(p_rm_Indices);
}

dxRender_Visual::dxRender_Visual()
{
	Type = 0;
	shader = 0;
	vis.clear();
}

dxRender_Visual::~dxRender_Visual()
{
}

void dxRender_Visual::Release()
{
}

//CStatTimer						tscreate;

void dxRender_Visual::Load(const char* N, IReader* data, u32)
{
	dbg_name = N;
	skinning = ::Render->m_skinning;

	// header
	VERIFY(data);
	ogf_header hdr;
	if (data->r_chunk_safe(OGF_HEADER, &hdr, sizeof(hdr)))
	{
		R_ASSERT2(hdr.format_version==xrOGF_FormatVersion, "Invalid visual version");
		Type = hdr.type;
		//if (hdr.shader_id)	shader	= ::Render->getShader	(hdr.shader_id);
		if (hdr.shader_id) shader = ::RImplementation.getShader(hdr.shader_id);
		vis.box.set(hdr.bb.min, hdr.bb.max);
		vis.sphere.set(hdr.bs.c, hdr.bs.r);
	}
	else
	{
		FATAL("Invalid visual");
	}

	// Shader
	if (data->find_chunk(OGF_TEXTURE))
	{
		string256 fnT, fnS;
		data->r_stringZ(fnT, sizeof(fnT));
		data->r_stringZ(fnS, sizeof(fnS));
		dbg_shader_def = fnS;
		dbg_texture_def = fnT;
		ResetShaderTexture();
	}

	// desc
#ifdef _EDITOR
    if (data->find_chunk(OGF_S_DESC)) 
	    desc.Load		(*data);
#endif
}

//--DSR-- HeatVision_start
CTexture* dxRender_Visual::GetTexture()
{
	Shader* pSh = shader._get();
	if (pSh == 0) 
		return 0;

	ShaderElement* pShE = pSh->E[0]._get();
	if (pShE == 0 || pShE->passes.empty()) 
		return 0;

	SPass* pPass = pShE->passes[0]._get();
	if (pPass == 0)
		return 0;

	STextureList* pTexList = pPass->T._get();
	if (pTexList == 0 || pTexList->empty()) 
		return 0;

	return pTexList->at(0).second._get();
}

void dxRender_Visual::MarkAsHot(bool is_hot) 
{
	auto texture = GetTexture();
	if (texture)
		texture->m_is_hot = is_hot;
}
//--DSR-- HeatVision_end

//--DSR-- SilencerOverheat_start
void dxRender_Visual::MarkAsGlowing(bool is_glowing)
{
	auto texture = GetTexture();
	if (texture)
		texture->m_is_glowing = is_glowing;
}
//--DSR-- SilencerOverheat_end

void dxRender_Visual::SetShaderTexture(LPCSTR s_shader, LPCSTR s_texture)
{
	if (s_shader && strlen(s_shader))
	{
		char* shader = xr_strdup(s_shader);
		char* no_shadow = strstr(shader, "$no_shadows");

		if (no_shadow)
		{
			flags.set(IRenderVisualFlags::eNoShadow, TRUE);
			*no_shadow = 0;
		}
		else
			flags.set(IRenderVisualFlags::eNoShadow, FALSE);

		dbg_shader = shader;
		xr_delete(shader);
	}

	if (s_texture && strlen(s_texture))
	{
		dbg_texture = s_texture;
	}

	::Render->m_skinning = skinning;
	shader.create(*dbg_shader, *dbg_texture);
}

void dxRender_Visual::ResetShaderTexture()
{
	if (!dbg_shader.equal(dbg_shader_def) || !dbg_texture.equal(dbg_texture_def))
		SetShaderTexture(*dbg_shader_def, *dbg_texture_def);
}

#define PCOPY(a)	a = pFrom->a

void dxRender_Visual::Copy(dxRender_Visual* pFrom)
{
	PCOPY(Type);
	PCOPY(shader);
	PCOPY(vis);
#ifdef _EDITOR
	PCOPY(desc);
#endif
	PCOPY(flags);
	PCOPY(dbg_name);
	PCOPY(dbg_shader);
	PCOPY(dbg_shader_def);
	PCOPY(dbg_texture);
	PCOPY(dbg_texture_def);
	PCOPY(skinning);
}

// FHierrarhyVisual.cpp: implementation of the FHierrarhyVisual class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#include "FHierrarhyVisual.h"
#include "../../xrEngine/Fmesh.h"
#ifndef _EDITOR
#include "../../xrEngine/render.h"
#else
#include "../../Include/xrAPI/xrAPI.h"
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

FHierrarhyVisual::FHierrarhyVisual() : dxRender_Visual()
{
	bDontDelete = FALSE;
}

FHierrarhyVisual::~FHierrarhyVisual()
{
	if (!bDontDelete)
	{
		for (u32 i = 0; i < children.size(); i++)
			::Render->model_Delete((IRenderVisual *&)children[i]);
	}
	children.clear();
}

void FHierrarhyVisual::Release()
{
	if (!bDontDelete)
	{
		for (u32 i = 0; i < children.size(); i++)
			((dxRender_Visual*)children[i])->Release();
	}
}

void FHierrarhyVisual::Load(const char* N, IReader* data, u32 dwFlags)
{
	dxRender_Visual::Load(N, data, dwFlags);
	if (data->find_chunk(OGF_CHILDREN_L))
	{
		// From Link
		u32 cnt = data->r_u32();
		children.resize(cnt);
		for (u32 i = 0; i < cnt; i++)
		{
#ifdef _EDITOR
			THROW;
#else
			u32 ID = data->r_u32();
			children[i] = ::Render->getVisual(ID);
#endif
		}
		bDontDelete = TRUE;
	}
	else
	{
		if (data->find_chunk(OGF_CHILDREN))
		{
			// From stream
			IReader* OBJ = data->open_chunk(OGF_CHILDREN);
			if (OBJ)
			{
				IReader* O = OBJ->open_chunk(0);
				for (int count = 1; O; count++)
				{
					string_path name_load, short_name, num;
					xr_strcpy(short_name, N);
					if (strext(short_name)) *strext(short_name) = 0;
					strconcat(sizeof(name_load), name_load, short_name, ":", itoa(count, num, 10));
					children.push_back(::Render->model_CreateChild(name_load, O));
					O->close();
					O = OBJ->open_chunk(count);
				}
				OBJ->close();
			}
			bDontDelete = FALSE;
		}
		else
		{
			FATAL("Invalid visual");
		}
	}
}

//--DSR-- HeatVision_start
void FHierrarhyVisual::MarkAsHot(bool is_hot) 
{
	dxRender_Visual::MarkAsHot(is_hot);
	for (u32 i = 0; i < children.size(); i++)
		children[i]->MarkAsHot(is_hot);
}
//--DSR-- HeatVision_end

void FHierrarhyVisual::Copy(dxRender_Visual* pSrc)
{
	dxRender_Visual::Copy(pSrc);

	FHierrarhyVisual* pFrom = (FHierrarhyVisual *)pSrc;

	children.clear();
	children.reserve(pFrom->children.size());
	for (u32 i = 0; i < pFrom->children.size(); i++)
	{
		IRenderVisual* p = ::Render->model_Duplicate(pFrom->children[i]);
		children.push_back(p);
	}
	bDontDelete = FALSE;
}

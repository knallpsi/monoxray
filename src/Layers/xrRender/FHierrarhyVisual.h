// FHierrarhyVisual.h: interface for the FHierrarhyVisual class.
//
//////////////////////////////////////////////////////////////////////

#ifndef FHierrarhyVisualH
#define FHierrarhyVisualH

#pragma once

#include "fbasicvisual.h"

class FHierrarhyVisual : public dxRender_Visual
{
public:
	xr_vector<IRenderVisual*> children;
	BOOL bDontDelete;
public:
	FHierrarhyVisual();
	virtual ~FHierrarhyVisual();

	virtual void Load(const char* N, IReader* data, u32 dwFlags);
	virtual void Copy(dxRender_Visual* pFrom);
	virtual void Release();

	//--DSR-- HeatVision_start
	virtual void MarkAsHot(bool is_hot);
	//--DSR-- HeatVision_end

	virtual xr_vector<IRenderVisual*>* get_children() { return &children; };
};

#endif //FHierrarhyVisualH

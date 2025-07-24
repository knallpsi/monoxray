#ifndef FBasicVisualH
#define FBasicVisualH
#pragma once

#include "../../xrEngine/vis_common.h"

#include "../../Include/xrRender/RenderVisual.h"

#define VLOAD_NOVERTICES		(1<<0)

// The class itself
class CKinematicsAnimated;
class CKinematics;
class IParticleCustom;

struct IRender_Mesh
{
	// format
	ref_geom rm_geom;

	// verts
	ID3DVertexBuffer* p_rm_Vertices;
	u32 vBase;
	u32 vCount;

	// indices
	ID3DIndexBuffer* p_rm_Indices;
	u32 iBase;
	u32 iCount;
	u32 dwPrimitives;

	IRender_Mesh()
	{
		p_rm_Vertices = 0;
		p_rm_Indices = 0;
	}

	virtual ~IRender_Mesh();
private:
	IRender_Mesh(const IRender_Mesh& other);
	void operator=(const IRender_Mesh& other);
};

// The class itself
class ECORE_API dxRender_Visual : public IRenderVisual
{
public:
#ifdef _EDITOR
    ogf_desc					desc		;
#endif
	shared_str					dbg_name	;
	shared_str					dbg_shader	;
	shared_str					dbg_texture	;
	shared_str					dbg_shader_def	;
	shared_str					dbg_texture_def	;
	virtual shared_str	_BCL	getDebugName() { return dbg_name; }
	virtual LPCSTR _BCL			getDebugShader() { return *dbg_shader; }
	virtual LPCSTR _BCL			getDebugTexture() { return *dbg_texture; }
	virtual LPCSTR _BCL			getDebugShaderDef() { return *dbg_shader_def; }
	virtual LPCSTR _BCL			getDebugTextureDef() { return *dbg_texture_def; }
public:
	// Common data for rendering
	u32 Type; // visual's type
	vis_data vis; // visibility-data
	ref_shader shader; // pipe state, shared
	s32 skinning;

	virtual void Render(float LOD)
	{
	}; // LOD - Level Of Detail  [0..1], Ignored
	virtual void Load(const char* N, IReader* data, u32 dwFlags);
	virtual void Release(); // Shared memory release
	virtual void Copy(dxRender_Visual* from);

	virtual void Spawn()
	{
	};

	virtual void Depart()
	{
	};

	//	virtual	CKinematics*		dcast_PKinematics			()				{ return 0;	}
	//	virtual	CKinematicsAnimated*dcast_PKinematicsAnimated	()				{ return 0;	}
	//	virtual IParticleCustom*	dcast_ParticleCustom		()				{ return 0;	}

	virtual void SetShaderTexture(LPCSTR shader, LPCSTR texture);
	virtual void ResetShaderTexture();

	virtual vis_data& _BCL getVisData() { return vis; }
	virtual u32 getType() { return Type; }
	
	CTexture* GetTexture();							//--DSR--
	virtual void MarkAsHot(bool is_hot);			//--DSR-- HeatVision
	virtual void MarkAsGlowing(bool is_glowing);	//--DSR-- SilencerOverheat

	dxRender_Visual();
	virtual ~dxRender_Visual();
};

#endif // !FBasicVisualH

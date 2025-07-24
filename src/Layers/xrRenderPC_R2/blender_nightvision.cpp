#include "stdafx.h"

#include "blender_nightvision.h"

CBlender_nightvision::CBlender_nightvision() { description.CLS = 0; }
CBlender_fakescope::CBlender_fakescope() { description.CLS = 0; } //crookr
CBlender_heatvision::CBlender_heatvision() { description.CLS = 0;  } //--DSR-- HeatVision

CBlender_nightvision::~CBlender_nightvision()
{
}

CBlender_fakescope::~CBlender_fakescope() //crookr
{
}

CBlender_heatvision::~CBlender_heatvision() //--DSR-- HeatVision_start
{
}

void CBlender_nightvision::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);
	switch (C.iElement)
	{
	case 0: //Dummy shader - because IDK what gonna happen when r2_nightvision will be 0
		C.r_Pass("null", "copy", FALSE, FALSE, FALSE);
		C.r_Sampler_clf("s_base", r2_RT_generic0);
		C.r_End();
		break;
	case 1:	
		C.r_Pass("null", "nightvision_gen_1", FALSE, FALSE, FALSE);	
		C.r_Sampler_rtf("s_position", r2_RT_P);
		C.r_Sampler_clf("s_image", r2_RT_generic0);	
		C.r_End();
		break;
	case 2:	
		C.r_Pass("null", "nightvision_gen_2", FALSE, FALSE, FALSE);	
		C.r_Sampler_rtf("s_position", r2_RT_P);
		C.r_Sampler_clf("s_image", r2_RT_generic0);	
		C.r_End();
		break;
	case 3:	
		C.r_Pass("null", "nightvision_gen_3", FALSE, FALSE, FALSE);	
		C.r_Sampler_rtf("s_position", r2_RT_P);
		C.r_Sampler_clf("s_image", r2_RT_generic0);	
		C.r_End();
		break;	
	}
}

void CBlender_fakescope::Compile(CBlender_Compile& C) //crookr
{
	IBlender::Compile(C);
	switch (C.iElement)
	{
	case 0: //Main pass
		C.r_Pass("null", "fakescope", FALSE, FALSE, FALSE);
		C.r_Sampler_rtf("s_position", r2_RT_P);
		C.r_Sampler_clf("s_image", r2_RT_generic0);
		C.r_Sampler_clf("s_blur_2", r2_RT_blur_2);
		C.r_Sampler_clf("s_blur_4", r2_RT_blur_4);
		C.r_Sampler_clf("s_blur_8", r2_RT_blur_8);
		C.r_Sampler_clf("s_scope", r2_RT_scopert);
		C.r_End();
		break;
	case 1: //Copy to rt_Generic_2
		C.r_Pass("null", "copy", FALSE, FALSE, FALSE);	
		C.r_Sampler_clf("s_base", "$user$generic_pingpong");	
		C.r_End();
		break;
	}
}

//--DSR-- HeatVision_start
void CBlender_heatvision::Compile(CBlender_Compile& C)
{
	IBlender::Compile(C);
	C.r_Pass("null", "heatvision", FALSE, FALSE, FALSE);
	C.r_Sampler_rtf("s_position", r2_RT_P);
	C.r_Sampler_clf("s_image", r2_RT_generic0);
	C.r_End();	
}
//--DSR-- HeatVision_end

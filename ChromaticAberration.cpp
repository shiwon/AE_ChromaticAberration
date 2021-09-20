/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2007 Adobe Systems Incorporated                       */
/* All Rights Reserved.                                            */
/*                                                                 */
/* NOTICE:  All information contained herein is, and remains the   */
/* property of Adobe Systems Incorporated and its suppliers, if    */
/* any.  The intellectual and technical concepts contained         */
/* herein are proprietary to Adobe Systems Incorporated and its    */
/* suppliers and may be covered by U.S. and Foreign Patents,       */
/* patents in process, and are protected by trade secret or        */
/* copyright law.  Dissemination of this information or            */
/* reproduction of this material is strictly forbidden unless      */
/* prior written permission is obtained from Adobe Systems         */
/* Incorporated.                                                   */
/*                                                                 */
/*******************************************************************/

/*	ChromaticAberration.cpp	

	This is a compiling husk of a project. Fill it in with interesting
	pixel processing code.
	
	Revision History

	Version		Change													Engineer	Date
	=======		======													========	======
	1.0			(seemed like a good idea at the time)					bbb			6/1/2002

	1.0			Okay, I'm leaving the version at 1.0,					bbb			2/15/2006
				for obvious reasons; you're going to 
				copy these files directly! This is the
				first XCode version, though.

	1.0			Let's simplify this barebones sample					zal			11/11/2010

	1.0			Added new entry point									zal			9/18/2017

*/

#include "ChromaticAberration.h"

static PF_Err 
About (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	AEGP_SuiteHandler suites(in_data->pica_basicP);
	
	suites.ANSICallbacksSuite1()->sprintf(	out_data->return_msg,
											"%s v%d.%d\r%s",
											STR(StrID_Name), 
											MAJOR_VERSION, 
											MINOR_VERSION, 
											STR(StrID_Description));
	return PF_Err_NONE;
}

static PF_Err 
GlobalSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	out_data->my_version = PF_VERSION(	MAJOR_VERSION, 
										MINOR_VERSION,
										BUG_VERSION, 
										STAGE_VERSION, 
										BUILD_VERSION);

	out_data->out_flags =  PF_OutFlag_DEEP_COLOR_AWARE;	// just 16bpc, not 32bpc
	
	return PF_Err_NONE;
}

static PF_Err 
ParamsSetup (	
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err		err		= PF_Err_NONE;
	PF_ParamDef	def;	

	AEFX_CLR_STRUCT(def);

	PF_ADD_ANGLE(			STR(StrID_Angle_Param_Name),
							SKELETON_ANGLE_DFLT,
							ANGLE_DISK_ID);

	AEFX_CLR_STRUCT(def);

	PF_ADD_FLOAT_SLIDERX(	STR(StrID_Shift_Param_Name),
							SKELETON_SHIFT_MIN,
							SKELETON_SHIFT_MAX,
							SKELETON_SHIFT_MIN,
							SKELETON_SHIFT_MAX,
							SKELETON_SHIFT_DFLT,
							PF_Precision_HUNDREDTHS,
							0,
							0,
							SHIFT_DISK_ID);


	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER(			STR(StrID_Type_Param_Name),
							0, 2, 0, 2, 0,
							TYPE_DISK_ID);
	
	out_data->num_params = SKELETON_NUM_PARAMS;

	return err;
}

static PF_Err
MySimpleGainFunc16 (
	void		*refcon, 
	A_long		xL, 
	A_long		yL, 
	PF_Pixel16	*inP, 
	PF_Pixel16	*outP)
{
	PF_Err		err = PF_Err_NONE;
	LayerInfo* lP = reinterpret_cast<LayerInfo*>(refcon);
	if (lP) {
		PF_Pixel16* pel = (PF_Pixel16*)((A_u_char*)lP->inPP16 + (yL * lP->rowbytes)) + xL;
		PF_Pixel16* pelL = lP->inPP16;
		PF_Pixel16* pelR = lP->inPP16;
		A_long dlx = xL + lP->dx;
		A_long dly = yL + lP->dy;
		A_long drx = xL - lP->dx;
		A_long dry = yL - lP->dy;
		if (0 <= dlx && dlx < lP->width && 0 <= dly && dly < lP->height) {
			pelL = (PF_Pixel16*)((A_u_char*)lP->inPP16 + (dly * lP->rowbytes)) + dlx;
		}
		else {
			pelL->alpha = 0;
			pelL->blue = 0;
			pelL->green = 0;
			pelL->red = 0;
		}
		if (0 <= drx && drx < lP->width && 0 <= dry && dry < lP->height) {
			pelR = (PF_Pixel16*)((A_u_char*)lP->inPP16 + (dry * lP->rowbytes)) + drx;
		}
		else {
			pelR->alpha = 0;
			pelR->blue = 0;
			pelR->green = 0;
			pelR->red = 0;
		}

		outP->alpha = MAX(pel->alpha, MAX(pelR->alpha, pelL->alpha));
		switch (lP->type) {
		case 0:
			//RG
			outP->red = pelL->red;
			outP->green = pelR->green;
			outP->blue = pel->blue;
			break;
		case 1:
			//GB
			outP->red = pel->red;
			outP->green = pelL->green;
			outP->blue = pelR->blue;
			break;
		default:
			//BR
			outP->red = pelR->red;
			outP->green = pel->green;
			outP->blue = pelL->blue;
			break;
		}
	}

	return err;
}

static PF_Err
MySimpleGainFunc8 (
	void*	refcon,
	A_long		xL, 
	A_long		yL, 
	PF_Pixel8	*inP, 
	PF_Pixel8	*outP)
{
	PF_Err		err = PF_Err_NONE;
	LayerInfo* lP = reinterpret_cast<LayerInfo*>(refcon);
	if (lP){
		PF_Pixel8* pel = (PF_Pixel8*)((A_u_char*)lP->inPP8 + (yL * lP->rowbytes)) + xL;
		PF_Pixel8* pelL = lP->inPP8;
		PF_Pixel8* pelR = lP->inPP8;
		A_long dlx = xL + lP->dx;
		A_long dly = yL + lP->dy;
		A_long drx = xL - lP->dx;
		A_long dry = yL - lP->dy;
		if (0 <= dlx && dlx < lP->width && 0 <= dly && dly < lP->height) {
			pelL = (PF_Pixel8*)((A_u_char*)lP->inPP8 + (dly * lP->rowbytes)) + dlx;
		}
		else {
			pelL->alpha = 0;
			pelL->blue = 0;
			pelL->green = 0;
			pelL->red = 0;
		}
		if (0 <= drx && drx < lP->width && 0 <= dry && dry < lP->height) {
			pelR = (PF_Pixel8*)((A_u_char*)lP->inPP8 + (dry * lP->rowbytes)) + drx;
		}
		else {
			pelR->alpha = 0;
			pelR->blue = 0;
			pelR->green = 0;
			pelR->red = 0;
		}

		outP->alpha = MAX(pel->alpha, MAX(pelR->alpha, pelL->alpha));
		switch (lP->type) {
		case 0:
			//RG
			outP->red = pelL->red;
			outP->green = pelR->green;
			outP->blue = pel->blue;
			break;
		case 1:
			//GB
			outP->red = pel->red;
			outP->green = pelL->green;
			outP->blue = pelR->blue;
			break;
		default:
			//BR
			outP->red = pelR->red;
			outP->green = pel->green;
			outP->blue = pelL->blue;
			break;
		}
	}

	return err;
}

static PF_Err 
Render (
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output )
{
	PF_Err				err		= PF_Err_NONE;
	AEGP_SuiteHandler	suites(in_data->pica_basicP);

	/*	Put interesting code here. */
	LayerInfo			lP;
	AEFX_CLR_STRUCT(lP);
	A_long				linesL	= 0;

	linesL 		= output->extent_hint.bottom - output->extent_hint.top;

	auto			angle = params[SKELETON_ANGLE]->u.ad.value / 65536.0;
	angle *= PI / 180;
	PF_FpLong			shift	= params[SKELETON_SHIFT]->u.fs_d.value;

	PF_LayerDef* input = &params[SKELETON_INPUT]->u.ld;
	lP.type = params[SKELETON_TYPE]->u.sd.value;
	lP.dx = shift * cos(angle) / in_data->downsample_x.den;
	lP.dy = shift * sin(angle) / in_data->downsample_y.den;
	lP.rowbytes = input->rowbytes;
	lP.width = input->width;
	lP.height = input->height;
	
	
	if (PF_WORLD_IS_DEEP(output)){
		PF_Pixel16* inPP;
		err = PF_GET_PIXEL_DATA16(input, NULL, &inPP);
		lP.inPP16 = inPP;
		ERR(suites.Iterate16Suite1()->iterate(	in_data,
												0,								// progress base
												linesL,							// progress final
												&params[SKELETON_INPUT]->u.ld,	// src 
												NULL,							// area - null for all pixels
												(void*)&lP,					// refcon - your custom data pointer
												MySimpleGainFunc16,				// pixel function pointer
												output));
	} else {
		PF_Pixel8* inPP;
		err = PF_GET_PIXEL_DATA8(input, NULL, &inPP);
		lP.inPP8 = inPP;
		ERR(suites.Iterate8Suite1()->iterate(	in_data,
												0,								// progress base
												linesL,							// progress final
												&params[SKELETON_INPUT]->u.ld,	// src 
												NULL,							// area - null for all pixels
												(void*)&lP,					// refcon - your custom data pointer
												MySimpleGainFunc8,				// pixel function pointer
												output));	
	}

	return err;
}


extern "C" DllExport
PF_Err PluginDataEntryFunction(
	PF_PluginDataPtr inPtr,
	PF_PluginDataCB inPluginDataCallBackPtr,
	SPBasicSuite* inSPBasicSuitePtr,
	const char* inHostName,
	const char* inHostVersion)
{
	PF_Err result = PF_Err_INVALID_CALLBACK;

	result = PF_REGISTER_EFFECT(
		inPtr,
		inPluginDataCallBackPtr,
		"tmcm_ChromAberration", // Name
		"TMCM Chromatic Aberration", // Match Name
		"tmcmPlugins", // Category
		AE_RESERVED_INFO); // Reserved Info

	return result;
}


PF_Err
EffectMain(
	PF_Cmd			cmd,
	PF_InData		*in_data,
	PF_OutData		*out_data,
	PF_ParamDef		*params[],
	PF_LayerDef		*output,
	void			*extra)
{
	PF_Err		err = PF_Err_NONE;
	
	try {
		switch (cmd) {
			case PF_Cmd_ABOUT:

				err = About(in_data,
							out_data,
							params,
							output);
				break;
				
			case PF_Cmd_GLOBAL_SETUP:

				err = GlobalSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_PARAMS_SETUP:

				err = ParamsSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_RENDER:

				err = Render(	in_data,
								out_data,
								params,
								output);
				break;
		}
	}
	catch(PF_Err &thrown_err){
		err = thrown_err;
	}
	return err;
}


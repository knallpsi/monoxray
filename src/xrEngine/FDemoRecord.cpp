#include "stdafx.h"
#include "igame_level.h"
#include "x_ray.h"

#include "gamefont.h"
#include "fDemoRecord.h"
#include "xr_ioconsole.h"
#include "xr_input.h"
#include "xr_object.h"
#include "render.h"
#include "CustomHUD.h"
#include "CameraManager.h"
#include "../xrGame/UICursor.h"
extern BOOL g_bDisableRedText;
static Flags32 s_hud_flag = {0};
static Flags32 s_dev_flags = {0};

BOOL stored_weapon;
BOOL stored_cross;
BOOL stored_red_text;

CDemoRecord* xrDemoRecord = 0;
CDemoRecord::force_position CDemoRecord::g_position = { false, {0, 0, 0} };
CDemoRecord::force_direction CDemoRecord::g_direction = {false, {0, 0, 0}};

Fbox curr_lm_fbox;

void setup_lm_screenshot_matrices()
{
	psHUD_Flags.assign(0);

	// build camera matrix
	Fbox bb = curr_lm_fbox;
	bb.getcenter(Device.vCameraPosition);

	Device.vCameraDirection.set(0.f, -1.f, 0.f);
	Device.vCameraTop.set(0.f, 0.f, 1.f);
	Device.vCameraRight.set(1.f, 0.f, 0.f);
	Device.mView.build_camera_dir(Device.vCameraPosition, Device.vCameraDirection, Device.vCameraTop);

	bb.xform(Device.mView);
	// build project matrix
	Device.mProject.build_projection_ortho(bb.max.x - bb.min.x,
	                                       bb.max.y - bb.min.y,
	                                       bb.min.z,
	                                       bb.max.z);
}

Fbox get_level_screenshot_bound()
{
	Fbox res = g_pGameLevel->ObjectSpace.GetBoundingVolume();
	if (g_pGameLevel->pLevel->section_exist("level_map"))
	{
		Fvector4 res2d = g_pGameLevel->pLevel->r_fvector4("level_map", "bound_rect");
		res.min.x = res2d.x;
		res.min.z = res2d.y;

		res.max.x = res2d.z;
		res.max.z = res2d.w;
	}

	return res;
}

void _InitializeFont(CGameFont*& F, LPCSTR section, u32 flags);

CDemoRecord::CDemoRecord(const char* name, float life_time, BOOL return_ctrl_inputs) : CEffectorCam(cefDemo, life_time/*,FALSE*/)
{
	stored_red_text = g_bDisableRedText;
	g_bDisableRedText = TRUE;
	m_iLMScreenshotFragment = -1;
	/*
	 stored_weapon = psHUD_Flags.test(HUD_WEAPON);
	 stored_cross = psHUD_Flags.test(HUD_CROSSHAIR);
	 psHUD_Flags.set(HUD_WEAPON, FALSE);
	 psHUD_Flags.set(HUD_CROSSHAIR, FALSE);
	 */
	m_b_redirect_input_to_level = false;
	_unlink(name);
	file = FS.w_open(name);
	isInputBlocked = FALSE;
	pDemoRecords = nullptr;
	if (file)
	{
		g_position.set_position = false;
		IR_Capture(); // capture input
		m_Camera.invert(Device.mView);
		m_fCameraBoundary = 100.f;

		// parse yaw
		Fvector& dir = m_Camera.k;
		Fvector DYaw;
		DYaw.set(dir.x, 0.f, dir.z);
		DYaw.normalize_safe();
		if (DYaw.x < 0) m_HPB.x = acosf(DYaw.z);
		else m_HPB.x = 2 * PI - acosf(DYaw.z);

		// parse pitch
		dir.normalize_safe();
		m_HPB.y = asinf(dir.y);
		m_HPB.z = 0;

		if (return_ctrl_inputs) {
			// Set the camera position behind the actor's position
			m_Actor_Position.set(m_Camera.c.x, m_Camera.c.y, m_Camera.c.z);

			// Move the camera a bit in front of the actor
			float distanceInFrontOfActor = 3.0f; // Set this to the desired distance
			Fvector cameraOffset = m_Camera.k;
			cameraOffset.mul(-distanceInFrontOfActor); // Note the negative sign
			m_Camera.c.add(m_Actor_Position, cameraOffset);

			// Turn the camera towards the actor
			// m_Camera.k.invert();
			m_Position.set(m_Camera.c);
		} else {
			// Set the camera position to the actor's position
			m_Position.set(m_Camera.c);
			m_Actor_Position.set(m_Camera.c.x, m_Camera.c.y, m_Camera.c.z);
		}
		m_fGroundPosition = m_Actor_Position.y - 1.0f;
		m_vVelocity.set(0, 0, 0);
		m_vAngularVelocity.set(0, 0, 0);
		iCount = 0;

		m_vT.set(0, 0, 0);
		m_vR.set(0, 0, 0);
		m_bMakeCubeMap = FALSE;
		m_bMakeScreenshot = FALSE;
		m_bMakeLevelMap = FALSE;
		return_ctrl_inputs = FALSE;
		m_CameraBoundaryEnabled = FALSE;
		m_fSpeed0 = pSettings->r_float("demo_record", "speed0");
		m_fSpeed1 = pSettings->r_float("demo_record", "speed1");
		m_fSpeed2 = pSettings->r_float("demo_record", "speed2");
		m_fSpeed3 = pSettings->r_float("demo_record", "speed3");
		m_fAngSpeed0 = pSettings->r_float("demo_record", "ang_speed0");
		m_fAngSpeed1 = pSettings->r_float("demo_record", "ang_speed1");
		m_fAngSpeed2 = pSettings->r_float("demo_record", "ang_speed2");
		m_fAngSpeed3 = pSettings->r_float("demo_record", "ang_speed3");
	} else
	{
		fLifeTime = -1;
	}
}

CDemoRecord::CDemoRecord(const char* name, xr_unordered_set<CDemoRecord*>* pDemoRecords, BOOL isInputBlocked, float life_time, BOOL return_ctrl_inputs) : CDemoRecord(name, life_time, return_ctrl_inputs)
{
	pDemoRecords->insert(this);
	this->pDemoRecords = pDemoRecords;
	this->isInputBlocked = isInputBlocked;
	this->return_ctrl_inputs = return_ctrl_inputs;
	if (!file) {
		StopDemo();
	}
	// when demo_record is launched, mouse controls are redirected to camera movements, so we hide the cursor
	GetUICursor().Hide();
}

void CDemoRecord::StopDemo() {
	fLifeTime = -1;
	if (pDemoRecords) {
		pDemoRecords->erase(this);
	}
}

void CDemoRecord::EnableReturnCtrlInputs() {
	return_ctrl_inputs = TRUE;
}
void CDemoRecord::SetCameraBoundary(float boundary) {
	m_CameraBoundaryEnabled = TRUE;
	m_fCameraBoundary = boundary;
}
CDemoRecord::~CDemoRecord()
{
	if (file)
	{
		IR_Release(); // release input
		FS.w_close(file);
	}
	if (pDemoRecords) {
		pDemoRecords->erase(this);
	}
	g_bDisableRedText = stored_red_text;

	Device.seqRender.Remove(this);
}

// +X, -X, +Y, -Y, +Z, -Z
static Fvector cmNorm[6] = {
	{0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 0.f, -1.f}, {0.f, 0.f, 1.f}, {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f}
};
static Fvector cmDir[6] = {
	{1.f, 0.f, 0.f}, {-1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, -1.f, 0.f}, {0.f, 0.f, 1.f}, {0.f, 0.f, -1.f}
};


void CDemoRecord::MakeScreenshotFace()
{
	switch (m_Stage)
	{
	case 0:
		s_hud_flag.assign(psHUD_Flags);
		psHUD_Flags.assign(0);
		break;
	case 1:
		Render->Screenshot();
		psHUD_Flags.assign(s_hud_flag);
		m_bMakeScreenshot = FALSE;
		break;
	}
	m_Stage++;
}


void GetLM_BBox(Fbox& bb, INT Step)
{
	float half_x = bb.min.x + (bb.max.x - bb.min.x) / 2;
	float half_z = bb.min.z + (bb.max.z - bb.min.z) / 2;
	switch (Step)
	{
	case 0:
		{
			bb.max.x = half_x;
			bb.min.z = half_z;
		}
		break;
	case 1:
		{
			bb.min.x = half_x;
			bb.min.z = half_z;
		}
		break;
	case 2:
		{
			bb.max.x = half_x;
			bb.max.z = half_z;
		}
		break;
	case 3:
		{
			bb.min.x = half_x;
			bb.max.z = half_z;
		}
		break;
	default:
		{
		}
		break;
	}
};

void CDemoRecord::MakeLevelMapProcess()
{
	switch (m_Stage)
	{
	case 0:
		{
			s_dev_flags = psDeviceFlags;
			s_hud_flag.assign(psHUD_Flags);
			psDeviceFlags.zero();
			psDeviceFlags.set(rsClearBB | rsDrawStatic, TRUE);
		}
		break;

	case DEVICE_RESET_PRECACHE_FRAME_COUNT + 30:
		{
			setup_lm_screenshot_matrices();

			string_path tmp;
			if (m_iLMScreenshotFragment == -1)
				xr_sprintf(tmp, sizeof(tmp), "map_%s", *g_pGameLevel->name());
			else
				xr_sprintf(tmp, sizeof(tmp), "map_%s#%d", *g_pGameLevel->name(), m_iLMScreenshotFragment);

			if (m_iLMScreenshotFragment != -1)
			{
				++m_iLMScreenshotFragment;

				if (m_iLMScreenshotFragment != 4)
				{
					curr_lm_fbox = get_level_screenshot_bound();
					GetLM_BBox(curr_lm_fbox, m_iLMScreenshotFragment);
					m_Stage -= 20;
				}
			}

			Render->Screenshot(IRender_interface::SM_FOR_LEVELMAP, tmp);

			if (m_iLMScreenshotFragment == -1 || m_iLMScreenshotFragment == 4)
			{
				psHUD_Flags.assign(s_hud_flag);

				BOOL bDevReset = !psDeviceFlags.equal(s_dev_flags, rsFullscreen);
				psDeviceFlags = s_dev_flags;
				m_bMakeLevelMap = FALSE;
				m_iLMScreenshotFragment = -1;
			}
		}
		break;
	default:
		{
			setup_lm_screenshot_matrices();
		}
		break;
	}
	m_Stage++;
}

void CDemoRecord::MakeCubeMapFace(Fvector& D, Fvector& N)
{
	string32 buf;
	switch (m_Stage)
	{
	case 0:
		N.set(cmNorm[m_Stage]);
		D.set(cmDir[m_Stage]);
		s_hud_flag.assign(psHUD_Flags);
		psHUD_Flags.assign(0);
		break;
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		N.set(cmNorm[m_Stage]);
		D.set(cmDir[m_Stage]);
		Render->Screenshot(IRender_interface::SM_FOR_CUBEMAP, itoa(m_Stage, buf, 10));
		break;
	case 6:
		Render->Screenshot(IRender_interface::SM_FOR_CUBEMAP, itoa(m_Stage, buf, 10));
		N.set(m_Camera.j);
		D.set(m_Camera.k);
		psHUD_Flags.assign(s_hud_flag);
		m_bMakeCubeMap = FALSE;
		break;
	}
	m_Stage++;
}

BOOL CDemoRecord::ProcessCam(SCamEffectorInfo& info)
{
	info.dont_apply = false;
	if (0 == file) return TRUE;

	m_Starting_Position.set(m_Position.x, m_Position.y, m_Position.z);

	if (m_bMakeScreenshot)
	{
		MakeScreenshotFace();
		// update camera
		info.n.set(m_Camera.j);
		info.d.set(m_Camera.k);
		info.p.set(m_Camera.c);
	}
	else if (m_bMakeLevelMap)
	{
		MakeLevelMapProcess();
		info.dont_apply = true;
	}
	else if (m_bMakeCubeMap)
	{
		MakeCubeMapFace(info.d, info.n);
		info.p.set(m_Camera.c);
		info.fAspect = 1.f;
	}
	else
	{
		if (IR_GetKeyState(DIK_F1))
		{
			pApp->pFontSystem->SetColor(color_rgba(255, 0, 0, 255));
			pApp->pFontSystem->SetAligment(CGameFont::alCenter);
			pApp->pFontSystem->OutSetI(0, -.05f);
			pApp->pFontSystem->OutNext("%s", "RECORDING");
			pApp->pFontSystem->OutNext("Key frames count: %d", iCount);
			pApp->pFontSystem->SetAligment(CGameFont::alLeft);
			pApp->pFontSystem->OutSetI(-0.2f, +.05f);
			pApp->pFontSystem->OutNext("SPACE");
			pApp->pFontSystem->OutNext("BACK");
			pApp->pFontSystem->OutNext("ESC");
			pApp->pFontSystem->OutNext("F11");
			pApp->pFontSystem->OutNext("LCONTROL+F11");
			pApp->pFontSystem->OutNext("F12");
			pApp->pFontSystem->SetAligment(CGameFont::alLeft);
			pApp->pFontSystem->OutSetI(0, +.05f);
			pApp->pFontSystem->OutNext("= Append Key");
			pApp->pFontSystem->OutNext("= Cube Map");
			pApp->pFontSystem->OutNext("= Quit");
			pApp->pFontSystem->OutNext("= Level Map ScreenShot");
			pApp->pFontSystem->OutNext("= Level Map ScreenShot(High Quality)");
			pApp->pFontSystem->OutNext("= ScreenShot");
		}

		m_vVelocity.lerp(m_vVelocity, m_vT, 0.3f);
		m_vAngularVelocity.lerp(m_vAngularVelocity, m_vR, 0.3f);

		float speed = m_fSpeed1, ang_speed = m_fAngSpeed1;

		if (IR_GetKeyState(DIK_LSHIFT))
		{
			speed = m_fSpeed0;
			ang_speed = m_fAngSpeed0;
		}
		else if (IR_GetKeyState(DIK_LALT))
		{
			speed = m_fSpeed2;
			ang_speed = m_fAngSpeed2;
		}
		else if (IR_GetKeyState(DIK_LCONTROL))
		{
			speed = m_fSpeed3;
			ang_speed = m_fAngSpeed3;
		}

		m_vT.mul(m_vVelocity, Device.fTimeDelta * speed);
		m_vR.mul(m_vAngularVelocity, Device.fTimeDelta * ang_speed);

		m_HPB.x -= m_vR.y;
		m_HPB.y -= m_vR.x;
		m_HPB.z += m_vR.z;
		if (g_position.set_position)
		{
			m_Position.set(g_position.p);
			g_position.set_position = false;
		}
		else
			g_position.p.set(m_Position);

		if (g_direction.set_direction)
		{
			m_HPB.set(g_direction.d);
			g_direction.set_direction = false;
		} else
			g_direction.d.set(m_Position);
		// move
		Fvector vmove;

		vmove.set(m_Camera.k);
		vmove.normalize_safe();
		vmove.mul(m_vT.z);
		m_Position.add(vmove);

		vmove.set(m_Camera.i);
		vmove.normalize_safe();
		vmove.mul(m_vT.x);
		m_Position.add(vmove);

		vmove.set(m_Camera.j);
		vmove.normalize_safe();
		vmove.mul(m_vT.y);
		m_Position.add(vmove);

		if (m_CameraBoundaryEnabled)
		{
			float x = m_Position.x;
			float y = m_Position.y;
			float z = m_Position.z;
			// let's check if we're out of bound
			if (_abs(m_Position.x - m_Actor_Position.x) > m_fCameraBoundary)
			{
				x = m_Starting_Position.x;
			}
			if (_abs(m_Position.y - m_Actor_Position.y) > m_fCameraBoundary)
			{
				y = m_Starting_Position.y;
			}
			if (_abs(m_Position.z - m_Actor_Position.z) > m_fCameraBoundary)
			{
				z = m_Starting_Position.z;
			}
			// fake ground collision check
			if (m_Position.y < m_fGroundPosition) {
				y = m_Starting_Position.y;
			}
			m_Position.set(x, y, z);
		}

		m_Camera.setHPB(m_HPB.x, m_HPB.y, m_HPB.z);
		m_Camera.translate_over(m_Position);
		// update camera
		info.n.set(m_Camera.j);
		info.d.set(m_Camera.k);
		info.p.set(m_Camera.c);

		fLifeTime -= Device.fTimeDelta;
		if (fLifeTime < 0) {
			StopDemo();
		}

		m_vT.set(0, 0, 0);
		m_vR.set(0, 0, 0);
	}
	return TRUE;
}

extern int mouse_button_2_key[];

void CDemoRecord::IR_OnMousePress(int btn)
{
	IR_OnKeyboardPress(mouse_button_2_key[btn]);
}

void CDemoRecord::IR_OnMouseRelease(int btn)
{
	IR_OnKeyboardRelease(mouse_button_2_key[btn]);
}

void CDemoRecord::IR_OnMouseWheel(int direction)
{
	if (m_b_redirect_input_to_level) g_pGameLevel->IR_OnMouseWheel(direction);
}

void CDemoRecord::IR_OnKeyboardPress(int dik)
{
	if (isInputBlocked) {
		if (dik == DIK_PAUSE)
			Device.Pause(!Device.Paused(), TRUE, TRUE, "demo_record");
		if (dik == DIK_GRAVE)
			Console->Show();
		if (dik == DIK_ESCAPE)
			Console->Execute("main_menu on");
	} else {
		// Added dik == DIK_RCONTROL as extra keybind to support keyboards with no numpad
		// Added dik == DIK_TAB as extra keybind that can also be used if return_ctrl_inputs is enabled. This key press event can be captured by the Demo Record invoker 
		if (dik == DIK_MULTIPLY || dik == DIK_RCONTROL || (dik == DIK_TAB && return_ctrl_inputs)) m_b_redirect_input_to_level = !m_b_redirect_input_to_level;

		if (m_b_redirect_input_to_level)
		{
			// control inputs are redirected to the invoker
			if (return_ctrl_inputs) {
				// if the invoker is a script that launched demo_record with return_ctrl_inputs enabled we show the cursor
				GetUICursor().Show();
			}
			g_pGameLevel->IR_OnKeyboardPress(dik);
			return;
		} else {
			if (GetUICursor().IsVisible()) {
				// mouse controls are back to the camera movements, so we hide the cursor
				GetUICursor().Hide();
			}
			// we end up here if controls have not been redirected to the invoker
			// if we launched demo_record_return_ctrl_inputs we return the event DIK_TAB key press also to the invoker 
			if (dik == DIK_TAB && return_ctrl_inputs) {
				// Sends upstream the DIK_TAB keyboard press event. This key toggles controls between demo_record and its launcher (game console or script)
				// upon relinquishing the controls (to gameconsole/scripts), if the DIK_TAB key is pressed (captured by DemoRecord regardless) we end up here 
				// but we like to let know the invoker that the DIK_TAB key was pressed and controls are back into DemoRecord hands
				g_pGameLevel->IR_OnKeyboardPress(dik);
				return;
			}
		}

		if (dik == DIK_GRAVE)
			Console->Show();
		if (dik == DIK_ESCAPE) {
			StopDemo();
			// if we launched demo_record_return_ctrl_inputs we return the event ESCAPE key press also to the launcher entity
			if (return_ctrl_inputs) {
				// we also show the cursor
				GetUICursor().Show();
				// sends upstream the DIK_ESCAPE keyboard press event. This key quit demo_record and returns controls to its launcher (game console or script)
				// This can help scripts that execute command demo_record_photomode to know when the demo_record has exited
				g_pGameLevel->IR_OnKeyboardPress(dik);
			}
		}
		//Alundaio: Teleport to demo cam
		//#ifndef MASTER_GOLD
		if (dik == DIK_RETURN)
		{
			if (strstr(Core.Params, "-dbg"))
			{
				if (g_pGameLevel->CurrentEntity())
				{
					g_pGameLevel->CurrentEntity()->ForceTransform(m_Camera);
					StopDemo();
				}
			}
		}
		//#endif // #ifndef MASTER_GOLD
		//-Alundaio

		if (dik == DIK_PAUSE)
			Device.Pause(!Device.Paused(), TRUE, TRUE, "demo_record");

		if (Device.imgui_shown()) return;
		if (dik == DIK_SPACE) RecordKey();
		if (dik == DIK_BACK) MakeCubemap();
		if (dik == DIK_F11) MakeLevelMapScreenshot(IR_GetKeyState(DIK_LCONTROL));
		if (dik == DIK_F12) MakeScreenshot();
	}
}

void CDemoRecord::IR_OnKeyboardRelease(int dik)
{
	if (isInputBlocked) return;
	if (m_b_redirect_input_to_level) {
		g_pGameLevel->IR_OnKeyboardRelease(dik);
	} else {
		if (dik == DIK_F12 && return_ctrl_inputs) {
			// if demo_record_return_ctrl_inputs is enabled we return the event F12 key release also to the launcher entity
			g_pGameLevel->IR_OnKeyboardRelease(dik);
		}
	}
}

static void update_whith_timescale(Fvector& v, const Fvector& v_delta)
{
	VERIFY(!fis_zero(Device.time_factor()));
	float scale = 1.f / Device.time_factor();
	v.mad(v, v_delta, scale);
}


void CDemoRecord::IR_OnKeyboardHold(int dik)
{
	if (isInputBlocked) return;
	if (m_b_redirect_input_to_level)
	{
		g_pGameLevel->IR_OnKeyboardHold(dik);
		return;
	}
	Fvector vT_delta = Fvector().set(0, 0, 0);
	Fvector vR_delta = Fvector().set(0, 0, 0);

	switch (dik)
	{
	case DIK_A:
	case DIK_NUMPAD1:
	case DIK_LEFT:
		vT_delta.x -= 1.0f;
		break; // Slide Left
	case DIK_D:
	case DIK_NUMPAD3:
	case DIK_RIGHT:
		vT_delta.x += 1.0f;
		break; // Slide Right
	case DIK_S:
		vT_delta.y -= 1.0f;
		break; // Slide Down
	case DIK_W:
		vT_delta.y += 1.0f;
		break; // Slide Up
		// rotate
	case DIK_NUMPAD2:
		vR_delta.x -= 1.0f;
		break; // Pitch Down
	case DIK_NUMPAD8:
		vR_delta.x += 1.0f;
		break; // Pitch Up
	case DIK_E:
	case DIK_NUMPAD6:
		vR_delta.y += 1.0f;
		break; // Turn Left
	case DIK_Q:
	case DIK_NUMPAD4:
		vR_delta.y -= 1.0f;
		break; // Turn Right
	case DIK_C: // Added DIK_C as extra keybind to support keyboards with no numpad
	case DIK_NUMPAD9:
		vR_delta.z -= 2.0f;
		break; // tilt Right
	case DIK_Z: // Added DIK_Z as extra keybind to support keyboards with no numpad
	case DIK_NUMPAD7:
		vR_delta.z += 2.0f;
		break; // tilt left
	}

	update_whith_timescale(m_vT, vT_delta);
	update_whith_timescale(m_vR, vR_delta);
}

void CDemoRecord::IR_OnMouseMove(int dx, int dy)
{
	if (isInputBlocked) return;
	if (m_b_redirect_input_to_level)
	{
		g_pGameLevel->IR_OnMouseMove(dx, dy);
		return;
	}

	Fvector vR_delta = Fvector().set(0, 0, 0);

	float scale = .5f; //psMouseSens;
	if (dx || dy)
	{
		vR_delta.y += float(dx) * scale; // heading
		vR_delta.x += ((psMouseInvert.test(1)) ? -1 : 1) * float(dy) * scale * (3.f / 4.f); // pitch
	}
	update_whith_timescale(m_vR, vR_delta);
}

void CDemoRecord::IR_OnMouseHold(int btn)
{
	if (isInputBlocked) return;
	if (m_b_redirect_input_to_level)
	{
		g_pGameLevel->IR_OnMouseHold(btn);
		return;
	}
	Fvector vT_delta = Fvector().set(0, 0, 0);
	switch (btn)
	{
	case 0:
		vT_delta.z += 1.0f;
		break; // Move Backward
	case 1:
		vT_delta.z -= 1.0f;
		break; // Move Forward
	}
	update_whith_timescale(m_vT, vT_delta);
}

void CDemoRecord::RecordKey()
{
	Fmatrix g_matView;

	g_matView.invert(m_Camera);
	file->w(&g_matView, sizeof(Fmatrix));
	iCount++;
}

void CDemoRecord::MakeCubemap()
{
	m_bMakeCubeMap = TRUE;
	m_Stage = 0;
}

void CDemoRecord::MakeScreenshot()
{
	m_bMakeScreenshot = TRUE;
	m_Stage = 0;
}

void CDemoRecord::MakeLevelMapScreenshot(BOOL bHQ)
{
	//Console->Execute("run_string level.set_weather(\"map\",true)");

	if (!bHQ)
		m_iLMScreenshotFragment = -1;
	else
		m_iLMScreenshotFragment = 0;

	curr_lm_fbox = get_level_screenshot_bound();
	GetLM_BBox(curr_lm_fbox, m_iLMScreenshotFragment);

	m_bMakeLevelMap = TRUE;
	m_Stage = 0;
}

void CDemoRecord::OnRender()
{
	pApp->pFontSystem->OnRender();
}

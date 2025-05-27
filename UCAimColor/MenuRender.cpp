#include "MenuRender.hpp"
#include "Config.hpp"
#include "MenuBackground.hpp"
#include "xor.hpp"
#include "MyFont.hpp"
#include "Custom.hpp"
#include "resource.h"
#include "icon.hpp"

#include <iostream>
#include <Windows.h>
#include <string>
#include <functional>
#include <vector>
#include <random>
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "d3dx11.lib")
#pragma comment(lib, "d3d11.lib")

using namespace Menu;
float Menu::DPI_SCALE = 1.f;

ImFont* Menu::Bold = nullptr;
ImFont* Menu::Medium = nullptr;
ImFont* Menu::Regular = nullptr;
ImFont* Menu::Light = nullptr;

int Menu::DefaultMenuWidht = 650;
int Menu::DefaultMenuHeight = 550;

ImColor Menu::MenuColor = ImColor(255, 255, 255);

float Menu::ChildPosRight;
float Menu::ChildPosBottom;
float Menu::ChildWidht;
float Menu::ChildHeight50;
float Menu::ChildHeight100;

ImVec2 LastMenuPos;

int SelectedTab = 0;

ImFont* font2;
ImFont* font3;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND window, UINT message, WPARAM wideParameter, LPARAM longParameter);

long __stdcall window_process(HWND window, UINT message, WPARAM wideParameter, LPARAM longParameter)
{
	static POINT ptMouseStart;
	static POINT ptMouseLast;
	static bool bMoving = false;
	static RECT rcTitleBar; // �rea superior da janela onde o arrasto � permitido

	if (ImGui_ImplWin32_WndProcHandler(window, message, wideParameter, longParameter))
		return true;

	switch (message)
	{
	case WM_SIZE:
		if (render_menu::device && wideParameter != SIZE_MINIMIZED)
		{
			render_menu::destory_render_target();
			render_menu::swap_chain->ResizeBuffers(0, (UINT)LOWORD(longParameter), (UINT)HIWORD(longParameter), DXGI_FORMAT_UNKNOWN, 0);
			render_menu::create_render_target();
		}
		return 0;

	case WM_SYSCOMMAND:
		if ((wideParameter & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_CREATE:
		// Calcule a �rea superior da janela onde o arrasto � permitido
		GetWindowRect(window, &rcTitleBar);
		rcTitleBar.bottom = rcTitleBar.top + 30; // Ajuste a altura da �rea superior aqui
		return 0;

	case WM_LBUTTONDOWN:
		// Inicie o movimento da janela apenas se o clique for na �rea superior
		ptMouseStart.x = LOWORD(longParameter);
		ptMouseStart.y = HIWORD(longParameter);
		if (PtInRect(&rcTitleBar, ptMouseStart))
		{
			ptMouseLast = ptMouseStart;
			bMoving = true;
			SetCapture(window);
		}
		return 0;

	case WM_MOUSEMOVE:
		if (bMoving)
		{

		}
		return 0;
	case WM_LBUTTONUP:
		// Pare o movimento da janela quando o bot�o esquerdo do mouse for liberado
		ReleaseCapture();
		bMoving = false;
		return 0;
	case WM_MOUSEWHEEL:
		// Impede o processamento da mensagem WM_MOUSEWHEEL para evitar que o conte�do da janela role
		return 0;
	case WM_NCHITTEST:
		// Defina a �rea onde o usu�rio pode arrastar a janela
		LRESULT hit = DefWindowProc(window, message, wideParameter, longParameter);
		if (hit == HTCLIENT)
		{
			POINTS ptMouse = MAKEPOINTS(longParameter);
			POINT ptMousePos;
			ptMousePos.x = ptMouse.x;
			ptMousePos.y = ptMouse.y;
			ScreenToClient(window, &ptMousePos);
			if (ptMousePos.y <= 30 && ptMousePos.x <= 550) // Ajuste a altura da �rea superior aqui
				return HTCAPTION; // Permite que o usu�rio arraste a janela clicando na �rea do cliente
		}
		return hit;

	}

	return ::DefWindowProc(window, message, wideParameter, longParameter);
}

void render_menu::create_window(LPCWSTR window_name)
{
	window_class.cbSize = sizeof(WNDCLASSEX);
	window_class.style = CS_CLASSDC;
	window_class.lpfnWndProc = (WNDPROC)window_process;
	window_class.cbClsExtra = 0L;
	window_class.cbWndExtra = 0L;
	window_class.hInstance = GetModuleHandleA(NULL);
	window_class.hIcon = LoadIcon(window_class.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	window_class.hCursor = NULL;
	window_class.hbrBackground = NULL;
	window_class.lpszMenuName = NULL;
	window_class.lpszClassName = xorstr_(L"class001");
	window_class.hIconSm = NULL;

	RegisterClassEx(&window_class);

	window = CreateWindow(window_class.lpszClassName, window_name,
		WS_POPUP /*| WS_MAXIMIZEBOX*/, 100, 100, width, height, NULL, NULL,
		window_class.hInstance, NULL);
	//float radius = 8.;
	//SetWindowLong(window, GWL_STYLE, GetWindowLong(window, GWL_STYLE) & ~WS_CAPTION);
	//HRGN hRgn = CreateRoundRectRgn(0, 0, width, height, radius, radius);
	//SetWindowRgn(window, hRgn, TRUE);
	if (!create_device())
	{
		destroy_device();
		::UnregisterClass(window_class.lpszClassName, window_class.hInstance);
		return;
	}

	::ShowWindow(window, SW_SHOWDEFAULT);
	::UpdateWindow(window);
}

void render_menu::destroy_window()
{
	::DestroyWindow(window);
	::UnregisterClass(window_class.lpszClassName, window_class.hInstance);
}

bool render_menu::create_device()
{
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = window;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &swap_chain, &device, &featureLevel, &device_context) != S_OK)
		return false;

	create_render_target();
	return true;
}

void render_menu::destroy_device()
{
	if (swap_chain) { swap_chain->Release(); swap_chain = nullptr; }
	if (device_context) { device_context->Release(); device_context = nullptr; }
	if (device) { device->Release(); device = nullptr; }
}

void render_menu::create_render_target()
{
	ID3D11Texture2D* pBackBuffer;
	swap_chain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	device->CreateRenderTargetView(pBackBuffer, NULL, &render_target_view);
	pBackBuffer->Release();
}

void render_menu::destory_render_target()
{
	if (render_target_view) { render_target_view->Release(); render_target_view = NULL; }
}

inline ID3D11ShaderResourceView* logo;

void render_menu::create_imgui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ::ImGui::GetIO();
	io.Fonts->Clear(); // clear fonts if you loaded some before (even if only default one was loaded)
	io.IniFilename = NULL;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	ImGuiStyle& style = ::ImGui::GetStyle();

	ImGui::StyleColorsDark();

	ImFontConfig font_config;
	font_config.PixelSnapH = false;
	font_config.FontDataOwnedByAtlas = false;
	font_config.OversampleH = 5;
	font_config.OversampleV = 5;
	font_config.RasterizerMultiply = 1.2f;

	static const ImWchar ranges[] = {

		0x0020, 0x00FF, // Basic Latin + Latin Supplement
		0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
		0x2DE0, 0x2DFF, // Cyrillic Extended-A
		0xA640, 0xA69F, // Cyrillic Extended-B
		0xE000, 0xE226, // icons
		0,
	};

	font_config.GlyphRanges = ranges;

	io.Fonts->AddFontFromFileTTF(xorstr_("C:\\Windows\\Fonts\\bahnschrift.ttf"), 15, &font_config, ranges);
	io.Fonts->AddFontFromMemoryTTF(icons_binary11, sizeof icons_binary11, 15, &font_config, ranges);
	font2 = io.Fonts->AddFontFromFileTTF(xorstr_("C:\\Windows\\Fonts\\calibrib.ttf"), 14, &font_config, ranges);
	font3 = io.Fonts->AddFontFromFileTTF(xorstr_("C:\\Windows\\Fonts\\bahnschrift.ttf"), 15, &font_config, ranges);

	static const ImWchar icon_ranges[]{ 0xf000, 0xf3ff, 0 };
	ImFontConfig icons_config;
	icons_config.MergeMode = true;
	icons_config.PixelSnapH = true;
	icons_config.OversampleH = 3;
	icons_config.OversampleV = 3;

	icons_font = io.Fonts->AddFontFromMemoryCompressedTTF(font_awesome_data, font_awesome_size, 19.5f, &icons_config, icon_ranges);
	Icons = io.Fonts->AddFontFromMemoryCompressedTTF(icon_compressed_data, icon_compressed_size, 25);

	Medium = io.Fonts->AddFontFromMemoryCompressedTTF(Medium_compressed_data, Medium_compressed_size, 20);
	Regular = io.Fonts->AddFontFromMemoryCompressedTTF(Regular_compressed_data, Regular_compressed_size, 20);
	SemiBold = io.Fonts->AddFontFromMemoryCompressedTTF(SemiBold_compressed_data, SemiBold_compressed_size, 25);
	Light = io.Fonts->AddFontFromMemoryCompressedTTF(SemiBold_compressed_data, Light_compressed_size, 11);
	Bold = io.Fonts->AddFontFromMemoryCompressedTTF(Bold_compressed_data, Bold_compressed_size, 30);

	D3DX11_IMAGE_LOAD_INFO info;
	ID3DX11ThreadPump* pump{ nullptr };
	D3DX11CreateShaderResourceViewFromMemory(device, logomain, sizeof(logomain), &info, pump, &logo, 0);

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(device, device_context);
}

void render_menu::destroy_imgui()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void render_menu::begin_render()
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);

		if (message.message == WM_QUIT)
		{
			is_running = false;
			return;
		}
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void render_menu::end_render()
{
	ImGui::Render();
	const float clear_color_with_alpha[4] = { 0.f, 0.f, 0.f, 1.f };
	device_context->OMSetRenderTargets(1, &render_target_view, NULL);
	device_context->ClearRenderTargetView(render_target_view, clear_color_with_alpha);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	swap_chain->Present(1, 0); // Present with vsync
}




static const char* aimkeys[]
{
	(""),
	("Left Mouse"),
	("Right Mouse"),
	("Cancel"),
	("Middle Mouse"),
	("Mouse 5"),
	("Mouse 4"),
	(""),
	("Backspace"),
	("Tab"),
	(""),
	(""),
	("Clear"),
	("Enter"),
	(""),
	(""),
	("Shift"),
	("Control"),
	("Alt"),
	("Pause"),
	("Caps"),
	(""),
	(""),
	(""),
	(""),
	(""),
	(""),
	("Escape"),
	(""),
	(""),
	(""),
	(""),
	("Space"),
	("Page Up"),
	("Page Down"),
	("End"),
	("Home"),
	("Left"),
	("Up"),
	("Right"),
	("Down"),
	(""),
	(""),
	(""),
	("Print"),
	("Insert"),
	("Delete"),
	(""),
	("0"),
	("1"),
	("2"),
	("3"),
	("4"),
	("5"),
	("6"),
	("7"),
	("8"),
	("9"),
	(""),
	(""),
	(""),
	(""),
	(""),
	(""),
	(""),
	("A"),
	("B"),
	("C"),
	("D"),
	("E"),
	("F"),
	("G"),
	("H"),
	("I"),
	("J"),
	("K"),
	("L"),
	("M"),
	("N"),
	("O"),
	("P"),
	("Q"),
	("R"),
	("S"),
	("T"),
	("U"),
	("V"),
	("W"),
	("X"),
	("Y"),
	("Z"),
	(""),
	(""),
	(""),
	(""),
	(""),
	("Numpad 0"),
	("Numpad 1"),
	("Numpad 2"),
	("Numpad 3"),
	("Numpad 4"),
	("Numpad 5"),
	("Numpad 6"),
	("Numpad 7"),
	("Numpad 8"),
	("Numpad 9"),
	("Multiply"),
	("Add"),
	(""),
	("Subtract"),
	("Decimal"),
	("Divide"),
	("F1"),
	("F2"),
	("F3"),
	("F4"),
	("F5"),
	("F6"),
	("F7"),
	("F8"),
	("F9"),
	("F10"),
	("F11"),
	("F12"),

};
static int keystatus = 0;
static int keystatusR = 0;
static int keystatusA = 0;
static int keystatusS = 0;
static int keystatusF = 0;
static int keystatusK = 0;

void ChangeKeyK(void* blank)
{
	keystatusK = 1;
	while (true)
	{
		for (int i = 0; i < 0x87; i++)
		{
			if (GetKeyState(i) & 0x8000)
			{
				cfg::triggerbot_key = i;
				keystatusK = 0;
				return;
			}
		}
	}
}

void ChangeKeyF(void* blank)
{
	keystatusF = 1;
	while (true)
	{
		for (int i = 0; i < 0x87; i++)
		{
			if (GetKeyState(i) & 0x8000)
			{
				cfg::flicker_key = i;
				keystatusF = 0;
				return;
			}
		}
	}
}
void ChangeKeyS(void* blank)
{
	keystatusS = 1;
	while (true)
	{
		for (int i = 0; i < 0x87; i++)
		{
			if (GetKeyState(i) & 0x8000)
			{
				cfg::silent_key = i;
				keystatusS = 0;
				return;
			}
		}
	}
}
void ChangeKeyA(void* blank)
{
	keystatusA = 1;
	while (true)
	{
		for (int i = 0; i < 0x87; i++)
		{
			if (GetKeyState(i) & 0x8000)
			{
				cfg::assist_aimkey = i;
				keystatusA = 0;
				return;
			}
		}
	}
}
void ChangeKeyR(void* blank)
{
	keystatusR = 1;
	while (true)
	{
		for (int i = 0; i < 0x87; i++)
		{
			if (GetKeyState(i) & 0x8000)
			{
				cfg::recoil_key = i;
				keystatusR = 0;
				return;
			}
		}
	}
}
void ChangeKey(void* blank)
{
	keystatus = 1;
	while (true)
	{
		for (int i = 0; i < 0x87; i++)
		{
			if (GetKeyState(i) & 0x8000)
			{
				cfg::aimkey = i;
				keystatus = 0;
				return;
			}
		}
	}
}

static bool Items_ArrayGetter(void* data, int idx, const char** out_text)
{
	const char* const* items = (const char* const*)data;
	if (out_text)
		*out_text = items[idx];
	return true;
}

void HotkeyButtons(int aimkey, void* changekey, int status)
{
	const char* preview_value = NULL;
	if (aimkey >= 0 && aimkey < IM_ARRAYSIZE(aimkeys))
		Items_ArrayGetter(aimkeys, aimkey, &preview_value);

	std::string aimkeys;
	if (preview_value == NULL)
		aimkeys = xorstr_("SELECIONAR TECLA");
	else
		aimkeys = preview_value;

	if (status == 1)
	{
		aimkeys = xorstr_("PERSSIONE A TECLA");
	}
	if (ImGui::Button(aimkeys.c_str(), ImVec2(190, 35)))
	{
		if (status == 0)
		{
			CreateThread(0, 0, (LPTHREAD_START_ROUTINE)changekey, nullptr, 0, nullptr);
		}
	}
	if (ImGui::IsItemHovered() && dialog == true)
	{
		ImGui::SetTooltip(xorstr_("Seta o botao de ativacao do programa."));
	}
}



namespace setting 
{
	static int settsMode = 0;
}
static const char* settsName[] =
{
	"1",
	"2",
	"3",
};
static BOOL WritePrivateProfileInt(LPCSTR lpAppName, LPCSTR lpKeyName, int nInteger, LPCSTR lpFileName) {
	char lpString[1024];
	sprintf_s(lpString, xorstr_("%d"), nInteger);
	return WritePrivateProfileStringA(lpAppName, lpKeyName, lpString, lpFileName);
}
static BOOL WritePrivateProfileFloat(LPCSTR lpAppName, LPCSTR lpKeyName, float nInteger, LPCSTR lpFileName) {
	char lpString[1024];
	sprintf_s(lpString, xorstr_("%f"), nInteger);
	return WritePrivateProfileStringA(lpAppName, lpKeyName, lpString, lpFileName);
}
static float GetPrivateProfileFloat(LPCSTR lpAppName, LPCSTR lpKeyName, FLOAT flDefault, LPCSTR lpFileName)
{
	char szData[32];

	GetPrivateProfileStringA(lpAppName, lpKeyName, std::to_string(flDefault).c_str(), szData, 32, lpFileName);

	return (float)atof(szData);
}
static void Save_Settings(LPCSTR path)
{
	WritePrivateProfileInt((xorstr_("Aimbot")), (xorstr_("ACTIVATE")), cfg::aimbot_ativo, path);
	WritePrivateProfileInt((xorstr_("Aimbot")), (xorstr_("FOV")), cfg::aimbot_fov, path);
	WritePrivateProfileFloat((xorstr_("Aimbot")), (xorstr_("SMOOTH")), cfg::aimbot_smooth, path);
	WritePrivateProfileInt((xorstr_("Aimbot")), (xorstr_("X_OFFSET")), cfg::head_offset_x, path);
	WritePrivateProfileInt((xorstr_("Aimbot")), (xorstr_("Y_OFFSET")), cfg::head_offset_y, path);
	WritePrivateProfileInt((xorstr_("Aimbot")), (xorstr_("SPEED")), cfg::speed, path);
	WritePrivateProfileInt((xorstr_("Aimbot")), (xorstr_("SLEEP")), cfg::sleep, path);
	WritePrivateProfileInt((xorstr_("Aimbot")), (xorstr_("KEY")), cfg::aimkey, path);

	WritePrivateProfileInt((xorstr_("AimAssist")), (xorstr_("ACTIVATE")), cfg::aimassist_ativo, path);
	WritePrivateProfileInt((xorstr_("AimAssist")), (xorstr_("FOV")), cfg::aimassist_fov, path);
	WritePrivateProfileFloat((xorstr_("AimAssist")), (xorstr_("SMOOTH")), cfg::aimassist_smooth, path);
	WritePrivateProfileInt((xorstr_("AimAssist")), (xorstr_("X_OFFSET")), cfg::assist_head_offset_x, path);
	WritePrivateProfileInt((xorstr_("AimAssist")), (xorstr_("Y_OFFSET")), cfg::assist_head_offset_y, path);
	WritePrivateProfileInt((xorstr_("AimAssist")), (xorstr_("SPEED")), cfg::assist_speed, path);
	WritePrivateProfileInt((xorstr_("AimAssist")), (xorstr_("SLEEP")), cfg::assist_sleep, path);
	WritePrivateProfileInt((xorstr_("AimAssist")), (xorstr_("KEY")), cfg::assist_aimkey, path);

	WritePrivateProfileInt((xorstr_("Silent")), (xorstr_("ACTIVATE_SILENT")), cfg::silent_ativo, path);
	WritePrivateProfileInt((xorstr_("Silent")), (xorstr_("FOV_SILENT")), cfg::silent_fov, path);
	WritePrivateProfileInt((xorstr_("Silent")), (xorstr_("X_OFFSET_SILENT")), cfg::silent_head_offset_x, path);
	WritePrivateProfileInt((xorstr_("Silent")), (xorstr_("Y_OFFSET_SILENT")), cfg::silent_head_offset_y, path);
	WritePrivateProfileInt((xorstr_("Silent")), (xorstr_("SLEEP_SILENT")), cfg::silent_delay_between_shots, path);
	WritePrivateProfileFloat((xorstr_("Silent")), (xorstr_("DISTANCE_SILENT")), cfg::distance, path);
	WritePrivateProfileInt((xorstr_("Silent")), (xorstr_("KEY_SILENT")), cfg::silent_key, path);

	WritePrivateProfileInt((xorstr_("Flicker")), (xorstr_("ACTIVATE_FLICKER")), cfg::flicker_ativo, path);
	WritePrivateProfileInt((xorstr_("Flicker")), (xorstr_("FOV_FLICKER")), cfg::flicker_fov, path);
	WritePrivateProfileInt((xorstr_("Flicker")), (xorstr_("SLEEP_FLICKER")), cfg::flicker_delay_between_shots, path);
	WritePrivateProfileFloat((xorstr_("Flicker")), (xorstr_("DISTANCE_FLICKER")), cfg::flicker_distance, path);
	WritePrivateProfileInt((xorstr_("Flicker")), (xorstr_("KEY_FLICKER")), cfg::flicker_key, path);

	WritePrivateProfileInt((xorstr_("Trigger")), (xorstr_("ACTIVATE_TRIGGER")), cfg::triggerbot_ativo, path);
	WritePrivateProfileInt((xorstr_("Trigger")), (xorstr_("FOVX_TRIGGER")), cfg::triggerbot_fovX, path);
	WritePrivateProfileInt((xorstr_("Trigger")), (xorstr_("FOVY_TRIGGER")), cfg::triggerbot_fovY, path);
	WritePrivateProfileInt((xorstr_("Trigger")), (xorstr_("SLEEP_TRIGGER")), cfg::triggerbot_delay, path);
	WritePrivateProfileInt((xorstr_("Trigger")), (xorstr_("KEY_TRIGGER")), cfg::triggerbot_key, path);

	WritePrivateProfileInt((xorstr_("RCS")), (xorstr_("ACTIVATE_RCS")), cfg::recoil_ativo, path);
	WritePrivateProfileInt((xorstr_("RCS")), (xorstr_("STEPS_RCS")), cfg::recoil_length, path);
	WritePrivateProfileFloat((xorstr_("RCS")), (xorstr_("SPEED_RCS")), cfg::recoil_speed, path);
	WritePrivateProfileInt((xorstr_("RCS")), (xorstr_("DELAY_RCS")), cfg::time_to_start, path);
	WritePrivateProfileInt((xorstr_("RCS")), (xorstr_("SLEEP_RCS")), cfg::recoil_sleep, path);
	WritePrivateProfileInt((xorstr_("RCS")), (xorstr_("KEY_RCS")), cfg::recoil_key, path);


	WritePrivateProfileInt((xorstr_("Color")), (xorstr_("MODE")), cfg::color_mode, path);
	WritePrivateProfileInt((xorstr_("Color")), (xorstr_("R_MENOR")), cfg::menorRGB[0], path);
	WritePrivateProfileInt((xorstr_("Color")), (xorstr_("G_MENOR")), cfg::menorRGB[1], path);
	WritePrivateProfileInt((xorstr_("Color")), (xorstr_("B_MENOR")), cfg::menorRGB[2], path);

	WritePrivateProfileInt((xorstr_("Color")), (xorstr_("R_MAIOR")), cfg::maiorRGB[0], path);
	WritePrivateProfileInt((xorstr_("Color")), (xorstr_("G_MAIOR")), cfg::maiorRGB[1], path);
	WritePrivateProfileInt((xorstr_("Color")), (xorstr_("B_MAIOR")), cfg::maiorRGB[2], path);

	WritePrivateProfileInt((xorstr_("Color")), (xorstr_("H_MENOR")), cfg::menorHSV[0], path);
	WritePrivateProfileInt((xorstr_("Color")), (xorstr_("S_MENOR")), cfg::menorHSV[1], path);
	WritePrivateProfileInt((xorstr_("Color")), (xorstr_("V_MENOR")), cfg::menorHSV[2], path);

	WritePrivateProfileInt((xorstr_("Color")), (xorstr_("H_MAIOR")), cfg::maiorHSV[0], path);
	WritePrivateProfileInt((xorstr_("Color")), (xorstr_("S_MAIOR")), cfg::maiorHSV[1], path);
	WritePrivateProfileInt((xorstr_("Color")), (xorstr_("V_MAIOR")), cfg::maiorHSV[2], path);

	WritePrivateProfileInt((xorstr_("Dialog")), (xorstr_("DIALOG_BOOL")), dialog, path);

}
static void Load_Settings(LPCSTR path)
{
	cfg::aimbot_ativo = GetPrivateProfileIntA((xorstr_("Aimbot")), (xorstr_("ACTIVATE")), cfg::aimbot_ativo, path);
	cfg::aimbot_fov = GetPrivateProfileIntA((xorstr_("Aimbot")), (xorstr_("FOV")), cfg::aimbot_fov, path);
	cfg::aimbot_smooth = GetPrivateProfileFloat((xorstr_("Aimbot")), (xorstr_("SMOOTH")), cfg::aimbot_smooth, path);
	cfg::head_offset_x = GetPrivateProfileIntA((xorstr_("Aimbot")), (xorstr_("X_OFFSET")), cfg::head_offset_x, path);
	cfg::head_offset_y = GetPrivateProfileIntA((xorstr_("Aimbot")), (xorstr_("Y_OFFSET")), cfg::head_offset_y, path);
	cfg::speed = GetPrivateProfileIntA((xorstr_("Aimbot")), (xorstr_("SPEED")), cfg::speed, path);
	cfg::sleep = GetPrivateProfileIntA((xorstr_("Aimbot")), (xorstr_("SLEEP")), cfg::sleep, path);
	cfg::aimkey = GetPrivateProfileIntA((xorstr_("Aimbot")), (xorstr_("KEY")), cfg::aimkey, path);

	cfg::aimassist_ativo = GetPrivateProfileIntA((xorstr_("AimAssist")), (xorstr_("ACTIVATE")), cfg::aimassist_ativo, path);
	cfg::aimassist_fov = GetPrivateProfileIntA((xorstr_("AimAssist")), (xorstr_("FOV")), cfg::aimassist_fov, path);
	cfg::aimassist_smooth = GetPrivateProfileFloat((xorstr_("AimAssist")), (xorstr_("SMOOTH")), cfg::aimassist_smooth, path);
	cfg::assist_head_offset_x = GetPrivateProfileIntA((xorstr_("AimAssist")), (xorstr_("X_OFFSET")), cfg::assist_head_offset_x, path);
	cfg::assist_head_offset_y = GetPrivateProfileIntA((xorstr_("AimAssist")), (xorstr_("Y_OFFSET")), cfg::assist_head_offset_y, path);
	cfg::assist_speed = GetPrivateProfileIntA((xorstr_("AimAssist")), (xorstr_("SPEED")), cfg::assist_speed, path);
	cfg::assist_sleep = GetPrivateProfileIntA((xorstr_("AimAssist")), (xorstr_("SLEEP")), cfg::assist_sleep, path);
	cfg::assist_aimkey = GetPrivateProfileIntA((xorstr_("AimAssist")), (xorstr_("KEY")), cfg::assist_aimkey, path);

	cfg::silent_ativo = GetPrivateProfileIntA((xorstr_("Silent")), (xorstr_("ACTIVATE_SILENT")), cfg::silent_ativo, path);
	cfg::silent_fov = GetPrivateProfileIntA((xorstr_("Silent")), (xorstr_("FOV_SILENT")), cfg::silent_fov, path);
	cfg::silent_head_offset_x = GetPrivateProfileIntA((xorstr_("Silent")), (xorstr_("X_OFFSET_SILENT")), cfg::silent_head_offset_x, path);
	cfg::silent_head_offset_y = GetPrivateProfileIntA((xorstr_("Silent")), (xorstr_("Y_OFFSET_SILENT")), cfg::silent_head_offset_y, path);
	cfg::silent_delay_between_shots = GetPrivateProfileIntA((xorstr_("Silent")), (xorstr_("SLEEP_SILENT")), cfg::silent_delay_between_shots, path);
	cfg::distance = GetPrivateProfileFloat((xorstr_("Silent")), (xorstr_("DISTANCE_SILENT")), cfg::distance, path);
	cfg::silent_key = GetPrivateProfileIntA((xorstr_("Silent")), (xorstr_("KEY_SILENT")), cfg::silent_key, path);

	cfg::flicker_ativo = GetPrivateProfileIntA((xorstr_("Flicker")), (xorstr_("ACTIVATE_FLICKER")), cfg::flicker_ativo, path);
	cfg::flicker_fov = GetPrivateProfileIntA((xorstr_("Flicker")), (xorstr_("FOV_FLICKER")), cfg::flicker_fov, path);
	cfg::flicker_delay_between_shots = GetPrivateProfileIntA((xorstr_("Flicker")), (xorstr_("SLEEP_FLICKER")), cfg::flicker_delay_between_shots, path);
	cfg::flicker_distance = GetPrivateProfileFloat((xorstr_("Flicker")), (xorstr_("DISTANCE_FLICKER")), cfg::flicker_distance, path);
	cfg::flicker_key = GetPrivateProfileIntA((xorstr_("Flicker")), (xorstr_("KEY_FLICKER")), cfg::flicker_key, path);

	cfg::triggerbot_ativo = GetPrivateProfileIntA((xorstr_("Trigger")), (xorstr_("ACTIVATE_TRIGGER")), cfg::triggerbot_ativo, path);
	cfg::triggerbot_fovX = GetPrivateProfileIntA((xorstr_("Trigger")), (xorstr_("FOVX_TRIGGER")), cfg::triggerbot_fovX, path);
	cfg::triggerbot_fovY = GetPrivateProfileIntA((xorstr_("Trigger")), (xorstr_("FOVY_TRIGGER")), cfg::triggerbot_fovY, path);
	cfg::triggerbot_delay = GetPrivateProfileIntA((xorstr_("Trigger")), (xorstr_("SLEEP_TRIGGER")), cfg::triggerbot_delay, path);
	cfg::triggerbot_key = GetPrivateProfileIntA((xorstr_("Trigger")), (xorstr_("KEY_TRIGGER")), cfg::triggerbot_key, path);

	cfg::recoil_ativo = GetPrivateProfileIntA((xorstr_("RCS")), (xorstr_("ACTIVATE_RCS")), cfg::recoil_ativo, path);
	cfg::recoil_length = GetPrivateProfileIntA((xorstr_("RCS")), (xorstr_("STEPS_RCS")), cfg::recoil_length, path);
	cfg::recoil_speed = GetPrivateProfileFloat((xorstr_("RCS")), (xorstr_("SPEED_RCS")), cfg::recoil_speed, path);
	cfg::time_to_start = GetPrivateProfileIntA((xorstr_("RCS")), (xorstr_("DELAY_RCS")), cfg::time_to_start, path);
	cfg::recoil_sleep = GetPrivateProfileIntA((xorstr_("RCS")), (xorstr_("SLEEP_RCS")), cfg::recoil_sleep, path);
	cfg::recoil_key = GetPrivateProfileIntA((xorstr_("RCS")), (xorstr_("KEY_RCS")), cfg::recoil_key, path);

	cfg::color_mode = GetPrivateProfileIntA((xorstr_("Color")), (xorstr_("MODE")), cfg::color_mode, path);
	cfg::menorRGB[0] = GetPrivateProfileIntA((xorstr_("Color")), (xorstr_("R_MENOR")), cfg::menorRGB[0], path);
	cfg::menorRGB[1] = GetPrivateProfileIntA((xorstr_("Color")), (xorstr_("G_MENOR")), cfg::menorRGB[1], path);
	cfg::menorRGB[2] = GetPrivateProfileIntA((xorstr_("Color")), (xorstr_("B_MENOR")), cfg::menorRGB[2], path);

	cfg::maiorRGB[0] = GetPrivateProfileIntA((xorstr_("Color")), (xorstr_("R_MAIOR")), cfg::maiorRGB[0], path);
	cfg::maiorRGB[1] = GetPrivateProfileIntA((xorstr_("Color")), (xorstr_("G_MAIOR")), cfg::maiorRGB[1], path);
	cfg::maiorRGB[2] = GetPrivateProfileIntA((xorstr_("Color")), (xorstr_("B_MAIOR")), cfg::maiorRGB[2], path);

	cfg::menorHSV[0] = GetPrivateProfileIntA((xorstr_("Color")), (xorstr_("H_MENOR")), cfg::menorHSV[0], path);
	cfg::menorHSV[1] = GetPrivateProfileIntA((xorstr_("Color")), (xorstr_("S_MENOR")), cfg::menorHSV[1], path);
	cfg::menorHSV[2] = GetPrivateProfileIntA((xorstr_("Color")), (xorstr_("V_MENOR")), cfg::menorHSV[2], path);

	cfg::maiorHSV[0] = GetPrivateProfileIntA((xorstr_("Color")), (xorstr_("H_MAIOR")), cfg::maiorHSV[0], path);
	cfg::maiorHSV[1] = GetPrivateProfileIntA((xorstr_("Color")), (xorstr_("S_MAIOR")), cfg::maiorHSV[1], path);
	cfg::maiorHSV[2] = GetPrivateProfileIntA((xorstr_("Color")), (xorstr_("V_MAIOR")), cfg::maiorHSV[2], path);

	dialog = GetPrivateProfileIntA((xorstr_("Dialog")), (xorstr_("DIALOG_BOOL")), dialog, path);
};
bool FilterInputChars(ImGuiInputTextCallbackData* data)
{
	if (data->EventChar < '0' || data->EventChar > '9')
		return true;
	return false;
}

struct Config {
	std::string path;
	std::string name;
};

static int selectedConfigIndex = -1;
std::vector<Config> configs;
char newConfigName[128] = "";
char Key[128] = "";
bool DeleteFileW(const char* filePath) {
	return std::remove(filePath) == 0;
}
bool CreateEmptyFile(const char* filePath) {
	std::ofstream file(filePath);
	return file.good();
}
bool LoadConfigs() {
	configs.clear(); // Limpa as configura��es existentes

	for (const auto& entry : filesystem::directory_iterator(xorstr_("C:\\"))) {
		if (entry.is_regular_file() && entry.path().extension() == xorstr_(".ini")) {
			Config config;
			config.path = entry.path().string();
			config.name = entry.path().stem().string();
			configs.push_back(config);
		}
	}

	return true; // Retorna true se o carregamento for bem-sucedido
}

float FastFloatLerp__(std::string identifier, bool state, float min, float max, float speed)
{

	static std::unordered_map<std::string, float> valuesMapFloat;
	auto value = valuesMapFloat.find(identifier);

	if (value == valuesMapFloat.end()) {
		valuesMapFloat.insert({ identifier, min });
		value = valuesMapFloat.find(identifier);
	}

	const float frameRateSpeed = speed * (1.f - ImGui::GetIO().DeltaTime);

	if (state) {
		if (value->second < max)
			value->second += frameRateSpeed;
	}
	else {
		if (value->second > min)
			value->second -= frameRateSpeed;
	}

	value->second = std::clamp(value->second, min, max);

	return value->second;
}

void Menu::DrawLegitTab()
{
	ImGui::SetCursorPos(ImVec2(95 * DPI_SCALE, 70 * DPI_SCALE));
	ImGui::BeginFeaturesChild("Aimbot", ImVec2(ChildWidht - 27, ChildHeight50));
	{
		ImGui::Checkbox(xorstr_("Activate Aimbot"), &cfg::aimbot_ativo);
		ImGui::SliderInt(xorstr_("Fov Aimbot"), &cfg::aimbot_fov, 1, 200);
		ImGui::SliderFloat(xorstr_("Smooth"), &cfg::aimbot_smooth, 1, 10, "%.2f");
		ImGui::SliderInt(xorstr_("Speed"), &cfg::speed, 1, 10);
		ImGui::SliderInt(xorstr_("Sleep"), &cfg::sleep, 0, 100);
		ImGui::KeyBind("Aim Key", &cfg::aimkey);


	}
	ImGui::EndFeaturesChild();

	ImGui::SetCursorPos(ImVec2(370 * DPI_SCALE, 70 * DPI_SCALE));
	ImGui::BeginFeaturesChild("Magnetic", ImVec2(ChildWidht - 27, ChildHeight50));
	{
		ImGui::Checkbox(xorstr_("Activate Aim Assist"), &cfg::aimassist_ativo);
		ImGui::SliderInt(xorstr_("Aim Assist Fov"), &cfg::aimassist_fov, 1, 200);
		ImGui::SliderFloat(xorstr_("Aim Assist Smooth"), &cfg::aimassist_smooth, 1, 10, "%.2f");
		ImGui::SliderInt(xorstr_("Aim Assist Speed"), &cfg::assist_speed, 1, 10);
		ImGui::SliderInt(xorstr_("Aim Assist Sleep"), &cfg::assist_sleep, 0, 100);
		ImGui::KeyBind("Panic Key", &cfg::assist_aimkey);
	}
	ImGui::EndFeaturesChild();

	ImGui::SetCursorPos(ImVec2(370 * DPI_SCALE, (ChildHeight50 + 80) * DPI_SCALE));
	ImGui::BeginFeaturesChild("Offsets", ImVec2(ChildWidht - 25, ChildHeight50 - 37));
	{
		ImGui::SliderInt(xorstr_("Head Offset X"), &cfg::head_offset_x, 0, 100);
		ImGui::SliderInt(xorstr_("Head Offset Y"), &cfg::head_offset_y, 0, 100);
	}
	ImGui::EndFeaturesChild();


	ImGui::SetCursorPos(ImVec2(95 * DPI_SCALE, (ChildHeight50 + 80) * DPI_SCALE));
	ImGui::BeginFeaturesChild("Triggerbot", ImVec2(ChildWidht - 25, ChildHeight50 - 37));
	{
		ImGui::Checkbox(xorstr_("Activate Triggerbot"), &cfg::triggerbot_ativo);
		ImGui::SliderInt(xorstr_("Trigger Fov X"), &cfg::triggerbot_fovX, 1, 100);
		ImGui::SliderInt(xorstr_("Trigger Fov Y"), &cfg::triggerbot_fovY, 1, 100);
		ImGui::SliderInt(xorstr_("Trigger Delay"), &cfg::triggerbot_delay, 1, 500);
		ImGui::KeyBind("Trigger Key", &cfg::triggerbot_key);
	}
	ImGui::EndFeaturesChild();

	/*ImGui::SetCursorPos(ImVec2(ChildPosRight, 20 * DPI_SCALE));
	ImGui::BeginFeaturesChild("Trigger", ImVec2(ChildWidht, ChildHeight100));
	{
		static int sliderint = 50;
		ImGui::SliderInt("SLider", &sliderint, 0, 100);
		static bool teste2 = false;
		ImGui::Checkbox("Teste2", &teste2);
		ImGui::ColorEdit4("Color Picker", (float*)&MenuColor.Value);
		static int selectedkey = 0;
		ImGui::KeyBind("Piroca", &selectedkey);
		static char pirocagrande[64] = "dawfawf";
		ImGui::InputText("Labelilua", pirocagrande, sizeof(pirocagrande));
		static bool teste = false;
		ImGui::Checkbox("Testedwa", &teste);
	}
	ImGui::EndFeaturesChild();*/
}

void Menu::DrawFlicker()
{
	ImGui::SetCursorPos(ImVec2(95 * DPI_SCALE, 70 * DPI_SCALE));
	ImGui::BeginFeaturesChild("Flicker", ImVec2(ChildWidht - 27, ChildHeight50 - 33));
	{

		ImGui::Checkbox(xorstr_("Activate Flicker"), &cfg::flicker_ativo);
		ImGui::SliderInt(xorstr_("Flicker Fov"), &cfg::flicker_fov, 1, 300);
		ImGui::SliderInt(xorstr_("Flicker Speed"), &cfg::flicker_delay_between_shots, 1, 500);
		ImGui::SliderFloat(xorstr_("Flicker Distance"), &cfg::flicker_distance, 0.f, 10);
		ImGui::KeyBind("Flicker Key", &cfg::flicker_key);
	}
	ImGui::EndFeaturesChild();

	ImGui::SetCursorPos(ImVec2(370 * DPI_SCALE, 70 * DPI_SCALE));
	ImGui::BeginFeaturesChild("Silent Aim", ImVec2(ChildWidht - 27, ChildHeight50));
	{
		ImGui::Checkbox(xorstr_("Activate Silent"), &cfg::silent_ativo);
		ImGui::SliderInt(xorstr_("Silent Fov"), &cfg::silent_fov, 1, 300);
		ImGui::SliderInt(xorstr_("Shoot Sleep"), &cfg::silent_delay_between_shots, 1, 1000);
		ImGui::SliderFloat(xorstr_("Distance"), &cfg::distance, 0.f, 10);
		ImGui::KeyBind("Silent Key", &cfg::silent_key);
	}
	ImGui::EndFeaturesChild();

	ImGui::SetCursorPos(ImVec2(370 * DPI_SCALE, (ChildHeight50 + 80) * DPI_SCALE));
	ImGui::BeginFeaturesChild("Offsets Silent", ImVec2(ChildWidht - 25, ChildHeight50 - 37));
	{
		ImGui::SliderInt(xorstr_("Silent Offset Head X"), &cfg::silent_head_offset_x, 0, 100);
		ImGui::SliderInt(xorstr_("Silent Offset Head Y"), &cfg::silent_head_offset_y, 0, 100);
	}
	ImGui::EndFeaturesChild();


	ImGui::SetCursorPos(ImVec2(95 * DPI_SCALE, (ChildHeight50 + 47) * DPI_SCALE));
	ImGui::BeginFeaturesChild("RCS", ImVec2(ChildWidht - 25, ChildHeight50 - 3));
	{
		ImGui::Checkbox(xorstr_("Activate RCS"), &cfg::recoil_ativo);
		ImGui::SliderInt(xorstr_("Recoil Steps"), &cfg::recoil_length, 0, 100);
		ImGui::SliderFloat(xorstr_("Recoil Speed"), &cfg::recoil_speed, 0, 1);
		ImGui::SliderInt(xorstr_("Recoil Delay"), &cfg::time_to_start, 0, 500);
		ImGui::SliderInt(xorstr_("Recoil Sleep"), &cfg::recoil_sleep, 1, 100);
		ImGui::KeyBind("Recoil Key", &cfg::recoil_key);
	}
	ImGui::EndFeaturesChild();

	/*ImGui::SetCursorPos(ImVec2(ChildPosRight, 20 * DPI_SCALE));
	ImGui::BeginFeaturesChild("Trigger", ImVec2(ChildWidht, ChildHeight100));
	{
		static int sliderint = 50;
		ImGui::SliderInt("SLider", &sliderint, 0, 100);
		static bool teste2 = false;
		ImGui::Checkbox("Teste2", &teste2);
		ImGui::ColorEdit4("Color Picker", (float*)&MenuColor.Value);
		static int selectedkey = 0;
		ImGui::KeyBind("Piroca", &selectedkey);
		static char pirocagrande[64] = "dawfawf";
		ImGui::InputText("Labelilua", pirocagrande, sizeof(pirocagrande));
		static bool teste = false;
		ImGui::Checkbox("Testedwa", &teste);
	}
	ImGui::EndFeaturesChild();*/
}
ImVec4 corListBoxHeader = ImVec4(21.0f / 255.0f, 21.0f / 255.0f, 21.0f / 255.0f, 1.0f);

void Menu::DrawColor_AndConfig()
{
	ImGui::SetCursorPos(ImVec2(95 * DPI_SCALE, 70 * DPI_SCALE));
	ImGui::BeginFeaturesChild("Colors", ImVec2(ChildWidht - 27, ChildHeight50));
	{
		static bool isButtonSet[4] = { true, false, false, false };
		ImGui::SetCursorPosX(10);
		ImGui::Text(xorstr_("Apex Deuteranopia"));
		ImGui::SetCursorPosX(10);
		if (ImGui::Button((isButtonSet[0] ? xorstr_("current") : xorstr_("set ##3")), ImVec2(55, 25)))
		{
			cfg::color_mode = 0;
			isButtonSet[0] = true;
			isButtonSet[1] = false;
			isButtonSet[2] = false;
			isButtonSet[3] = false;
			cfg::menorRGB[0] = 70;
			cfg::menorRGB[1] = 0;
			cfg::menorRGB[2] = 120;

			cfg::maiorRGB[0] = 255;
			cfg::maiorRGB[1] = 190;
			cfg::maiorRGB[2] = 255;

			cfg::menorHSV[0] = 270;
			cfg::menorHSV[1] = 38;
			cfg::menorHSV[2] = 40;

			cfg::maiorHSV[0] = 310;
			cfg::maiorHSV[1] = 100;
			cfg::maiorHSV[2] = 100;
		};
		ImGui::SetCursorPosX(10);
		ImGui::Text(xorstr_("Valorant Yellow"));
		ImGui::SetCursorPosX(10);
		if (ImGui::Button((isButtonSet[1] ? xorstr_("current") : xorstr_("Set ##4")), ImVec2(55, 25))) {
			cfg::color_mode = 1;
			isButtonSet[0] = false;
			isButtonSet[1] = true;
			isButtonSet[2] = false;
			isButtonSet[3] = false;
			cfg::menorRGB[0] = 70;
			cfg::menorRGB[1] = 110;
			cfg::menorRGB[2] = 120;

			cfg::maiorRGB[0] = 255;
			cfg::maiorRGB[1] = 190;
			cfg::maiorRGB[2] = 255;

			cfg::menorHSV[0] = 270;
			cfg::menorHSV[1] = 25;
			cfg::menorHSV[2] = 40;

			cfg::maiorHSV[0] = 310;
			cfg::maiorHSV[1] = 100;
			cfg::maiorHSV[2] = 100;
		};
		ImGui::SetCursorPosX(10);
		ImGui::Text(xorstr_("Marvel Rivals Yellow"));
		ImGui::SetCursorPosX(10);
		if (ImGui::Button((isButtonSet[2] ? xorstr_("current") : xorstr_("Set ##2")), ImVec2(55, 25))) {
			cfg::color_mode = 2;
			isButtonSet[0] = false;
			isButtonSet[1] = false;
			isButtonSet[2] = true;
			isButtonSet[3] = false;
			cfg::menorRGB[0] = 168;
			cfg::menorRGB[1] = 168;
			cfg::menorRGB[2] = 0;
			cfg::maiorRGB[0] = 255;
			cfg::maiorRGB[1] = 255;
			cfg::maiorRGB[2] = 110;
			cfg::menorHSV[0] = 55;
			cfg::menorHSV[1] = 5;
			cfg::menorHSV[2] = 70;
			cfg::maiorHSV[0] = 65;
			cfg::maiorHSV[1] = 100;
			cfg::maiorHSV[2] = 100;
		};
		ImGui::SetCursorPosX(10);
		ImGui::Text(xorstr_("OverWatch Magenta"));
		ImGui::SetCursorPosX(10);
		if (ImGui::Button((isButtonSet[3] ? xorstr_("current") : xorstr_("Set ##1")), ImVec2(55, 25))) {
			cfg::color_mode = 3;
			isButtonSet[0] = false;
			isButtonSet[1] = false;
			isButtonSet[2] = false;
			isButtonSet[3] = true;
			cfg::menorRGB[0] = 225;
			cfg::menorRGB[1] = 45;
			cfg::menorRGB[2] = 45;
			cfg::maiorRGB[0] = 255;
			cfg::maiorRGB[1] = 136;
			cfg::maiorRGB[2] = 136;
			cfg::menorHSV[0] = 0;
			cfg::menorHSV[1] = 37;
			cfg::menorHSV[2] = 88;
			cfg::maiorHSV[0] = 1;
			cfg::maiorHSV[1] = 80;
			cfg::maiorHSV[2] = 100;
		};

	}
	ImGui::EndFeaturesChild();

	ImGui::SetCursorPos(ImVec2(370 * DPI_SCALE, 70 * DPI_SCALE));
	ImGui::BeginFeaturesChild("Configs", ImVec2(ChildWidht - 27, ChildHeight50));
	{
		if (ImGui::InputText(xorstr_("Create Config"), newConfigName, sizeof(newConfigName), ImGuiInputTextFlags_EnterReturnsTrue)) {
			if (newConfigName[0] != '\0') {
				Config newConfig;
				newConfig.name = newConfigName;
				newConfig.path = xorstr_("C:\\") + std::string(newConfigName) + xorstr_(".ini"); // Defina o caminho completo
				if (CreateEmptyFile(newConfig.path.c_str())) {
					configs.push_back(newConfig);
					newConfigName[0] = '\0'; // Limpa o campo de entrada
					selectedConfigIndex = configs.size() - 1; // Define a configura��o rec�m-criada como selecionada
					Save_Settings(configs[selectedConfigIndex].path.c_str());
				}
			}
		}
		ImGui::SetCursorPosX(5);
		ImGui::PushStyleColor(ImGuiCol_Header, corListBoxHeader);
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, corListBoxHeader);
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, corListBoxHeader);


		ImGui::ListBoxHeader(xorstr_("##ConfigList"), ImVec2(256, 140));
		for (int i = 0; i < configs.size(); ++i) {
			bool isSelected = (selectedConfigIndex == i);
			if (ImGui::Selectable(configs[i].name.c_str(), isSelected)) {
				selectedConfigIndex = i;
				// Quando uma configura��o for selecionada, atualize o texto na caixa de entrada
				strcpy_s(newConfigName, configs[i].name.c_str());
			}
		}
		ImGui::ListBoxFooter();
		ImGui::PopStyleColor(3);
	}
	ImGui::EndFeaturesChild();

	ImGui::SetCursorPos(ImVec2(370 * DPI_SCALE, (ChildHeight50 + 80) * DPI_SCALE));
	ImGui::BeginFeaturesChild("Manage Config", ImVec2(ChildWidht - 25, ChildHeight50 - 37));
	{
		float y = 10;
		ImGui::SetCursorPosX(10);
		ImGui::SetCursorPosY(y);

		if (ImGui::Button(xorstr_("Create Config"), ImVec2(120, 25))) {
			if (newConfigName[0] != '\0') {
				Config newConfig;
				newConfig.name = newConfigName;
				newConfig.path = xorstr_("C:\\") + std::string(newConfigName) + xorstr_(".ini"); // Defina o caminho completo
				if (CreateEmptyFile(newConfig.path.c_str())) {
					configs.push_back(newConfig);
					newConfigName[0] = '\0'; // Limpa o campo de entrada
					selectedConfigIndex = configs.size() - 1; // Define a configura��o rec�m-criada como selecionada
					Save_Settings(configs[selectedConfigIndex].path.c_str());
				}
			}
		}
		ImGui::SetCursorPosX(10);
		y += 35;
		ImGui::SetCursorPosY(y);
		if (ImGui::Button(xorstr_("Delete Config"), ImVec2(120, 25)) && selectedConfigIndex >= 0 && selectedConfigIndex < configs.size()) {
			const std::string filePath = configs[selectedConfigIndex].path;
			if (DeleteFile(filePath.c_str())) {
				configs.erase(configs.begin() + selectedConfigIndex);
				selectedConfigIndex = -1;
			}
		}
		ImGui::SetCursorPosX(10);
		y += 35;
		ImGui::SetCursorPosY(y);
		if (ImGui::Button(((xorstr_("Save Config"))), ImVec2(120, 25)))
		{
			if (selectedConfigIndex >= 0 && selectedConfigIndex < configs.size()) {
				Save_Settings(configs[selectedConfigIndex].path.c_str());
			}
		}
		ImGui::SetCursorPosX(10);
		y += 35;
		ImGui::SetCursorPosY(y);
		if (ImGui::Button(((xorstr_("Load Config"))), ImVec2(120, 25)))
		{
			if (selectedConfigIndex >= 0 && selectedConfigIndex < configs.size()) {
				Load_Settings(configs[selectedConfigIndex].path.c_str());
			}

		}
	}
	ImGui::EndFeaturesChild();


	ImGui::SetCursorPos(ImVec2(95 * DPI_SCALE, (ChildHeight50 + 80) * DPI_SCALE));
	ImGui::BeginFeaturesChild("Infos", ImVec2(ChildWidht - 25, ChildHeight50 - 37));
	{
		
	}
	ImGui::EndFeaturesChild();

	/*ImGui::SetCursorPos(ImVec2(ChildPosRight, 20 * DPI_SCALE));
	ImGui::BeginFeaturesChild("Trigger", ImVec2(ChildWidht, ChildHeight100));
	{
		static int sliderint = 50;
		ImGui::SliderInt("SLider", &sliderint, 0, 100);
		static bool teste2 = false;
		ImGui::Checkbox("Teste2", &teste2);
		ImGui::ColorEdit4("Color Picker", (float*)&MenuColor.Value);
		static int selectedkey = 0;
		ImGui::KeyBind("Piroca", &selectedkey);
		static char pirocagrande[64] = "dawfawf";
		ImGui::InputText("Labelilua", pirocagrande, sizeof(pirocagrande));
		static bool teste = false;
		ImGui::Checkbox("Testedwa", &teste);
	}
	ImGui::EndFeaturesChild();*/
}


std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<int> randomint(100, 999); // Range from 100 to 999 inclusive



void render_menu::render()
{
	LoadConfigs();

	ImGui::SetNextWindowPos(ImVec2(-1, -1), ImGuiCond_Once);
	ImGui::SetNextWindowSize({ width + 2, height + 2 });

	ImGui::BeginDragDropTarget();

	int sizem = cfg::advancednigger ? 62 : 80;
	static bool bools[50]{};
	static int ints[50]{};

	static float color[4] = { 1.f, 1.f, 1.f, 1.f };

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 150));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(510, 380));

	// Desative a rolagem da janela
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

	ImGui::Begin(xorstr_("AimColor"), NULL,ImGuiWindowFlags_NoMove |ImGuiWindowFlags_NoTitleBar |ImGuiWindowFlags_NoScrollbar |ImGuiWindowFlags_NoCollapse |ImGuiWindowFlags_NoResize);
	{
		ChildPosRight = (ImGui::GetWindowSize().x / 2 + 10 * DPI_SCALE);
		ChildPosBottom = (ImGui::GetWindowSize().y / 2 + 10 * DPI_SCALE);
		ChildWidht = (ImGui::GetWindowSize().x / 2 - 20 * DPI_SCALE - 10 * DPI_SCALE);
		ChildHeight50 = (ImGui::GetWindowSize().y / 2 - 20 * DPI_SCALE - 10 * DPI_SCALE);
		ChildHeight100 = (ImGui::GetWindowSize().y - 80 * DPI_SCALE);

		float LegitTabAlpha = FastFloatLerp__("LegitTabAlphaAnimation", SelectedTab == 0, 0, 1.f, 0.05f);
		if (ImGui::GetStyle().Alpha != 1.f && SelectedTab == 0)
			LegitTabAlpha = ImGui::GetStyle().Alpha;
		if (LegitTabAlpha != 0)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, LegitTabAlpha);
			{
				DrawLegitTab();
			}
			ImGui::PopStyleVar();
		}

		float ESPTabAlpha = FastFloatLerp__("ESPTabAlphaAnimation", SelectedTab == 1, 0, 1.f, 0.05f);
		if (ImGui::GetStyle().Alpha != 1.f && SelectedTab == 0)
			ESPTabAlpha = ImGui::GetStyle().Alpha;
		if (ESPTabAlpha != 0)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ESPTabAlpha);
			{
				DrawFlicker();
			}
			ImGui::PopStyleVar();
		}

		float MiscTabAlpha = FastFloatLerp__("MiscTabAlphaAnimation", SelectedTab == 2, 0, 1.f, 0.05f);
		if (ImGui::GetStyle().Alpha != 1.f && SelectedTab == 0)
			MiscTabAlpha = ImGui::GetStyle().Alpha;
		if (MiscTabAlpha != 0)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, MiscTabAlpha);
			{
				DrawColor_AndConfig();
			}
			ImGui::PopStyleVar();
		}

		float ConfigTabAlpha = FastFloatLerp__("ConfigTabAlphaAnimation", SelectedTab == 3, 0, 1.f, 0.05f);
		if (ImGui::GetStyle().Alpha != 1.f && SelectedTab == 0)
			ConfigTabAlpha = ImGui::GetStyle().Alpha;
		if (ConfigTabAlpha != 0)
		{
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ConfigTabAlpha);
			{
				
			}
			ImGui::PopStyleVar();
		}
		ImDrawList* drawlist = ImGui::GetWindowDrawList();
		ImVec2 pos = ImGui::GetWindowPos();
		ImVec2 size = ImGui::GetWindowSize();

		ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + 85 * DPI_SCALE, pos.y + size.y), ImColor(27, 27, 27), ImGui::GetStyle().WindowRounding, ImDrawCornerFlags_Left); // Left Bar
		ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(pos.x + 85 * DPI_SCALE, pos.y), ImVec2(pos.x + size.x, pos.y + 60 * DPI_SCALE), ImColor(23, 23, 23), ImGui::GetStyle().WindowRounding, ImDrawCornerFlags_TopRight); // Top Bar

		ImGui::GetWindowDrawList()->AddLine(ImVec2(pos.x + 85 * DPI_SCALE, pos.y), ImVec2(pos.x + 85 * DPI_SCALE, pos.y + size.y), ImColor(0, 0, 0), 1.5f); // Left Line BG
		ImGui::GetWindowDrawList()->AddLine(ImVec2(pos.x + 85 * DPI_SCALE, pos.y), ImVec2(pos.x + 85 * DPI_SCALE, pos.y + size.y), ImColor(73, 73, 73)); // Left Line

		ImGui::GetWindowDrawList()->AddLine(ImVec2(pos.x + 86 * DPI_SCALE, pos.y + 60 * DPI_SCALE), ImVec2(pos.x + size.x, pos.y + 60 * DPI_SCALE), ImColor(0, 0, 0), 1.5f); // Top Line BG
		ImGui::GetWindowDrawList()->AddLine(ImVec2(pos.x + 86 * DPI_SCALE, pos.y + 60 * DPI_SCALE), ImVec2(pos.x + size.x, pos.y + 60 * DPI_SCALE), ImColor(73, 73, 73)); // Top Line


		//ImGui::PushFont(Icons);
		//drawlist->AddText(ImVec2(pos.x + 42.5f - ImGui::CalcTextSize("0").x / 2, pos.y + 30 - ImGui::CalcTextSize("0").y / 2), Menu::MenuColor, "0"); // Logo
		constexpr int imageWidth{ 50 };
		constexpr int imageHeight{ 50 };
		constexpr int offsetX{ 10 };  // Ajuste conforme necess�rio

		drawlist->AddImage(logo, ImVec2(5 + offsetX, 5), ImVec2(5 + offsetX + imageWidth, 5 + imageHeight));

		//ImGui::PopFont();


		static const char* SelectedTabName = "Legit";
		static const char* SelectedTabName_Login = "Login";
		static float TextAnimationPos = 0;

		ImGui::SetCursorPos(ImVec2(0, 60 * DPI_SCALE));
		ImGui::BeginGroup();
		{
			if (SelectedTab == 3)
			{
				if (ImGui::Tab(SelectedTabName_Login, "4", SelectedTab == 3))
				{
					SelectedTab = 3;
					SelectedTabName_Login = "Login";
					TextAnimationPos = ImGui::CalcTextSize(SelectedTabName_Login).x + 10;
				}
			}
			else
			{
				if (ImGui::Tab("Legit", "1", SelectedTab == 0))
				{
					SelectedTab = 0;
					SelectedTabName = "Legit";
					TextAnimationPos = ImGui::CalcTextSize(SelectedTabName).x + 10;
				}
				if (ImGui::Tab("Exploits", "2", SelectedTab == 1))
				{
					SelectedTab = 1;
					SelectedTabName = "Exploits";
					TextAnimationPos = ImGui::CalcTextSize(SelectedTabName).x + 10;
				}
				if (ImGui::Tab("Misc", "3", SelectedTab == 2))
				{
					SelectedTab = 2;
					SelectedTabName = "Misc";
					TextAnimationPos = ImGui::CalcTextSize(SelectedTabName).x + 10;
				}
			}

			
			
		}
		ImGui::EndGroup();

		if (TextAnimationPos > 0 && TextAnimationPos != 0)
			TextAnimationPos -= 2;

		ImGui::PushClipRect(ImVec2(pos.x + 86, pos.y), ImVec2(pos.x + size.x, pos.y + 60), true);
		ImGui::PushFont(Bold);
		if (SelectedTab == 3)
		{
			drawlist->AddText(ImVec2(pos.x + 86 * DPI_SCALE + 10 * DPI_SCALE - TextAnimationPos, pos.y + 30 * DPI_SCALE - ImGui::CalcTextSize(SelectedTabName_Login).y / 2 + 2 * DPI_SCALE), ImColor(255, 255, 255), SelectedTabName_Login);
		}
		else
		{
			drawlist->AddText(ImVec2(pos.x + 86 * DPI_SCALE + 10 * DPI_SCALE - TextAnimationPos, pos.y + 30 * DPI_SCALE - ImGui::CalcTextSize(SelectedTabName).y / 2 + 2 * DPI_SCALE), ImColor(255, 255, 255), SelectedTabName);
		}
		
		ImGui::PopFont();
		ImGui::PopClipRect();

		drawlist->AddLine(ImVec2(pos.x + 1 * DPI_SCALE, pos.y + size.y - 60 * DPI_SCALE), ImVec2(pos.x + 85 * DPI_SCALE, pos.y + size.y - 60 * DPI_SCALE), ImColor(0, 0, 0), 1.5f); // Bot Line BG
		drawlist->AddLine(ImVec2(pos.x + 1 * DPI_SCALE, pos.y + size.y - 60 * DPI_SCALE), ImVec2(pos.x + 85 * DPI_SCALE, pos.y + size.y - 60 * DPI_SCALE), ImColor(73, 73, 73)); // Bot Line

		drawlist->AddRect(ImVec2(pos.x + 1 * DPI_SCALE, pos.y + 1 * DPI_SCALE), ImVec2(pos.x + size.x - 1 * DPI_SCALE, pos.y + size.y - 1 * DPI_SCALE), ImColor(0, 0, 0), ImGui::GetStyle().WindowRounding, ImDrawCornerFlags_All, 1.5f); // Border BG
		drawlist->AddRect(ImVec2(pos.x + 1 * DPI_SCALE, pos.y + 1 * DPI_SCALE), ImVec2(pos.x + size.x - 1 * DPI_SCALE, pos.y + size.y - 1 * DPI_SCALE), ImColor(73, 73, 73), ImGui::GetStyle().WindowRounding); // Border
	}
	ImGui::End();

	ImGui::PopStyleVar(2);
}



void render_menu::render_()
{
	if (Menu::DPI_SCALE == 0)
		return;

	int flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove;
	if (DPI_SCALE < 1)
	{
		flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove;
		ImGui::SetNextWindowPos(ImVec2(0,0));
	}

	ImGui::SetNextWindowSize(ImVec2(DefaultMenuWidht * DPI_SCALE, DefaultMenuHeight * DPI_SCALE));
	ImGui::Begin("TogamiGUI", NULL, flags);
	{
		if (DPI_SCALE == 1)
			LastMenuPos = ImGui::GetWindowPos();

		ImDrawList* drawlist = ImGui::GetWindowDrawList();
		ImVec2 pos = ImGui::GetWindowPos();
		ImVec2 size = ImGui::GetWindowSize();

		ImGui::SetCursorPos(ImVec2(10 * DPI_SCALE, 10 * DPI_SCALE));
		ImGui::BeginChild("MainChild", ImVec2(size.x - 85 * DPI_SCALE, size.y - 60 * DPI_SCALE), false, ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
		{
			ChildPosRight = (ImGui::GetWindowSize().x / 2 + 10 * DPI_SCALE);
			ChildPosBottom = (ImGui::GetWindowSize().y / 2 + 10 * DPI_SCALE);
			ChildWidht = (ImGui::GetWindowSize().x / 2 - 20 * DPI_SCALE - 10 * DPI_SCALE);
			ChildHeight50 = (ImGui::GetWindowSize().y / 2 - 20 * DPI_SCALE - 10 * DPI_SCALE);
			ChildHeight100 = (ImGui::GetWindowSize().y - 40 * DPI_SCALE);

			float LegitTabAlpha = FastFloatLerp__("LegitTabAlphaAnimation", SelectedTab == 0, 0, 1.f, 0.05f);
			if (ImGui::GetStyle().Alpha != 1.f && SelectedTab == 0)
				LegitTabAlpha = ImGui::GetStyle().Alpha;
			if (LegitTabAlpha != 0)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, LegitTabAlpha);
				{
					//DrawLegitTab();
				}
				ImGui::PopStyleVar();
			}

			float ESPTabAlpha = FastFloatLerp__("ESPTabAlphaAnimation", SelectedTab == 1, 0, 1.f, 0.05f);
			if (ImGui::GetStyle().Alpha != 1.f && SelectedTab == 0)
				ESPTabAlpha = ImGui::GetStyle().Alpha;
			if (ESPTabAlpha != 0)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ESPTabAlpha);
				{
					//DrawESPTab();
				}
				ImGui::PopStyleVar();
			}

			float MiscTabAlpha = FastFloatLerp__("MiscTabAlphaAnimation", SelectedTab == 2, 0, 1.f, 0.05f);
			if (ImGui::GetStyle().Alpha != 1.f && SelectedTab == 0)
				MiscTabAlpha = ImGui::GetStyle().Alpha;
			if (MiscTabAlpha != 0)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, MiscTabAlpha);
				{
					//DrawMiscTab();
				}
				ImGui::PopStyleVar();
			}

			float ConfigTabAlpha = FastFloatLerp__("ConfigTabAlphaAnimation", SelectedTab == 3, 0, 1.f, 0.05f);
			if (ImGui::GetStyle().Alpha != 1.f && SelectedTab == 0)
				ConfigTabAlpha = ImGui::GetStyle().Alpha;
			if (ConfigTabAlpha != 0)
			{
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ConfigTabAlpha);
				{
					//DrawConfigTab();
				}
				ImGui::PopStyleVar();
			}
		}
		ImGui::EndChild();

		drawlist->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + 85 * DPI_SCALE, pos.y + size.y), ImColor(27, 27, 27), ImGui::GetStyle().WindowRounding, ImDrawCornerFlags_Left); // Left Bar
		drawlist->AddRectFilled(ImVec2(pos.x + 85 * DPI_SCALE, pos.y), ImVec2(pos.x + size.x, pos.y + 60 * DPI_SCALE), ImColor(23, 23, 23), ImGui::GetStyle().WindowRounding, ImDrawCornerFlags_TopRight); // Top Bar

		drawlist->AddLine(ImVec2(pos.x + 85 * DPI_SCALE, pos.y), ImVec2(pos.x + 85 * DPI_SCALE, pos.y + size.y), ImColor(0, 0, 0), 1.5f); // Left Line BG
		drawlist->AddLine(ImVec2(pos.x + 85 * DPI_SCALE, pos.y), ImVec2(pos.x + 85 * DPI_SCALE, pos.y + size.y), ImColor(73, 73, 73)); // Left Line

		drawlist->AddLine(ImVec2(pos.x + 86 * DPI_SCALE, pos.y + 60 * DPI_SCALE), ImVec2(pos.x + size.x, pos.y + 60 * DPI_SCALE), ImColor(0, 0, 0), 1.5f); // Top Line BG
		drawlist->AddLine(ImVec2(pos.x + 86 * DPI_SCALE, pos.y + 60 * DPI_SCALE), ImVec2(pos.x + size.x, pos.y + 60 * DPI_SCALE), ImColor(73, 73, 73)); // Top Line


		ImGui::PushFont(Icons);
		drawlist->AddText(ImVec2(pos.x + 42.5f - ImGui::CalcTextSize("0").x / 2, pos.y + 30 - ImGui::CalcTextSize("0").y / 2), Menu::MenuColor, "0"); // Logo
		ImGui::PopFont();

		static const char* SelectedTabName = "Legit";
		static float TextAnimationPos = 0;

		ImGui::SetCursorPos(ImVec2(0, 60 * DPI_SCALE));
		ImGui::BeginGroup();
		{
			if (ImGui::Tab("Legit", "1", SelectedTab == 0))
			{
				SelectedTab = 0;
				SelectedTabName = "Legit";
				TextAnimationPos = ImGui::CalcTextSize(SelectedTabName).x + 10;
			}
			if (ImGui::Tab("ESP", "2", SelectedTab == 1))
			{
				SelectedTab = 1;
				SelectedTabName = "ESP";
				TextAnimationPos = ImGui::CalcTextSize(SelectedTabName).x + 10;
			}
			if (ImGui::Tab("Misc", "3", SelectedTab == 2))
			{
				SelectedTab = 2;
				SelectedTabName = "Misc";
				TextAnimationPos = ImGui::CalcTextSize(SelectedTabName).x + 10;
			}
			if (ImGui::Tab("Config", "4", SelectedTab == 3))
			{
				SelectedTab = 3;
				SelectedTabName = "Config";
				TextAnimationPos = ImGui::CalcTextSize(SelectedTabName).x + 10;
			}
		}
		ImGui::EndGroup();

		if (TextAnimationPos > 0 && TextAnimationPos != 0)
			TextAnimationPos -= 2;

		ImGui::PushClipRect(ImVec2(pos.x + 86, pos.y), ImVec2(pos.x + size.x, pos.y + 60), true);
		//ImGui::PushFont(Bold);
		drawlist->AddText(ImVec2(pos.x + 86 * DPI_SCALE + 10 * DPI_SCALE - TextAnimationPos, pos.y + 30 * DPI_SCALE - ImGui::CalcTextSize(SelectedTabName).y / 2 + 2 * DPI_SCALE), ImColor(255, 255, 255), SelectedTabName);
		//ImGui::PopFont();
		ImGui::PopClipRect();

		drawlist->AddLine(ImVec2(pos.x + 1 * DPI_SCALE, pos.y + size.y - 60 * DPI_SCALE), ImVec2(pos.x + 85 * DPI_SCALE, pos.y + size.y - 60 * DPI_SCALE), ImColor(0, 0, 0), 1.5f); // Bot Line BG
		drawlist->AddLine(ImVec2(pos.x + 1 * DPI_SCALE, pos.y + size.y - 60 * DPI_SCALE), ImVec2(pos.x + 85 * DPI_SCALE, pos.y + size.y - 60 * DPI_SCALE), ImColor(73, 73, 73)); // Bot Line

		drawlist->AddRect(ImVec2(pos.x + 1 * DPI_SCALE, pos.y + 1 * DPI_SCALE), ImVec2(pos.x + size.x - 1 * DPI_SCALE, pos.y + size.y - 1 * DPI_SCALE), ImColor(0, 0, 0), ImGui::GetStyle().WindowRounding, ImDrawCornerFlags_All, 1.5f); // Border BG
		drawlist->AddRect(ImVec2(pos.x + 1 * DPI_SCALE, pos.y + 1 * DPI_SCALE), ImVec2(pos.x + size.x - 1 * DPI_SCALE, pos.y + size.y - 1 * DPI_SCALE), ImColor(73, 73, 73), ImGui::GetStyle().WindowRounding); // Border
	}
	ImGui::End();
}

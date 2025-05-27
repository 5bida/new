#ifndef RENDER_MENU_HPP
#define RENDER_MENU_HPP

#include <string>
#include <D3DX11.h>
#include <d3d11.h>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem> // Para usar a biblioteca filesystem
#pragma comment(lib, "d3d11.lib")

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

inline bool dialog = true;
namespace render_menu
{
	constexpr int width{ 650 };
	constexpr int height{ 550 };
	inline bool is_running{ true };

	inline ImFont* icon_font{ nullptr };
	inline ImFont* SemiBold{ nullptr };
	inline ImFont* Icons{ nullptr };
	inline HWND window{ nullptr };
	inline WNDCLASSEX window_class = { };

	static ID3D11Device* device{ NULL };
	static ID3D11DeviceContext* device_context{ NULL };
	static IDXGISwapChain* swap_chain{ NULL };
	static ID3D11RenderTargetView* render_target_view{ NULL };
	static ID3D11ShaderResourceView* background{ NULL };

	void create_window(LPCWSTR window_name);
	void destroy_window();

	bool create_device();
	void destroy_device();

	void create_render_target();
	void destory_render_target();

	void create_imgui();
	void destroy_imgui();

	void begin_render();
	void end_render();
	void render();
	void render_();
}

#endif // !RENDER_MENU_HPP


namespace Menu
{
	void DrawLegitTab();
	void DrawFlicker();
	void DrawColor_AndConfig();
	extern float DPI_SCALE;
	//inline extern bool bIsMenuOpened;

	extern ImFont* Bold;
	extern ImFont* Medium;
	extern ImFont* Regular;
	extern ImFont* Light;

	extern int DefaultMenuWidht;
	extern int DefaultMenuHeight;

	extern ImColor MenuColor;

	extern float ChildPosRight;
	extern float ChildPosBottom;
	extern float ChildWidht;
	extern float ChildHeight50;
	extern float ChildHeight100;
}



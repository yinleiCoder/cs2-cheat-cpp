#pragma once
#include <d3d11.h>
#include <dwmapi.h>
#include <windowsx.h>
#include <Windows.h>

namespace gui
{
	// imgui控件状态
	inline bool exit = true;
	inline bool menutoggle = true;
	inline bool boxEsp = true;// 方框透视
	inline bool playerBodyGlow = true;// 玩家身体发光
	inline bool playerHealth = true;// 玩家血量
	inline bool aimbot = false;// 自瞄并锁头
	inline bool rcs = false; // 后座力补偿
	inline bool radar = false;// 雷达
	inline bool flash = true; // 防闪光
	inline bool bhop = false;// 连跳

	// win32api window相关变量
	inline HWND overlay = nullptr;
	inline WNDCLASSEXW windowClass = {};

	// directx相关变量
	inline DXGI_SWAP_CHAIN_DESC sd{};
	inline ID3D11Device* device{ nullptr };
	inline ID3D11DeviceContext* device_context{ nullptr };
	inline IDXGISwapChain* swap_chain{ nullptr };
	inline ID3D11RenderTargetView* render_target_view{ nullptr };
	inline D3D_FEATURE_LEVEL level{};

	// 窗口的创建和销毁
	void CreateHWindow(
		const char* windowName,
		const char* className,
		HINSTANCE instance,
		INT cmd_show) noexcept;
	void DestroyHWindow() noexcept;

	// directx设备的创建和销毁
	bool CreateDevice() noexcept;
	void ResetDevice() noexcept;
	void DestroyDevice() noexcept;

	// ImGui的创建和销毁
	void CreateImGui() noexcept;
	void DestroyImGui() noexcept;

	void BeginRender() noexcept;
	void EndRender() noexcept;
	void Render() noexcept;
}
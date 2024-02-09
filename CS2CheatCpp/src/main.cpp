#include <Windows.h>
#include <thread>
#include <dwmapi.h>
#include <d3d11.h>
#include <windowsx.h>
#include "../dependencies/ImGui/imgui.h"
#include "../dependencies/ImGui/imgui_impl_dx11.h"
#include "../dependencies/ImGui/imgui_impl_win32.h"
#include "memory/memory.h"
#include "vector.h"
#include "render.h"
#include "bone.hpp"

namespace offsets {
	// offsets.hpp
	constexpr std::ptrdiff_t dwLocalPlayerPawn = 0x17262E8;
	constexpr std::ptrdiff_t dwEntityList = 0x18B0FC8;
	constexpr std::ptrdiff_t dwViewMatrix = 0x19102B0;
	// client.dll.hpp 
	constexpr std::ptrdiff_t m_iHealth = 0x334; // int32_t
	constexpr std::ptrdiff_t m_hPlayerPawn = 0x7E4; // CHandle<C_CSPlayerPawn>
	constexpr std::ptrdiff_t m_iTeamNum = 0x3CB; // uint8_t
	constexpr std::ptrdiff_t m_vOldOrigin = 0x127C; // Vector
}

// Screen width height
int screenWidth = GetSystemMetrics(SM_CXSCREEN);
int screenHeight = GetSystemMetrics(SM_CYSCREEN);

// ImGui Win32 Handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK window_procedure(HWND window, UINT message, WPARAM w_param, LPARAM l_param) {
	if (ImGui_ImplWin32_WndProcHandler(window, message, w_param, l_param)) {
		return 0L;
	}

	if (message == WM_DESTROY) {
		PostQuitMessage(0);
		return 0L;
	}

	switch (message)
	{
	case WM_NCHITTEST:
	{
		const LONG borderWith = GetSystemMetrics(SM_CXSIZEFRAME);
		const LONG titleBarHeight = GetSystemMetrics(SM_CYSIZEFRAME);
		POINT cursorPos = { GET_X_LPARAM(w_param), GET_Y_LPARAM(l_param) };
		RECT windowRect;
		GetWindowRect(window, &windowRect);
		if (cursorPos.y >= windowRect.top && cursorPos.y < windowRect.top + titleBarHeight) {
			return HTCAPTION;
		}
		break;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(window, message, w_param, l_param);
	}
}

INT APIENTRY WinMain(HINSTANCE instance, HINSTANCE, PSTR, INT cmd_show)
{
	auto mem = Memory("cs2.exe");
	//mem.GetProcessId()
	const auto client = mem.GetModuleAddress("client.dll");
	//std::cout << "client.dll -> " << "0x" << std::hex << client << std::dec << std::endl;
	//std::this_thread::sleep_for(std::chrono::milliseconds(5));

	WNDCLASSEXW wc{};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = window_procedure;
	wc.hInstance = instance;
	wc.lpszClassName = L"cs2yinlei";
	
	RegisterClassExW(&wc);

	const HWND overlay = CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
		wc.lpszClassName,
		L"cs2yinlei",
		WS_POPUP,
		0,
		0,
		screenWidth,
		screenHeight,
		nullptr,
		nullptr,
		wc.hInstance,
		nullptr
	);

	SetLayeredWindowAttributes(overlay, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);

	{
		RECT client_area{};
		GetClientRect(overlay, &client_area);

		RECT window_area{};
		GetWindowRect(overlay, &window_area);

		POINT diff{};
		ClientToScreen(overlay, &diff);

		const MARGINS margins{
			window_area.left + (diff.x - window_area.left),
			window_area.top + (diff.y - window_area.top),
			client_area.right,
			client_area.bottom,
		};
		DwmExtendFrameIntoClientArea(overlay, &margins);
	}

	DXGI_SWAP_CHAIN_DESC sd{};
	sd.BufferDesc.RefreshRate.Numerator = 60U; // fps
	sd.BufferDesc.RefreshRate.Denominator = 1U;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.SampleDesc.Count = 1U;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 2U;
	sd.OutputWindow = overlay;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	constexpr D3D_FEATURE_LEVEL levels[2]{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_0,
	};

	ID3D11Device* device{ nullptr };
	ID3D11DeviceContext* device_context{ nullptr };
	IDXGISwapChain* swap_chain{ nullptr };
	ID3D11RenderTargetView* render_target_view{ nullptr };
	D3D_FEATURE_LEVEL level{};

	D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		0U,
		levels,
		2U,
		D3D11_SDK_VERSION,
		&sd,
		&swap_chain,
		&device,
		&level,
		&device_context
	);

	ID3D11Texture2D* back_buffer{ nullptr };
	swap_chain->GetBuffer(0U, IID_PPV_ARGS(&back_buffer));

	if (back_buffer) {
		device->CreateRenderTargetView(back_buffer, nullptr, &render_target_view);
		back_buffer->Release();
	}
	else {
		return 1;
	}

	ShowWindow(overlay, cmd_show);
	UpdateWindow(overlay);

	ImGui::CreateContext();
	ImGui::StyleColorsClassic();

	ImGui_ImplWin32_Init(overlay);
	ImGui_ImplDX11_Init(device, device_context);

	bool running = true;

	while (running) 
	{
		/*MSG msg;
		while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) {
				running = false;
			}
		}
		if (!running) {
			break;
		}*/

		// 处理外挂业务逻辑: 获取相关数据的地址
		uintptr_t localPlayer = mem.Read<uintptr_t>(client+offsets::dwLocalPlayerPawn);
		Vector3 localOrigin = mem.Read<Vector3>(localPlayer+offsets::m_vOldOrigin);
		view_matrix_t view_matrix = mem.Read<view_matrix_t>(client+offsets::dwViewMatrix);
		uintptr_t enity_list = mem.Read<uintptr_t>(client+offsets::dwEntityList);
		int localTeam = mem.Read<int>(localPlayer+offsets::m_iTeamNum);

		// ImGui渲染
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// 处理外挂业务逻辑: 
		for (int playerIndex = 1; playerIndex < 32; ++playerIndex)
		{
			uintptr_t list_entry = mem.Read<uintptr_t>(enity_list +(8*(playerIndex & 0x7FFF) >> 9)+16);
			if (!list_entry) {
				continue;
			}
			uintptr_t player = mem.Read<uintptr_t>(list_entry + 120 * (playerIndex & 0x1FF));
			if (!player) {
				continue;
			}
			int playerTeam = mem.Read<int>(player+offsets::m_iTeamNum);
			if (playerTeam == localTeam) {
				continue;
			}
			uint32_t playerPawn = mem.Read<uint32_t>(player+offsets::m_hPlayerPawn);
			uintptr_t list_entry2 = mem.Read<uintptr_t>(enity_list + 0x8 * ((playerPawn & 0x7FFF) >> 9) + 16);
			if (!list_entry2) {
				continue;
			}
			uintptr_t pCSPlayerPawn = mem.Read<uintptr_t>(list_entry2 + 120 * (playerPawn & 0x1FF));
			if (!pCSPlayerPawn) {
				continue;
			}
			int health = mem.Read<int>(pCSPlayerPawn+offsets::m_iHealth);
			if (health <= 0 || health > 100) {
				continue;
			}
			if (pCSPlayerPawn == localPlayer) {
				continue;
			}
			uintptr_t gameScene = mem.Read<uintptr_t>(pCSPlayerPawn + 0x310);
			uintptr_t boneArray = mem.Read<uintptr_t>(gameScene + 0x160 + 0x80);

			Vector3 origin = mem.Read<Vector3>(pCSPlayerPawn+offsets::m_vOldOrigin);
			Vector3 head = { origin.x, origin.y, origin.z + 75.f };
			//Vector3 head = mem.Read<Vector3>(boneArray + bones::head * 32);
			Vector3 screenPos;
			Vector3 screenHead;
			float headHeight = (screenPos.y - screenHead.y) / 8;

			RGB enemy = { 255, 0, 0 };
			RGB bone = { 255, 255, 255 };
			RGB hp = { 0, 255, 0 };

			if (!mem.InForeground()) {
				continue;
			}

			if (Vector3::WorldConvertToScreen(view_matrix, origin, screenPos) &&
				Vector3::WorldConvertToScreen(view_matrix, head, screenHead) &&
				origin.x != 0) {
				float height = screenPos.y - screenHead.y;
				float width = height / 2.4f;
					Render::DrawRect(
						screenHead.x - width /2,
						screenHead.y,
						width,
						height,
						enemy,
						1.5,
						false,
						255
					);

					Render::DrawRect(
						screenHead.x - (width / 2 + 10),
						screenHead.y + (height * (100 - health) / 100),
						2,
						height - (height * (100 - health) / 100),
						hp,
						1.5,
						true,
						255
					);
			}
			/*Render::Circle(
				screenHead.x,
				screenHead.y,
				headHeight - 3,
				bone
			);

			for (int i = 0; i < sizeof(boneConnections) / sizeof(boneConnections[0]); i++) {
				int bone1 = boneConnections[i].bone1;
				int bone2 = boneConnections[i].bone2;

				Vector3 vectorBone1 = mem.Read<Vector3>(boneArray + bone1*32);
				Vector3 vectorBone2 = mem.Read<Vector3>(boneArray + bone2*32);

				Vector3 b1 = vectorBone1.WorldConvertToScreen(view_matrix);
				Vector3 b2 = vectorBone1.WorldConvertToScreen(view_matrix);

				Render::Line(b1.x, b1.y, b2.x, b2.y, bone, 2.0);
			}*/
		}

		ImGui::Render();
		float color[4]{0, 0, 0, 0};
		device_context->OMSetRenderTargets(1U, &render_target_view, nullptr);
		device_context->ClearRenderTargetView(render_target_view, color);

		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		swap_chain->Present(0U, 0u);
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();

	ImGui::DestroyContext();

	if (swap_chain) {
		swap_chain->Release();
	}
	if (device_context) {
		device_context->Release();
	}
	if (device) {
		device->Release();
	}
	if (render_target_view) {
		render_target_view->Release();
	}

	DestroyWindow(overlay);
	UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return 0;
}
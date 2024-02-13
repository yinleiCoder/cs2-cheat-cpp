#include "gui.h"
#include "../dependencies/ImGui/imgui.h"
#include "../dependencies/ImGui/imgui_impl_dx11.h"
#include "../dependencies/ImGui/imgui_impl_win32.h"
#include <corecrt_math.h>

// ImGui Win32 Handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// 屏幕宽高
int screenWidth = GetSystemMetrics(SM_CXSCREEN);
int screenHeight = GetSystemMetrics(SM_CYSCREEN);

// windows消息处理
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
			return 0L;
		}
	}

	return DefWindowProc(window, message, w_param, l_param);
}

void gui::CreateHWindow(const char* windowName, const char* className, HINSTANCE instance, INT cmd_show) noexcept
{
	windowClass.cbSize = sizeof(WNDCLASSEXW);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = window_procedure;
	windowClass.hInstance = instance;// instance从WinMain入口获得
	windowClass.lpszClassName = L"cs2yinlei"; // className

	RegisterClassExW(&windowClass);

	overlay = CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
		windowClass.lpszClassName,
		L"cs2yinlei",// windowName
		WS_POPUP,
		0,
		0,
		screenWidth,
		screenHeight,
		nullptr,
		nullptr,
		windowClass.hInstance,
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

	ShowWindow(overlay, cmd_show);//cmd_show从WinMain入口获得
	UpdateWindow(overlay);
}

void gui::DestroyHWindow() noexcept
{
	DestroyWindow(overlay);
	UnregisterClassW(windowClass.lpszClassName, windowClass.hInstance);
}

bool gui::CreateDevice() noexcept
{
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
		return false;
	}
	return true;
}

void gui::ResetDevice() noexcept
{
}

void gui::DestroyDevice() noexcept
{
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
}

void gui::CreateImGui() noexcept
{
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(overlay);
	ImGui_ImplDX11_Init(device, device_context);
}

void gui::DestroyImGui() noexcept
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void gui::BeginRender() noexcept
{
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if (msg.message == WM_QUIT) {
			exit = false;
		}
	}

	// ImGui渲染
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void gui::EndRender() noexcept
{
	//ImGui::EndFrame();
	ImGui::Render();

	constexpr float color[4]{ 0.f, 0.f, 0.f, 0.f };
	device_context->OMSetRenderTargets(1U, &render_target_view, nullptr);
	device_context->ClearRenderTargetView(render_target_view, color);

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	// 开启垂直同步
	swap_chain->Present(1U, 0U);
}

void gui::Render() noexcept
{
	ImGui::Begin("My First Tool", &exit, ImGuiWindowFlags_MenuBar);
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */ }
			if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do stuff */ }
			if (ImGui::MenuItem("Close", "Ctrl+W")) { exit = false; }
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	// Generate samples and plot them
	float samples[100];
	for (int n = 0; n < 100; n++)
		samples[n] = sinf(n * 0.2f + ImGui::GetTime() * 1.5f);
	ImGui::PlotLines("Samples", samples, 100);

	// Display contents in a scrolling region
	ImGui::TextColored(ImVec4(1, 1, 0, 1), "Important Stuff");
	ImGui::BeginChild("Scrolling");
	for (int n = 0; n < 50; n++)
		ImGui::Text("%04d: Some text", n);
	ImGui::EndChild();
	ImGui::End();
}

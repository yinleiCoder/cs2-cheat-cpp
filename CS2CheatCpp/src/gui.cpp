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
		return true;
	}

	switch (message)
	{
		case WM_SYSCOMMAND:
		{
			if ((w_param & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
				return 0;
			break;
		}
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0L;
		}
		default:
			return DefWindowProc(window, message, w_param, l_param);
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
	ZeroMemory(&sd, sizeof(sd));
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
	if (render_target_view) {
		render_target_view->Release();
		render_target_view = nullptr;
	}
	if (swap_chain) {
		swap_chain->Release();
		swap_chain = nullptr;
	}
	if (device_context) {
		device_context->Release();
		device_context = nullptr;
	}
	if (device) {
		device->Release();
		device = nullptr;
	}
}

void gui::CreateImGui() noexcept
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

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
	if (GetAsyncKeyState(VK_INSERT) & 1) {
		menutoggle = !menutoggle;
	}

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
	ImGui::EndFrame();
	ImGui::Render();

	constexpr float clear_color_with_alpha[4]{ 0.f, 0.f, 0.f, 0.f };
	device_context->OMSetRenderTargets(1U, &render_target_view, nullptr);
	device_context->ClearRenderTargetView(render_target_view, clear_color_with_alpha);

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	// 开启垂直同步
	swap_chain->Present(1U, 0U);
}

void gui::Render() noexcept
{
	static bool no_titlebar = false;
	static bool no_scrollbar = false;
	static bool no_menu = false;
	static bool no_move = false;
	static bool no_resize = false;
	static bool no_collapse = false;
	static bool no_nav = false;
	static bool no_background = false;
	static bool no_bring_to_front = false;
	static bool unsaved_document = false;
	ImGuiWindowFlags window_flags = 0;
	if (no_titlebar)        window_flags |= ImGuiWindowFlags_NoTitleBar;
	if (no_scrollbar)       window_flags |= ImGuiWindowFlags_NoScrollbar;
	if (!no_menu)           window_flags |= ImGuiWindowFlags_MenuBar;
	if (no_move)            window_flags |= ImGuiWindowFlags_NoMove;
	if (no_resize)          window_flags |= ImGuiWindowFlags_NoResize;
	if (no_collapse)        window_flags |= ImGuiWindowFlags_NoCollapse;
	if (no_nav)             window_flags |= ImGuiWindowFlags_NoNav;
	if (no_background)      window_flags |= ImGuiWindowFlags_NoBackground;
	if (no_bring_to_front)  window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	if (unsaved_document)   window_flags |= ImGuiWindowFlags_UnsavedDocument;
	bool show_demo_window = true;

	if (menutoggle) {
		ImGui::ShowDemoWindow(&show_demo_window);
		ImGui::SetNextWindowSize({ 1280.f,720.f }, ImGuiCond_FirstUseEver);
		ImGui::Begin("CS2 Cheat with C++", 0, window_flags);
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Menu"))
			{
				ImGui::MenuItem("Exit Cheat", NULL, &exit);
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		ImGui::Text("Tip: ImGui (%s) (%d)", IMGUI_VERSION, IMGUI_VERSION_NUM);
		ImGui::Spacing();
	
		if (ImGui::CollapsingHeader("Help"))
		{
			ImGui::SeparatorText("Usage:");
			ImGui::TextWrapped("Show and hide menus by pressing");
			ImGui::TextColored(ImVec4(1.0f, 0.f, 0.0f, 1.0f), " the INSERT key ");
			ImGui::TextWrapped("Download the zip file and unzip it to your computer, enter the unzipped folder, first open the CS2 game and enter a room match, then double-click CS2CheatCpp.exe to run the plug-in.");

			ImGui::SeparatorText("Github Homepage:");
			ImGui::TextWrapped("https://github.com/yinleiCoder/cs2-cheat-cpp");

			ImGui::SeparatorText("Software Download:");
			ImGui::TextWrapped("https://github.com/yinleiCoder/cs2-cheat-cpp/releases");
		}
		ImGui::Checkbox("Player box esp", &boxEsp);
		ImGui::Checkbox("Player body glow", &playerBodyGlow);
		ImGui::Checkbox("Player remaining health esp", &playerHealth);
		ImGui::Checkbox("Aimbot and headlock shot", &aimbot);
		ImGui::SetItemTooltip("please enter E keyboard");
		ImGui::Checkbox("Anti-rcs", &rcs);
		ImGui::Checkbox("Radar", &radar);
		ImGui::Checkbox("Anti-flash", &flash);
		ImGui::Checkbox("Bhop", &bhop);
		ImGui::SetItemTooltip("please keep pressing the space bar");

		if (ImGui::CollapsingHeader("Window options"))
		{
			if (ImGui::BeginTable("split", 3))
			{
				ImGui::TableNextColumn(); ImGui::Checkbox("No titlebar", &no_titlebar);
				ImGui::TableNextColumn(); ImGui::Checkbox("No scrollbar", &no_scrollbar);
				ImGui::TableNextColumn(); ImGui::Checkbox("No menu", &no_menu);
				ImGui::TableNextColumn(); ImGui::Checkbox("No move", &no_move);
				ImGui::TableNextColumn(); ImGui::Checkbox("No resize", &no_resize);
				ImGui::TableNextColumn(); ImGui::Checkbox("No collapse", &no_collapse);
				ImGui::TableNextColumn(); ImGui::Checkbox("No nav", &no_nav);
				ImGui::TableNextColumn(); ImGui::Checkbox("No background", &no_background);
				ImGui::TableNextColumn(); ImGui::Checkbox("No bring to front", &no_bring_to_front);
				ImGui::TableNextColumn(); ImGui::Checkbox("Unsaved document", &unsaved_document);
				ImGui::EndTable();
			}
		}
		
		ImGui::End();
		SetWindowLong(overlay, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_LAYERED);
	}
	else {
		SetWindowLong(overlay, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED );
	}
}

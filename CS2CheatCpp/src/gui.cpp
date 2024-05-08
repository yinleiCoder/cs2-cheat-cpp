#define _CRT_SECURE_NO_WARNINGS

#include <corecrt_math.h>
#include "gui.h"
#include "../dependencies/ImGui/imgui.h"
#include "../dependencies/ImGui/imgui_impl_dx11.h"
#include "../dependencies/ImGui/imgui_impl_win32.h"

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
	windowClass.lpszClassName = reinterpret_cast<LPCWSTR>(className);

	RegisterClassExW(&windowClass);

	overlay = CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
		windowClass.lpszClassName,
		reinterpret_cast<LPCWSTR>(windowName),
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
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 16.f, nullptr, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(overlay);
	ImGui_ImplDX11_Init(device, device_context);

	ImGuiStyle& style = ImGui::GetStyle();
	style.Alpha = 1.0;
	style.WindowRounding = 3;
	style.GrabRounding = 1;
	style.GrabMinSize = 20;
	style.FrameRounding = 3;

	style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.00f, 0.40f, 0.41f, 1.00f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 1.00f, 1.00f, 0.65f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.44f, 0.80f, 0.80f, 0.18f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.44f, 0.80f, 0.80f, 0.27f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.44f, 0.81f, 0.86f, 0.66f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.14f, 0.18f, 0.21f, 0.73f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.54f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.27f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.20f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.22f, 0.29f, 0.30f, 0.71f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.00f, 1.00f, 1.00f, 0.44f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.00f, 1.00f, 1.00f, 0.74f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.00f, 1.00f, 1.00f, 0.68f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.00f, 1.00f, 1.00f, 0.36f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.76f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.00f, 0.65f, 0.65f, 0.46f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.01f, 1.00f, 1.00f, 0.43f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.62f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.00f, 1.00f, 1.00f, 0.33f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.00f, 1.00f, 1.00f, 0.42f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 1.00f, 1.00f, 0.54f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 1.00f, 1.00f, 0.54f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.00f, 1.00f, 1.00f, 0.74f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.00f, 1.00f, 1.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 1.00f, 1.00f, 0.22f);
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
		menuToggle = !menuToggle;
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
	// 水印
	auto start = std::chrono::system_clock::now();
	auto end = std::chrono::system_clock::now();
	auto elasped_seconds = end - start;
	auto end_time = std::chrono::system_clock::to_time_t(end);
	ImGui::SetNextWindowPos({ 10, 10 });
	ImGui::SetWindowSize(ImVec2(296, 50));
	ImGui::Begin("watermark", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
	ImGui::Text("yinlei | %s", std::ctime(&end_time));
	ImGui::End();

	if (menuToggle) {
		// 更新当前玩家最大速度
		if (speed > maxSpeed) {
			maxSpeed = speed;
		}
		/*static bool show_demo_window = true;
		ImGui::ShowDemoWindow(&show_demo_window);*/
		ImGuiWindowFlags window_flags = 0;
		window_flags |= ImGuiWindowFlags_MenuBar;
		window_flags |= ImGuiWindowFlags_NoCollapse;
		ImGui::SetNextWindowSize(ImVec2(900, 450), ImGuiCond_FirstUseEver);
		ImGui::Begin("CS2 ESP Cheat", 0, window_flags);
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Menu"))
			{
				ImGui::MenuItem(("Quit"), NULL, &exit);
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Insert key: Show/Hide"))
			{
				ImGui::MenuItem(("Show"), NULL, &menuToggle);
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		ImGui::Text("Runtime Environment: ImGui %s %d", IMGUI_VERSION, IMGUI_VERSION_NUM);
		ImGui::Spacing();
		ImGui::Text("Screen Configuration(default fullscreen): width %d px, height %d px", screenWidth, screenHeight);
		ImGui::Spacing();
	
		if (ImGui::BeginTabBar("CS2 ESP Cheat"))
		{
			if (ImGui::BeginTabItem("Usage"))
			{
				ImGui::TextWrapped("1.Open the 'CS2' game and enter a game room.");
				ImGui::TextWrapped("2.Once in the game room, double-click 'CS2CheatCpp.exe' to run the cheat program.");
				ImGui::TextWrapped("3.After running the cheat program, the cheat program menu will appear. You can press the 'Insert'' key to show / hide it.");
				ImGui::TextWrapped("4.If the cheat program menu doesn't appear, please submit a log file. Most likely, the offsets are out of sync with the official CS2 updates. You can clone the code and build it yourself.");
				ImGui::TextWrapped("5.After each game, it's advisable to exit the cheat program first. Then, repeat the above steps when entering the game room again. This is because handle hijacking sometimes isn't very effective, at least in my experience, I've never been banned.");
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("ESP Cheat"))
			{
				ImGui::Columns(2);
				ImGui::Checkbox("Team mode", &enableTeamMode);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("If you're currently in team mode, please check the box.");
				}
				ImGui::Checkbox("Box perspective", &enableBoxEsp);
				ImGui::Checkbox("Bone perspective", &enableBoneEsp);
				ImGui::Checkbox("Radar", &enableRadar);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("This option will display a radar on the map showing all enemies.");
				}
				ImGui::Checkbox("RCS (Recoil Control System) ", &enableRcs);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("This option will compensate for recoil in the first few shots. The algorithm needs improvement.");
				}
				ImGui::Checkbox("Automatic firing", &enableAutoAttack);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("This option will automatically fire within the field of view.");
				}
				ImGui::Checkbox("Auto-aim (aimbot)", &enableAimbot);
				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("This option will lock onto the nearest enemy's head.");
				}
				ImGui::SliderFloat("Fov Aimbot Pixel", &fovAimbot, 10.0f, 300.0f, "value = %.2f");
				if (ImGui::CollapsingHeader("Fov aimbot pixel color"))
				{
					ImGui::ColorEdit4("fov circle color", fovAimbotColor);
				}

				ImGui::NextColumn();
				ImGui::Checkbox("Remaining health", &enableHealth);
				ImGui::Checkbox("Weapon", &enableWeapon);
				ImGui::Checkbox("Anti-flash", &enableFlash);
				ImGui::Checkbox("Bunny hop", &enableBhop);
				ImGui::SliderInt("fov (Field of view)", &fov, 0, 180);
				ImGui::Text("Current movement speed: %d", speed);
				ImGui::Text("Maximum movement speed: %d", maxSpeed);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Misc"))
			{
				ImGui::SeparatorText("Github Homepage");
				ImGui::TextWrapped("https://github.com/yinleiCoder/cs2-cheat-cpp");

				ImGui::SeparatorText("Software Download");
				ImGui::TextWrapped("https://github.com/yinleiCoder/cs2-cheat-cpp/releases");
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		ImGui::End();
		SetWindowLong(overlay, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_LAYERED);
	}
	else {
		SetWindowLong(overlay, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED );
	}
}

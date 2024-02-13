#include <thread>
#include "gui.h"
#include "memory/memory.h"
#include "vector.h"
#include "render.h"
#include "bone.hpp"

namespace offsets {
	// offsets.hpp
	constexpr std::ptrdiff_t dwLocalPlayerPawn = 0x17272E8;
	constexpr std::ptrdiff_t dwEntityList = 0x18B1FE8;
	constexpr std::ptrdiff_t dwViewMatrix = 0x19112D0;
	// client.dll.hpp 
	constexpr std::ptrdiff_t m_iHealth = 0x334; // int32_t
	constexpr std::ptrdiff_t m_hPlayerPawn = 0x7E4; // CHandle<C_CSPlayerPawn>
	constexpr std::ptrdiff_t m_iTeamNum = 0x3CB; // uint8_t
	constexpr std::ptrdiff_t m_vOldOrigin = 0x127C; // Vector
}

INT APIENTRY WinMain(HINSTANCE instance, HINSTANCE, PSTR, INT cmd_show)
{
	auto mem = Memory("cs2.exe");
	const auto client = mem.GetModuleAddress("client.dll");

	gui::CreateHWindow("csgo", "yinlei", instance, cmd_show);
	gui::CreateDevice();
	gui::CreateImGui();

	while (gui::exit) 
	{
		if (!gui::exit) {
			break;
		}
		gui::BeginRender();
		gui::Render();

		// 处理外挂业务逻辑: 获取相关数据的地址
		uintptr_t localPlayer = mem.Read<uintptr_t>(client+offsets::dwLocalPlayerPawn);
		Vector3 localOrigin = mem.Read<Vector3>(localPlayer+offsets::m_vOldOrigin);
		view_matrix_t view_matrix = mem.Read<view_matrix_t>(client+offsets::dwViewMatrix);
		uintptr_t enity_list = mem.Read<uintptr_t>(client+offsets::dwEntityList);
		int localTeam = mem.Read<int>(localPlayer+offsets::m_iTeamNum);

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

			if (Vector3::word_to_screen(view_matrix, origin, screenPos) &&
				Vector3::word_to_screen(view_matrix, head, screenHead) &&
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

		gui::EndRender();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	gui::DestroyImGui();
	gui::DestroyDevice();
	gui::DestroyHWindow();

	return 0;
}
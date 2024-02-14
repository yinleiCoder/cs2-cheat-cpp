#include <thread>
#include <vector>
#include <string>
#include <functional>
#include <algorithm>
#include <iostream>
#include "gui.h"
#include "memory/memory.h"
#include "vector.h"
#include "render.h"
#include "bone.hpp"
#include "entity.h"

namespace offsets {
	// offsets.hpp
	constexpr std::ptrdiff_t dwLocalPlayerPawn = 0x17272E8;
	constexpr std::ptrdiff_t dwEntityList = 0x18B1FE8;
	constexpr std::ptrdiff_t dwViewAngles = 0x191F100;
	constexpr std::ptrdiff_t dwViewMatrix = 0x19112D0;
	constexpr std::ptrdiff_t dwForceJump = 0x1720670;
	constexpr std::ptrdiff_t dwForceAttack = 0x1720160;

	// client.dll.hpp 
	constexpr std::ptrdiff_t m_iHealth = 0x334; // int32_t
	constexpr std::ptrdiff_t m_hPlayerPawn = 0x7E4; // CHandle<C_CSPlayerPawn>
	constexpr std::ptrdiff_t m_iTeamNum = 0x3CB; // uint8_t
	constexpr std::ptrdiff_t m_vOldOrigin = 0x127C; // Vector
	constexpr std::ptrdiff_t m_flFlashBangTime = 0x14B8; // float
	constexpr std::ptrdiff_t m_fFlags = 0x3D4; // uint32_t
	constexpr std::ptrdiff_t m_flDetectedByEnemySensorTime = 0x1440; // GameTime_t
	constexpr std::ptrdiff_t m_iszPlayerName = 0x638; // char[128]
	constexpr std::ptrdiff_t m_entitySpottedState = 0x1698; // EntitySpottedState_t C_CSPlayerPawnBase 
	constexpr std::ptrdiff_t m_bSpotted = 0x8; // bool
	constexpr std::ptrdiff_t m_iIDEntIndex = 0x15A4; // CEntityIndex
	constexpr std::ptrdiff_t m_vecViewOffset = 0xC58; // CNetworkViewOffsetVector
	constexpr std::ptrdiff_t m_lifeState = 0x338; // uint8_t
	constexpr std::ptrdiff_t m_pGameSceneNode = 0x318; // CGameSceneNode*
	constexpr std::ptrdiff_t m_modelState = 0x160; // CModelState
}

INT APIENTRY WinMain(HINSTANCE instance, HINSTANCE, PSTR, INT cmd_show)
{
	auto mem = Memory("cs2.exe");
	const auto client = mem.GetModuleAddress("client.dll");

	const unsigned int STANDING = 65665;// 站立
	const unsigned int CROUCHING = 65667;// 蹲伏
	const unsigned int PLUS_JUMP = 65537;// +jump
	const unsigned int MINUS_JUMP = 256;// -jump
	const unsigned int PLUS_ATTACK = 65537;// +attack
	const unsigned int MINUS_ATTACK = 256;// -attack

	gui::CreateHWindow("csgo", "yinlei", instance, cmd_show);
	gui::CreateDevice();
	gui::CreateImGui();

	Entity localPlayer;
	std::vector<Entity> entities;

	while (gui::exit) 
	{
		if (!gui::exit) {
			break;
		}

		entities.clear();

		gui::BeginRender();
		gui::Render();

		// 处理外挂业务逻辑: 获取相关数据的地址
		localPlayer.pawnAddress = mem.Read<uintptr_t>(client+offsets::dwLocalPlayerPawn);
		localPlayer.origin = mem.Read<Vector3>(localPlayer.pawnAddress +offsets::m_vOldOrigin);
		localPlayer.viewOffset = mem.Read<Vector3>(localPlayer.pawnAddress +offsets::m_vecViewOffset);
		localPlayer.team = mem.Read<int>(localPlayer.pawnAddress +offsets::m_iTeamNum);
		localPlayer.flashDuration = mem.Read<float>(localPlayer.pawnAddress +offsets::m_flFlashBangTime);
		localPlayer.fFlag = mem.Read<unsigned int>(localPlayer.pawnAddress +offsets::m_fFlags);
		localPlayer.entIndex = mem.Read<int>(localPlayer.pawnAddress +offsets::m_iIDEntIndex);// 十字准星前的玩家id
		view_matrix_t view_matrix = mem.Read<view_matrix_t>(client+offsets::dwViewMatrix);
		const auto enity_list = mem.Read<uintptr_t>(client+offsets::dwEntityList);

		// 防闪光弹
		if (localPlayer.flashDuration > 0)
		{
			mem.Write<float>(localPlayer.pawnAddress + offsets::m_flFlashBangTime, 0);
		}

		// 兔子连跳
		if (GetAsyncKeyState(VK_SPACE) & 0x01)
		{
			if (localPlayer.fFlag == STANDING || localPlayer.fFlag == CROUCHING)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				mem.Write<unsigned int>(client+offsets::dwForceJump, PLUS_JUMP);
			}
			else
			{
				mem.Write<unsigned int>(client + offsets::dwForceJump, MINUS_JUMP);
			}
		}

		// 处理外挂业务逻辑: 根据entityList找到currentPawn、currentController获取相关玩家信息，currentController下有pawnHandle，通过pawnHandle得到currentPawn
		for (int playerIndex = 1; playerIndex < 64; ++playerIndex)
		{
			const auto list_entry = mem.Read<uintptr_t>(enity_list +(8*(playerIndex & 0x7FFF) >> 9)+16);// 16==0x10
			if (!list_entry) {
				continue;
			}
			// 得到currentController
			const auto currentController = mem.Read<uintptr_t>(list_entry + 120 * (playerIndex & 0x1FF));// 120==0x78
			if (!currentController) {
				continue;
			}
			// 通过currentController获取团队编号等信息
			int team = mem.Read<int>(currentController+offsets::m_iTeamNum);
			const auto playerName = mem.ReadString<128>(currentController + offsets::m_iszPlayerName);

			if (team == localPlayer.team) {
				continue; // 队友
			}

			// 通过currentController得到pawnHandle
			const auto playerPawnHandle = mem.Read<uint32_t>(currentController+offsets::m_hPlayerPawn);
			// 通过pawnHandle得到currentPawn
			const auto list_entry2 = mem.Read<uintptr_t>(enity_list + 0x8 * ((playerPawnHandle & 0x7FFF) >> 9) + 16);// 16==0x10
			if (!list_entry2) {
				continue;
			}
			// 拿到了currentPawn
			const auto currentPawn = mem.Read<uintptr_t>(list_entry2 + 120 * (playerPawnHandle & 0x1FF));// 120==0x78
			if (!currentPawn || currentPawn == localPlayer.pawnAddress) {
				continue;
			}
			// currentPawn有生命值等信息,而玩家姓名在currentController下
			int health = mem.Read<int>(currentPawn+offsets::m_iHealth);
			const auto lifeState = mem.Read<unsigned int>(currentPawn+offsets::m_lifeState);
			if (health <= 0 || health > 100 || lifeState != 256) {
				continue;
			}
			// 获取雷达状态并设置敌人显示在雷达上
			bool spotted = mem.Read<bool>(currentController + offsets::m_entitySpottedState + offsets::m_bSpotted);
			mem.Write<bool>(currentController + offsets::m_entitySpottedState + offsets::m_bSpotted, true);
			// 玩家发光
			mem.Write<float>(currentPawn + offsets::m_flDetectedByEnemySensorTime, 86400);
			
			Entity entity;
			entity.name = playerName;
			entity.pawnAddress = currentPawn;
			entity.controllerAddress = currentController;
			entity.health = health;
			entity.lifeState = lifeState;
			entity.team = team;
			entity.origin = mem.Read<Vector3>(currentPawn+offsets::m_vOldOrigin);
			entity.viewOffset = mem.Read<Vector3>(currentPawn+offsets::m_vecViewOffset);
			entity.distance = Vector3::distance(entity.origin, localPlayer.origin);

			// 骨骼绘制
			// 获取玩家的头坐标实现锁头
			const auto sceneNode = mem.Read<uintptr_t>(currentPawn + offsets::m_pGameSceneNode);
			const auto boneMatrix = mem.Read<uintptr_t>(sceneNode + offsets::m_modelState + 0x80);
			//Vector3 head = { entity.origin.x, entity.origin.y, entity.origin.z + 75.f };
			entity.head = mem.Read<Vector3>(boneMatrix + bones::head * 32);

			entities.push_back(entity);

			Vector3 screenPos;
			Vector3 screenHead;
			float headHeight = (screenPos.y - screenHead.y) / 8;

			RGB enemy = { 255, 0, 0 };
			RGB bone = { 255, 255, 255 };
			RGB hp = { 0, 255, 0 };

			// 如果不是CS2游戏在前台就不绘制
			if (!mem.InForeground()) {
				continue;
			}

			if (Vector3::word_to_screen(view_matrix, entity.origin, screenPos) &&
				Vector3::word_to_screen(view_matrix, entity.head, screenHead) &&
				entity.origin.x != 0) {
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

		std::stable_sort(entities.begin(), entities.end(), [](const Entity& entity1, const Entity& entity2) {
			return entity1.distance < entity2.distance;
		});

		// 自瞄锁头并开枪
		if (entities.size() > 0 && (GetAsyncKeyState('E') & 0x8000))
		{
			Vector3 playerView = localPlayer.origin + localPlayer.viewOffset;
			Vector3 entityView = entities[0].origin + entities[0].viewOffset;
			Vector3 newAngles = Vector3::angles(playerView, entities[0].head);
			Vector3 newAnglesVec3{newAngles.y, newAngles.x, 0.0f};
			mem.Write<Vector3>(client+offsets::dwViewAngles, newAnglesVec3);

			if (localPlayer.entIndex > 0)
			{
				mem.Write<int>(client + offsets::dwForceAttack, PLUS_ATTACK);
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				mem.Write<int>(client + offsets::dwForceAttack, MINUS_ATTACK);
			}
		}

		gui::EndRender();
		std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}

	gui::DestroyImGui();
	gui::DestroyDevice();
	gui::DestroyHWindow();

	return 0;
}
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
	constexpr std::ptrdiff_t dwLocalPlayerPawn = 0x1730118;
	constexpr std::ptrdiff_t dwEntityList = 0x18BBAF8;
	constexpr std::ptrdiff_t dwViewAngles = 0x1929730;
	constexpr std::ptrdiff_t dwViewMatrix = 0x191CF30;
	constexpr std::ptrdiff_t dwForceJump = 0x17294A0;
	constexpr std::ptrdiff_t dwForceAttack = 0x1728F90;
	constexpr std::ptrdiff_t dwGameRules = 0x1918A30;
	constexpr std::ptrdiff_t dwSensitivity = 0x1919778;
	constexpr std::ptrdiff_t dwSensitivity_sensitivity = 0x40;

	// client.dll.hpp 
	constexpr std::ptrdiff_t m_iHealth = 0x334; // int32_t
	constexpr std::ptrdiff_t m_hPlayerPawn = 0x7E4; // CHandle<C_CSPlayerPawn>
	constexpr std::ptrdiff_t m_iTeamNum = 0x3CB; // uint8_t
	constexpr std::ptrdiff_t m_vOldOrigin = 0x127C; // Vector
	constexpr std::ptrdiff_t m_flFlashBangTime = 0x14B8; // float C_CSPlayerPawnBase { // C_BasePlayerPawn
	constexpr std::ptrdiff_t m_fFlags = 0x3D4; // uint32_t C_BaseEntity 
	constexpr std::ptrdiff_t m_flDetectedByEnemySensorTime = 0x1440; // GameTime_t
	constexpr std::ptrdiff_t m_iszPlayerName = 0x638; // char[128]
	constexpr std::ptrdiff_t m_entitySpottedState = 0x1698; // EntitySpottedState_t C_CSPlayerPawnBase 
	constexpr std::ptrdiff_t m_bSpotted = 0x8; // bool
	constexpr std::ptrdiff_t m_iIDEntIndex = 0x15A4; // CEntityIndex
	constexpr std::ptrdiff_t m_vecViewOffset = 0xC58; // CNetworkViewOffsetVector
	constexpr std::ptrdiff_t m_lifeState = 0x338; // uint8_t
	constexpr std::ptrdiff_t m_pGameSceneNode = 0x318; // CGameSceneNode*
	constexpr std::ptrdiff_t m_modelState = 0x160; // CModelState
	constexpr std::ptrdiff_t m_aimPunchAngle = 0x177C; // QAngle  C_CSPlayerPawn { // C_CSPlayerPawnBase
	constexpr std::ptrdiff_t m_angEyeAngles = 0x1578; // QAngle
	constexpr std::ptrdiff_t m_aimPunchCache = 0x17A0; // CUtlVector<QAngle>
	constexpr std::ptrdiff_t m_iShotsFired = 0x147C; // int32_t C_CSPlayerPawnBase { // C_BasePlayerPawn
	constexpr std::ptrdiff_t m_pCameraServices = 0x1138; // CPlayer_CameraServices*
	constexpr std::ptrdiff_t m_iFOV = 0x210; // uint32_t
	constexpr std::ptrdiff_t m_bIsScoped = 0x1400; // bool
	constexpr std::ptrdiff_t m_vecAbsVelocity = 0x3D8; // Vector
	constexpr std::ptrdiff_t m_bBombPlanted = 0x9DD; // bool C_CSGameRules 
}

struct C_UTL_VECTOR
{
	DWORD_PTR count = 0;
	DWORD_PTR data = 0;
};

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
	entities.reserve(64);
	bool bombPlanted = false;
	static auto oldPunch = Vector3{};

	while (gui::exit) 
	{
		entities.clear();

		if (!gui::exit) {
			break;
		}

		gui::BeginRender();
		gui::Render();

		// 处理外挂业务逻辑: 获取相关数据的地址
		localPlayer.pawnAddress = mem.Read<uintptr_t>(client + offsets::dwLocalPlayerPawn);
		localPlayer.origin = mem.Read<Vector3>(localPlayer.pawnAddress + offsets::m_vOldOrigin);
		localPlayer.viewOffset = mem.Read<Vector3>(localPlayer.pawnAddress + offsets::m_vecViewOffset);
		localPlayer.team = mem.Read<int>(localPlayer.pawnAddress + offsets::m_iTeamNum);
		localPlayer.entIndex = mem.Read<int>(localPlayer.pawnAddress + offsets::m_iIDEntIndex);// 准星前的玩家id
		localPlayer.fFlag = mem.Read<unsigned int>(localPlayer.pawnAddress + offsets::m_fFlags);// 玩家的fFlag
		localPlayer.flashDuration = mem.Read<float>(localPlayer.pawnAddress + offsets::m_flFlashBangTime);//玩家遭受闪光的时间
		localPlayer.velocity = mem.Read<Vector3>(localPlayer.pawnAddress + offsets::m_vecAbsVelocity);// 玩家移动速度

		const auto enity_list = mem.Read<uintptr_t>(client + offsets::dwEntityList);
		view_matrix_t view_matrix = mem.Read<view_matrix_t>(client + offsets::dwViewMatrix);
		uintptr_t gameRules = mem.Read<uintptr_t>(client + offsets::dwGameRules);

		// c4炸弹倒计时
		if (gameRules) 
		{
			bombPlanted = mem.Read<bool>(gameRules + offsets::m_bBombPlanted);
			if (bombPlanted)
			{
				gui::bombPlanted = true;
				/*for (int i = 0;i < 40;i++)
				{
					bombPlanted = mem.Read<bool>(gameRules + offsets::m_bBombPlanted);
					if (!bombPlanted) 
					{
						break;
					}
					gui::bombTimeLeft = 40 - i;
					gui::bombPlanted = true;

					std::this_thread::sleep_for(std::chrono::seconds(1));
				}*/
			}
			else
			{
				gui::bombTimeLeft = -1;
				gui::bombPlanted = false;
			}
		}

		// 处理外挂业务逻辑: 根据entityList找到currentPawn、currentController获取相关玩家信息，currentController下有pawnHandle，通过pawnHandle得到currentPawn
		for (int playerIndex = 1; playerIndex < 64; ++playerIndex)
		{
			// 如果不是CS2游戏在前台就不绘制
			if (!mem.InForeground()) {
				continue;
			}

			// 根据entity_list获取第一个入口点
			const auto list_entry = mem.Read<uintptr_t>(enity_list + (8 * (playerIndex & 0x7FFF) >> 9) + 16);// 16==0x10
			if (!list_entry) {
				continue;
			}

			// 获取currentController
			const auto currentController = mem.Read<uintptr_t>(list_entry + 120 * (playerIndex & 0x1FF));// 120==0x78
			if (!currentController) {
				continue;
			}

			// 通过currentController获取pawnHandle
			const auto playerPawnHandle = mem.Read<uint32_t>(currentController + offsets::m_hPlayerPawn);

			// 通过pawnHandle和entityList获取第二个入口点和currentPawn
			const auto list_entry2 = mem.Read<uintptr_t>(enity_list + 0x8 * ((playerPawnHandle & 0x7FFF) >> 9) + 16);// 16==0x10
			if (!list_entry2) {
				continue;
			}
			const auto currentPawn = mem.Read<uintptr_t>(list_entry2 + 120 * (playerPawnHandle & 0x1FF));// 120==0x78
			// 扫描到"我"就排除我的信息
			if (!currentPawn || currentPawn == localPlayer.pawnAddress) {
				continue;
			}

			// 通过currentController获取团队编号、姓名等信息
			int team = mem.Read<int>(currentController + offsets::m_iTeamNum);
			// 猪队友
			if (team == localPlayer.team) {
				continue;
			}
			// 玩家的真实姓名
			const auto playerName = mem.ReadString<128>(currentController + offsets::m_iszPlayerName);

			// currentPawn有生命值等信息,而玩家姓名在currentController下
			int health = mem.Read<int>(currentPawn + offsets::m_iHealth);
			const auto lifeState = mem.Read<unsigned int>(currentPawn + offsets::m_lifeState);
			if (health <= 0 || health > 100 || lifeState != 256) {
				continue;
			}

			// 获取雷达状态并设置敌人显示在雷达上
			if (gui::radar) {
				bool spotted = mem.Read<bool>(currentPawn + offsets::m_entitySpottedState + offsets::m_bSpotted);
				mem.Write<bool>(currentPawn + offsets::m_entitySpottedState + offsets::m_bSpotted, true);
			}

			// 玩家身体发光
			if (gui::playerBodyGlow) {
				mem.Write<float>(currentPawn + offsets::m_flDetectedByEnemySensorTime, 86400);
			}
			
			Entity entity;
			entity.pawnAddress = currentPawn;
			entity.controllerAddress = currentController;
			entity.name = playerName;
			entity.health = health;
			entity.lifeState = lifeState;
			entity.team = team;
			entity.spotted = mem.Read<bool>(currentPawn + offsets::m_entitySpottedState + offsets::m_bSpotted);
			entity.origin = mem.Read<Vector3>(currentPawn+offsets::m_vOldOrigin);
			entity.viewOffset = mem.Read<Vector3>(currentPawn+offsets::m_vecViewOffset);
			entity.distance = Vector3::distance(entity.origin, localPlayer.origin);

			// 获取玩家的头坐标实现锁头
			const auto sceneNode = mem.Read<uintptr_t>(currentPawn + offsets::m_pGameSceneNode);
			// 获取骨骼信息绘制玩家骨骼
			const auto boneMatrix = mem.Read<uintptr_t>(sceneNode + offsets::m_modelState + 0x80);
			//Vector3 head = { entity.origin.x, entity.origin.y, entity.origin.z + 75.f };
			entity.head = mem.Read<Vector3>(boneMatrix + bones::head * 32);

			// 将收集好的所有玩家信息存储起来
			entities.push_back(entity);

			// 准备绘制需要的数据
			Vector3 screenPos;
			Vector3 screenHead;

			RGB enemy = { 255, 0, 0 };
			RGB hp = { 0, 255, 0 };
			//RGB bone = { 255, 255, 255 };

			if (Vector3::word_to_screen(view_matrix, entity.origin, screenPos) &&
				Vector3::word_to_screen(view_matrix, entity.head, screenHead) &&
				entity.origin.x != 0) {
				float height = screenPos.y - screenHead.y;
				//float headHeight = height / 8;
				float width = height / 2.4f;

				if (gui::boxEsp) {
					Render::DrawRect(
						screenHead.x - width / 2,
						screenHead.y,
						width,
						height,
						enemy,
						1.5,
						false,
						255
					);
				}

				if (gui::playerHealth) {
					Render::DrawRect(
						screenHead.x - (width / 2 + 10),
						screenHead.y + (height * (100 - health) / 100),
						3,
						height - (height * (100 - health) / 100),
						hp,
						1.5,
						true,
						255
					);
				}
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

		// 自瞄锁头并开枪
		if (gui::aimbot && entities.size() > 0 && !gui::radar && entities[0].spotted)
		{
			// 扫描距离我最近的敌人
			std::stable_sort(entities.begin(), entities.end(), [](const Entity& entity1, const Entity& entity2) {
				return entity1.distance < entity2.distance;
				});

			// 计算自瞄需要偏移的角度
			Vector3 playerView = localPlayer.origin + localPlayer.viewOffset;
			//Vector3 entityView = entities[0].origin + entities[0].viewOffset;
			Vector3 newAngles = Vector3::angles(playerView, entities[0].head);
			Vector3 newAnglesVec3{newAngles.y, newAngles.x, 0.0f};
			mem.Write<Vector3>(client + offsets::dwViewAngles, newAnglesVec3);

			// 开枪
			if (gui::autoAttack && localPlayer.entIndex > 0)
			{
				mem.Write<int>(client + offsets::dwForceAttack, PLUS_ATTACK);
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				mem.Write<int>(client + offsets::dwForceAttack, MINUS_ATTACK);
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}

		// 防闪光弹
		if (gui::flash && localPlayer.flashDuration > 0)
		{
			mem.Write<float>(localPlayer.pawnAddress + offsets::m_flFlashBangTime, 0);
		}

		// 连跳
		if (gui::bhop && GetAsyncKeyState(VK_SPACE) & 0x01)
		{
			if (localPlayer.fFlag == STANDING || localPlayer.fFlag == CROUCHING)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				mem.Write<unsigned int>(client + offsets::dwForceJump, PLUS_JUMP);
			}
			else
			{
				mem.Write<unsigned int>(client + offsets::dwForceJump, MINUS_JUMP);
			}
		}

		// fov视野角度(相机Service)
		auto desiredFov = static_cast<unsigned int>(gui::fov);
		localPlayer.cameraServices = mem.Read<uintptr_t>(localPlayer.pawnAddress + offsets::m_pCameraServices);
		unsigned int currentFov = mem.Read<unsigned int>(localPlayer.cameraServices + offsets::m_iFOV);
		bool isScoped = mem.Read<bool>(localPlayer.pawnAddress + offsets::m_bIsScoped);
		if (!isScoped && currentFov != desiredFov)
		{
			mem.Write<unsigned int>(localPlayer.cameraServices + offsets::m_iFOV, desiredFov);
		}

		// 玩家暴走
		gui::speed = std::sqrt(localPlayer.velocity.x * localPlayer.velocity.x + localPlayer.velocity.y * localPlayer.velocity.y + localPlayer.velocity.z * localPlayer.velocity.z);

		// 后坐力补偿
		if (gui::rcs) {
			const auto shotsFired = mem.Read<int32_t>(localPlayer.pawnAddress + offsets::m_iShotsFired);// 开枪次数
			auto sensPointer = mem.Read<uintptr_t>(client + offsets::dwSensitivity);
			auto sensitivity = mem.Read<float>(sensPointer + offsets::dwSensitivity_sensitivity);
			auto aimPunchCache = mem.Read<C_UTL_VECTOR>(localPlayer.pawnAddress + offsets::m_aimPunchCache);
			if (aimPunchCache.data && aimPunchCache.count > 0 && aimPunchCache.count < 0xFFFF) {
				localPlayer.aimPunch = mem.Read<Vector3>(aimPunchCache.data + (aimPunchCache.count - 1) * sizeof(Vector3));
			}
			else {
				continue;
			}
			if (shotsFired > 1) {// 如果我们开枪了，就计算后坐力补偿
				Vector3 viewAngles = mem.Read<Vector3>(client+offsets::dwViewAngles);
				Vector3 delta = viewAngles - (viewAngles + (oldPunch - (localPlayer.aimPunch * 2.0f)));

				int mouse_angle_x = (int)(delta.x / (sensitivity*0.022f));
				int mouse_angle_y = (int)(delta.y / (sensitivity*0.022f));
				mouse_event(MOUSEEVENTF_MOVE, mouse_angle_x, -mouse_angle_y, 0, 0);
				oldPunch = localPlayer.aimPunch * 2.0f;
			}
			else {
				oldPunch.x = oldPunch.y = oldPunch.z = 0.f;
			}
		}

		// imgui渲染工作
		gui::EndRender();
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}

	gui::DestroyImGui();
	gui::DestroyDevice();
	gui::DestroyHWindow();

	return 0;
}
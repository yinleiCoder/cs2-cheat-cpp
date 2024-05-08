#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <functional>
#include <thread>
#include <chrono>

#include "memory/memory.h"
#include "gui.h"
#include "vector.h"
#include "render.h"
#include "bone.hpp"
#include "weapon.hpp"
#include "entity.h"
#include "../SDK/client.dll.hpp"
#include "../SDK/offsets.hpp"
#include "../SDK/buttons.hpp"

using namespace cs2_dumper;

INT APIENTRY WinMain(HINSTANCE instance, HINSTANCE, PSTR, INT cmd_show)
{
	std::string logFileName = "cs2_cheat_log.txt";
	std::ofstream fout(logFileName);
	fout.seekp(std::ios::beg);
	fout << "After running this software, please make sure to read the software usage instructions!" << std::endl;
	fout << "If you are stuck in the use of the software, please update the latest offsets value which located sdk folder or submit an issue on Github!" << std::endl;
	fout << "Support me? You can scan the WeiChat pay QR code or concat me yl1099129793. Thank you for your support!" << std::endl;
	fout << std::endl;

	auto mem = Memory("cs2.exe");
	auto now = std::chrono::system_clock::now();
	auto t_now = std::chrono::system_clock::to_time_t(now);
	auto tm_now = std::localtime(&t_now);
	std::stringstream ss;
	ss << std::put_time(tm_now, "%Y/%m/%d %H:%M:%S");
	if (mem.GetProcessHandle() == nullptr) 
	{
		fout << "["<< ss.str() <<"] " << "Please run this cs2 cheat app after enter cs2 game!" << std::endl;
	} 
	else
	{
		fout << "[" << ss.str() << "] " << "cs2.exe's process id located at " << mem.GetProcessId() << std::endl;
	}

	const auto client = mem.GetModuleAddress("client.dll");
	fout << "[" << ss.str() << "] " << "client.dll -> " << "0x" << std::hex << client << std::dec << std::endl;

	const unsigned int INAIR = 65664;// 在空中
	const unsigned int STANDING = 65665;// 站立
	const unsigned int CROUCHING = 65667;// 蹲伏
	const unsigned int PLUS_JUMP = 65537;// +jump
	const unsigned int MINUS_JUMP = 256;// -jump
	const unsigned int PLUS_ATTACK = 65537;// +attack
	const unsigned int MINUS_ATTACK = 256;// -attack

	gui::CreateHWindow("cs2 cheat", "yinlei", instance, cmd_show);
	gui::CreateDevice();
	gui::CreateImGui();

	Entity localPlayer;
	std::vector<Entity> entities;
	entities.reserve(64);

	static auto oldAngle = Vector3{};
	static auto newAngle = Vector3{};
	static auto velocity = Vector3{};

	while (gui::exit) 
	{
		entities.clear();

		if (!gui::exit) 
		{
			break;
		}

		gui::BeginRender();
		gui::Render();
		if (mem.GetProcessHandle() == nullptr)
		{
			gui::EndRender();
			continue;
		}

		// 处理外挂业务逻辑: 获取相关数据的地址
		localPlayer.pawnAddress = mem.Read<uintptr_t>(client + offsets::client_dll::dwLocalPlayerPawn);
		localPlayer.origin = mem.Read<Vector3>(localPlayer.pawnAddress + schemas::client_dll::C_BasePlayerPawn::m_vOldOrigin);
		localPlayer.viewOffset = mem.Read<Vector3>(localPlayer.pawnAddress + schemas::client_dll::C_BaseModelEntity::m_vecViewOffset);
		localPlayer.team = mem.Read<int>(localPlayer.pawnAddress + schemas::client_dll::C_BaseEntity::m_iTeamNum);
		localPlayer.fFlag = mem.Read<unsigned int>(localPlayer.pawnAddress + schemas::client_dll::C_BaseEntity::m_fFlags);// 玩家的fFlag
		localPlayer.flashDuration = mem.Read<float>(localPlayer.pawnAddress + schemas::client_dll::C_CSPlayerPawnBase::m_flFlashBangTime);//玩家遭受闪光的时间
		localPlayer.velocity = mem.Read<Vector3>(localPlayer.pawnAddress + schemas::client_dll::C_BaseEntity::m_vecAbsVelocity);// 玩家移动速度

		const auto enity_list = mem.Read<uintptr_t>(client + offsets::client_dll::dwEntityList);
		viewMatrix view_matrix = mem.Read<viewMatrix>(client + offsets::client_dll::dwViewMatrix);

		//处理外挂业务逻辑: 根据entityList找到currentPawn、currentController获取相关玩家信息，currentController下有pawnHandle，通过pawnHandle得到currentPawn
		for (int playerIndex = 1; playerIndex < 64; ++playerIndex)
		{
			if (!mem.InForeground()) continue;

			// 根据entity_list获取第一个入口点
			const auto list_entry = mem.Read<uintptr_t>(enity_list + (0x8 * (playerIndex & 0x7FFF) >> 9) + 0x10);
			if (!list_entry) continue;

			// 获取currentController
			const auto currentController = mem.Read<uintptr_t>(list_entry + 120 * (playerIndex & 0x1FF));
			if (!currentController) continue;

			// 通过currentController获取pawnHandle
			const auto playerPawnHandle = mem.Read<uint32_t>(currentController + schemas::client_dll::CCSPlayerController::m_hPlayerPawn);
			if (!playerPawnHandle) continue;

			// 通过pawnHandle和entityList获取第二个入口点和currentPawn
			const auto list_entry2 = mem.Read<uintptr_t>(enity_list + 0x8 * ((playerPawnHandle & 0x7FFF) >> 9) + 16);// 16==0x10
			if (!list_entry2) continue;

			const auto currentPawn = mem.Read<uintptr_t>(list_entry2 + 120 * (playerPawnHandle & 0x1FF));// 120==0x78

			// 扫描到"我"就排除我的信息
			if (!currentPawn || currentPawn == localPlayer.pawnAddress) continue;

			// 通过currentController获取团队编号、姓名等信息
			int team = mem.Read<int>(currentController + schemas::client_dll::C_BaseEntity::m_iTeamNum);

			const auto playerName = mem.ReadString<128>(currentController + schemas::client_dll::CBasePlayerController::m_iszPlayerName);

			// currentPawn有生命值、武器等信息,而玩家姓名在currentController下
			int health = mem.Read<int>(currentPawn + schemas::client_dll::C_BaseEntity::m_iHealth);
			const auto lifeState = mem.Read<unsigned int>(currentPawn + schemas::client_dll::C_BaseEntity::m_lifeState);
			if (health <= 0 || health > 100 || lifeState != 256) 
			{
				continue;
			}

			auto currentWeapon = mem.Read<uintptr_t>(currentPawn + schemas::client_dll::C_CSPlayerPawnBase::m_pClippingWeapon);
			short weaponDefinitionIndex = mem.Read<short>(currentWeapon + schemas::client_dll::C_EconEntity::m_AttributeManager + schemas::client_dll::C_AttributeContainer::m_Item + schemas::client_dll::C_EconItemView::m_iItemDefinitionIndex);

			// 获取雷达状态并设置敌人显示在雷达上
			if (gui::enableRadar) 
			{
				mem.Write<bool>(currentPawn + schemas::client_dll::C_CSPlayerPawn::m_entitySpottedState + schemas::client_dll::EntitySpottedState_t::m_bSpotted, true);
			}

			Entity entity;
			entity.pawnAddress = currentPawn;
			entity.controllerAddress = currentController;
			entity.name = playerName;
			entity.health = health;
			entity.lifeState = lifeState;
			entity.team = team;
			entity.spotted = mem.Read<bool>(currentPawn + schemas::client_dll::C_CSPlayerPawn::m_entitySpottedState + schemas::client_dll::EntitySpottedState_t::m_bSpotted);
			entity.origin = mem.Read<Vector3>(currentPawn + schemas::client_dll::C_BasePlayerPawn::m_vOldOrigin);
			entity.viewOffset = mem.Read<Vector3>(currentPawn + schemas::client_dll::C_BaseModelEntity::m_vecViewOffset);
			entity.distance = Vector3::distance(entity.origin, localPlayer.origin);
			entity.currentWeaponIndex = weaponDefinitionIndex;
			entity.currentWeaponName = getWeaponName(weaponDefinitionIndex);

			// 获取玩家的头坐标实现锁头
			const auto sceneNode = mem.Read<uintptr_t>(currentPawn + schemas::client_dll::C_BaseEntity::m_pGameSceneNode);
			// 获取骨骼信息绘制玩家骨骼
			entity.boneMatrix = mem.Read<uintptr_t>(sceneNode + schemas::client_dll::CSkeletonInstance::m_modelState + 0x80);
			entity.head = mem.Read<Vector3>(entity.boneMatrix + bones::head * 32);
			Vector3 screenHead;
			Vector3::world_to_screen(view_matrix, entity.head, screenHead);
			entity.head2d = { screenHead.x, screenHead.y };
			entity.pixelDistance = Vector2::distance(entity.head2d, Vector2{ static_cast<float>(screenWidth / 2), static_cast<float>(screenHeight / 2) });

			entities.push_back(entity);
		}

		// 根据收集的数据进行绘制
		for (auto& player : entities) 
		{
			Vector3 screenPos;
			Vector3 screenHead;

			if (Vector3::world_to_screen(view_matrix, player.origin, screenPos) &&
				Vector3::world_to_screen(view_matrix, player.head, screenHead) &&
				player.origin.x != 0) 
			{
				float height = screenPos.y - screenHead.y;
				float headHeight = height / 8;
				float width = height / 2.4f;

				if (gui::enableBoxEsp) 
				{
					if (gui::enableTeamMode) 
					{
						render::DrawRect(
							screenHead.x - width / 2,
							screenHead.y - headHeight,
							width,
							height + headHeight,
							player.team == localPlayer.team ? render::boddy : render::enemy,
							1,
							true,
							100
						);
					}
					else 
					{
						render::DrawRect(
							screenHead.x - width / 2,
							screenHead.y - headHeight,
							width,
							height + headHeight,
							render::enemy,
							1,
							true,
							100
						);
					}
				}

				render::DrawTextContent(
					screenHead.x,
					screenHead.y - headHeight,
					render::name,
					player.name.c_str()
				);

				render::DrawTextContent(
					screenHead.x,
					screenHead.y - 2*headHeight,
					render::distance,
					std::to_string(static_cast<int>(player.distance)).c_str()
				);

				if (gui::enableWeapon) 
				{
					render::DrawTextContent(
						screenHead.x - width / 2,
						screenHead.y + height,
						render::weapon,
						player.currentWeaponName
					);
				}

				if (gui::enableHealth)
				{
					render::DrawRect(
						screenHead.x - (width / 2 + 10),
						screenHead.y + (height * (100 - player.health) / 100) - headHeight,
						3,
						height - (height * (100 - player.health) / 100) + headHeight,
						render::hp,
						1.5,
						true,
						255
					);
					render::DrawTextContent(
						screenHead.x - (width / 2 + 10) - 8,
						screenHead.y + (height * (100 - player.health) / 100) - headHeight - 20,
						render::hpText,
						std::to_string(player.health).c_str()
					);
				}

				if (gui::enableBoneEsp) 
				{
					for (int i = 0; i < sizeof(boneConnections) / sizeof(boneConnections[0]); i++) {
						int bone1 = boneConnections[i].bone1;
						int bone2 = boneConnections[i].bone2;

						Vector3 vectorBone1 = mem.Read<Vector3>(player.boneMatrix + bone1 * 32);
						Vector3 vectorBone2 = mem.Read<Vector3>(player.boneMatrix + bone2 * 32);

						Vector3 b1;
						Vector3 b2;
						Vector3::world_to_screen(view_matrix, vectorBone1, b1);
						Vector3::world_to_screen(view_matrix, vectorBone2, b2);
						if(gui::enableBoxEsp){
							render::Circle(
								screenHead.x,
								screenHead.y,
								1,
								render::bone,
								false,
								255
							);
							render::Line(b1.x, b1.y, b2.x, b2.y, render::bone, 255, 1.5);
						}
						else {
							render::Circle(
								screenHead.x,
								screenHead.y,
								1,
								player.team == localPlayer.team ? render::boddy : render::bone,
								false,
								255
							);
							render::Line(b1.x, b1.y, b2.x, b2.y, player.team == localPlayer.team ? render::boddy : render::bone, 255, 1.5);
						}
					}
				}

				render::Circle(screenWidth / 2, screenHeight / 2, gui::fovAimbot, gui::fovAimbotColor, 1.0);
			}
		}

		// 自瞄锁头并开枪
		if (gui::enableAimbot && entities.size() > 0) 
		{
			if (gui::enableTeamMode)
			{
				entities.erase(std::remove_if(entities.begin(), entities.end(), [&](const Entity& entity) {
					return entity.team == localPlayer.team;
					}), entities.end());
			}
			std::stable_sort(entities.begin(), entities.end(), [](const Entity& entity1, const Entity& entity2) {
				return entity1.pixelDistance < entity2.pixelDistance;
				});

			
			// 计算自瞄需要偏移的角度
			if (entities[0].pixelDistance < gui::fovAimbot)
			{
				Vector3 playerView = localPlayer.origin + localPlayer.viewOffset;
				Vector3 entityView = entities[0].origin + entities[0].viewOffset;
				Vector3 newAngles = Vector3::angles(playerView, entities[0].head);
				Vector3 newAnglesVec3{ newAngles.y, newAngles.x, 0.0f };
				mem.Write<Vector3>(client + offsets::client_dll::dwViewAngles, newAnglesVec3);
			}

			// 自动开枪 - 查找准星瞄准的敌人数据
			// 枪口准星前的玩家实体index
			localPlayer.entIndex = mem.Read<int>(localPlayer.pawnAddress + schemas::client_dll::C_CSPlayerPawnBase::m_iIDEntIndex);
			if (gui::enableAutoAttack && localPlayer.entIndex > 0)
			{
				const auto list_entry = mem.Read<uintptr_t>(enity_list + 0x8 * (localPlayer.entIndex >> 9) + 0x10);
				const auto entity = mem.Read<uintptr_t>(list_entry + 120 * (localPlayer.entIndex & 0x1FF));
				const auto enemyTeam = mem.Read<int>(entity + schemas::client_dll::C_BaseEntity::m_iTeamNum);
				bool shouldShoot = false;
				if (gui::enableTeamMode) 
				{
					if ( enemyTeam != localPlayer.team)
						shouldShoot = true;
				}
				else 
				{
					shouldShoot = true;
				}
				if (shouldShoot) {
					mem.Write<int>(client + buttons::attack, PLUS_ATTACK);
					std::this_thread::sleep_for(std::chrono::milliseconds(2));
					mem.Write<int>(client + buttons::attack, MINUS_ATTACK);
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
			}
		}

		// 防闪光弹
		if (gui::enableFlash && localPlayer.flashDuration > 0)
		{
			mem.Write<float>(localPlayer.pawnAddress + schemas::client_dll::C_CSPlayerPawnBase::m_flFlashBangTime, 0);
		}

		// 连跳
		if (gui::enableBhop && GetAsyncKeyState(VK_SPACE))
		{
			if (localPlayer.fFlag == STANDING || localPlayer.fFlag == CROUCHING)
			{
				mem.Write<unsigned int>(client + buttons::jump, PLUS_JUMP);
			}
			else
			{
				mem.Write<unsigned int>(client + buttons::jump, MINUS_JUMP);
			}
		}

		// 跳越射击
		/*if (localPlayer.fFlag == INAIR && GetAsyncKeyState(VK_SHIFT))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(3));
			velocity = mem.Read<Vector3>(localPlayer.pawnAddress + schemas::client_dll::C_BaseEntity::m_vecAbsVelocity);
			while (velocity.z > 18 || velocity.z < -18) {
				velocity = mem.Read<Vector3>(localPlayer.pawnAddress + schemas::client_dll::C_BaseEntity::m_vecAbsVelocity);
			}
			mem.Write<int>(client + buttons::attack, PLUS_ATTACK);
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			mem.Write<int>(client + buttons::attack, MINUS_ATTACK);
		}*/

		// fov视野角度(相机Service)
		auto desiredFov = static_cast<unsigned int>(gui::fov);
		localPlayer.cameraServices = mem.Read<uintptr_t>(localPlayer.pawnAddress + schemas::client_dll::C_BasePlayerPawn::m_pCameraServices);
		unsigned int currentFov = mem.Read<unsigned int>(localPlayer.cameraServices + schemas::client_dll::CCSPlayerBase_CameraServices::m_iFOV);
		bool isScoped = mem.Read<bool>(localPlayer.pawnAddress + schemas::client_dll::C_CSPlayerPawn::m_bIsScoped);
		if (!isScoped && currentFov != desiredFov)
		{
			mem.Write<unsigned int>(localPlayer.cameraServices + schemas::client_dll::CCSPlayerBase_CameraServices::m_iFOV, desiredFov);
		}

		// 玩家暴走
		gui::speed = std::sqrt(localPlayer.velocity.x * localPlayer.velocity.x + localPlayer.velocity.y * localPlayer.velocity.y + localPlayer.velocity.z * localPlayer.velocity.z);

		// 后座力补偿
		if (gui::enableRcs)
		{
			const auto shotsFired = mem.Read<int32_t>(localPlayer.pawnAddress + schemas::client_dll::C_CSPlayerPawn::m_iShotsFired);// 开枪次数
			// 如果开枪了，就计算后座力补偿
			if (shotsFired > 1)
			{
				float m_pitch = 0.022;
				float m_yaw = 0.022;
				auto aimPunch = mem.Read<Vector3>(localPlayer.pawnAddress + schemas::client_dll::C_CSPlayerPawn::m_aimPunchAngle);
				Vector3 viewAngles = mem.Read<Vector3>(client + offsets::client_dll::dwViewAngles);
				auto sensPointer = mem.Read<uintptr_t>(client + offsets::client_dll::dwSensitivity);
				auto sensitivity = mem.Read<float>(sensPointer + offsets::client_dll::dwSensitivity_sensitivity);
				newAngle.x = (aimPunch.y - oldAngle.y) * 2.f / (m_pitch * sensitivity) / 1;
				newAngle.y = -(aimPunch.x - oldAngle.x) * 2.f / (m_yaw * sensitivity) / 1;

				mouse_event(MOUSEEVENTF_MOVE, -newAngle.x * -1, newAngle.y, 0, 0);

				oldAngle = aimPunch;
			}
			else 
			{
				oldAngle.x = oldAngle.y = oldAngle.z = 0.f;
			}
		}

		// imgui渲染工作
		gui::EndRender();
		//std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}

	gui::DestroyImGui();
	gui::DestroyDevice();
	gui::DestroyHWindow();
	fout.close();

	return 0;
}
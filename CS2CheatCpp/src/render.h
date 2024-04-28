#pragma once
#include "../dependencies/ImGui/imgui.h"
#include "../dependencies/ImGui/imgui_impl_dx11.h"
#include "../dependencies/ImGui/imgui_impl_win32.h"

typedef struct
{
	ImU32 R;
	ImU32 G;
	ImU32 B;
} RGB;

ImU32 ColorAlpha(RGB color, float alpha)
{
	return IM_COL32(color.R, color.G, color.B, alpha);
}

namespace render
{
	static RGB enemy = { 0, 0, 0 };
	static RGB boddy = { 255, 218, 116 };
	static RGB hp = { 0, 255, 0 };
	static RGB hpText = { 255, 255, 255 };
	static RGB name = { 255, 255, 255 };
	static RGB distance = { 255, 218, 116 };
	static RGB weapon = { 255, 255, 255 };
	static RGB bone = { 255, 255, 255 };

	void DrawTextContent(int x, int y, RGB color, const char* text_content)
	{
		ImGui::GetBackgroundDrawList()->AddText(ImVec2(x, y), ColorAlpha(color, 255), text_content);
	}

	void DrawRect(int x, int y, int w, int h, RGB color, int thickness, bool IsFilled, float alpha) 
	{
		if (!IsFilled) {
			ImGui::GetBackgroundDrawList()->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), ColorAlpha(color, alpha), 0, 0, thickness);
		}
		else {
			ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), ColorAlpha(color, alpha), 0, 0);
		}
	}

	void Line(float x1, float y1, float x2, float y2, RGB color, float alpha,float thickness)
	{
		ImGui::GetBackgroundDrawList()->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), ColorAlpha(color, alpha), thickness);
	}

	void Circle(float x, float y, float radius, RGB color, bool IsFilled, float alpha)
	{
		if (!IsFilled) {
			ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(x, y), radius, ColorAlpha(color, alpha), 0, 2);
		}
		else {
			ImGui::GetBackgroundDrawList()->AddCircleFilled(ImVec2(x, y), radius, ColorAlpha(color, alpha), 0);
		}
	}

	void Circle(float x, float y, float radius, float* color,float thickness)
	{
		ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(x, y), radius,  ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3])), 0, thickness);
	}
}
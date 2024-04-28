#pragma once
#include <numbers>
#include <cmath>

extern int screenWidth;
extern int screenHeight;

struct viewMatrix
{
	float matrix[4][4];

	float* operator[](int index)
	{
		return matrix[index];
	}
};

struct Vector2
{
	float x, y;

	Vector2(
		float x = 0.f,
		float y = 0.f) noexcept : x(x), y(y) {}

	const Vector2& operator-(const Vector2& other) const noexcept
	{
		return Vector2{ x - other.x, y - other.y };
	}

	static float distance(const Vector2& from, const Vector2& to)
	{
		Vector2 res = from - to;
		return std::sqrt(res.x * res.x + res.y * res.y);
	}
};

struct Vector3 
{
	float x, y, z;

	constexpr Vector3(
		const float x = 0.f,
		const float y = 0.f,
		const float z = 0.f) noexcept : x(x), y(y), z(z) {}

	constexpr const Vector3& operator+(const Vector3& other) const noexcept
	{
		return Vector3{ x + other.x, y + other.y, z + other.z };
	}

	constexpr const Vector3& operator-(const Vector3& other) const noexcept
	{
		return Vector3{ x - other.x, y - other.y, z - other.z };
	}

	constexpr const Vector3& operator*(const Vector3& other) const noexcept
	{
		return Vector3{ x * other.x, y * other.y, z * other.z };
	}

	constexpr const Vector3& operator/(const Vector3& other) const noexcept
	{
		return Vector3{ x / other.x, y / other.y, z / other.z };
	}

	constexpr const Vector3& operator/(const float  factor) const noexcept
	{
		return Vector3{ x / factor, y / factor, z / factor };
	}

	constexpr const Vector3& operator*(const float  factor) const noexcept
	{
		return Vector3{ x * factor, y * factor, z * factor };
	}

	static float distance(const Vector3& from, const Vector3& to)
	{
		Vector3 res = from - to;
		return std::sqrt(res.x * res.x + res.y * res.y + res.z * res.z);
	}

	static Vector3 angles(const Vector3& from, const Vector3& to)
	{
		float yaw;
		float pitch;

		float deltaX = to.x - from.x;
		float deltaY = to.y - from.y;

		yaw = std::atan2(deltaY, deltaX) * 180.0 / std::numbers::pi;

		float deltaZ = to.z - from.z;
		double distance = std::sqrt(std::pow(deltaX, 2) + std::pow(deltaY, 2));
		pitch = -(std::atan2(deltaZ, distance) * 180.0 / std::numbers::pi);
		return {yaw, pitch, 0};
	}

	const static bool world_to_screen(viewMatrix matrix, const Vector3& world, Vector3& screen)
	{
		screen.x= matrix[0][0] * world.x + matrix[0][1] * world.y + matrix[0][2] * world.z + matrix[0][3];
		screen.y = matrix[1][0] * world.x + matrix[1][1] * world.y + matrix[1][2] * world.z + matrix[1][3];
		float w = matrix[3][0] * world.x + matrix[3][1] * world.y + matrix[3][2] * world.z + matrix[3][3];

		if (w < 0.01f) return false;

		float inv_w = 1.f / w;
		screen.x *= inv_w;
		screen.y *= inv_w;

		float x = screenWidth / 2;
		float y = screenHeight / 2;
		
		x += 0.5f * screen.x * screenWidth + 0.5f;
		y -= 0.5f * screen.y * screenHeight + 0.5f;

		screen.x = x;
		screen.y = y;
		return true;
	}
};
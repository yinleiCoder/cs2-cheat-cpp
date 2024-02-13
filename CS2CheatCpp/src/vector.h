#pragma once
#include <numbers>
#include <cmath>

extern int screenWidth;
extern int screenHeight;

struct view_matrix_t
{
	float matrix[4][4];

	float* operator[](int index)
	{
		return matrix[index];
	}
};

struct Vector3 
{
	float x, y, z;

	constexpr Vector3(
		const float x = 0.f,
		const float y = 0.f,
		const float z = 0.f) noexcept : x(x), y(y), z(z) {}

	constexpr const Vector3& operator-(const Vector3& other) const noexcept
	{
		return Vector3{ x - other.x, y - other.y, z - other.z };
	}

	constexpr const Vector3& operator+(const Vector3& other) const noexcept
	{
		return Vector3{ x + other.x, y + other.y, z + other.z };
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

	const static bool word_to_screen(view_matrix_t matrix, Vector3& in, Vector3& out)
	{
		out.x= matrix[0][0] * in.x + matrix[0][1] * in.y + matrix[0][2] * in.z + matrix[0][3];
		out.y = matrix[1][0] * in.x + matrix[1][1] * in.y + matrix[1][2] * in.z + matrix[1][3];
		float w = matrix[3][0] * in.x + matrix[3][1] * in.y + matrix[3][2] * in.z + matrix[3][3];

		if (w<0.01f) {
			return false;
		}

		float inv_w = 1.f / w;
		out.x *= inv_w;
		out.y *= inv_w;

		float x = screenWidth / 2;
		float y = screenHeight / 2;
		
		x += 0.5f * out.x * screenWidth + 0.5f;
		y -= 0.5f * out.y * screenHeight + 0.5f;

		out.x = x;
		out.y = y;
		return true;
	}
};
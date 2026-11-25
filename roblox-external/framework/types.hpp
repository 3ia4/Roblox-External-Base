#pragma once
#include <cstdint>

struct vector3
{
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;

	vector3 operator+( const vector3& other ) const noexcept
	{
		return { x + other.x, y + other.y, z + other.z };
	}
};

struct vector2
{
	float x = 0.f;
	float y = 0.f;
};

struct matrix4
{
	float m[4][4] = {};
};

struct matrix3
{
	float m[3][3] = {};

	[[nodiscard]] vector3 transform( const vector3& v ) const noexcept
	{
		return {
			m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
			m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
			m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z
		};
	}
};

struct primitive_t
{
	char pad_0[176] = {};
	matrix3 rotation{};
	vector3 position{};
	char pad_1[228] = {};
	vector3 size{};
};

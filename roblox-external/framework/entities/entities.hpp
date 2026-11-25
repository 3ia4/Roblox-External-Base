#pragma once
#include <cstdint>
#include <vector>

struct bone_t
{
	std::uintptr_t primitive = 0;
};

struct entity_t
{
	std::vector<bone_t> bones{};
};

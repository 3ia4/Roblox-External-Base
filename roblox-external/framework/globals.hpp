#pragma once
#include "types.hpp"

struct globals_t
{
	vector2 dimensions{};
	matrix4 view_matrix{};
};

inline globals_t game{};

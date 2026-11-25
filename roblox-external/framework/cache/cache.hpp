#pragma once
#include <vector>
#include "../entities/entities.hpp"

namespace snapshot
{
	[[nodiscard]] const std::vector< entity_t >& get_snapshot( );
}

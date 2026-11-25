#pragma once
#include <cstdint>
#include "../../memory/mem.h"

struct memory_t
{
	template< typename T >
	T read( std::uintptr_t addr ) const
	{
		return mem::get().read< T >( addr );
	}
};

inline memory_t g_memory{};

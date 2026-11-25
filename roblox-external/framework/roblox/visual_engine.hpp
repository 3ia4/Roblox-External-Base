#pragma once
#include <cstdint>
#include "../types.hpp"
#include "../../offsets/updater.h"
#include "../../memory/mem.h"

struct visual_engine_t
{
	std::uintptr_t pointer = 0;

	[[nodiscard]] static visual_engine_t get( )
	{
		visual_engine_t engine{};
		engine.pointer = mem::get().read< std::uintptr_t >( mem::get().base + OFFSET( VisualEngine, Pointer ) );
		return engine;
	}

	[[nodiscard]] bool valid( ) const noexcept
	{
		return pointer != 0;
	}

	template< typename T >
	[[nodiscard]] T get_member( std::uintptr_t offset ) const
	{
		return mem::get().read< T >( pointer + offset );
	}

	[[nodiscard]] bool world_to_screen( const vector3& world, vector2& out ) const noexcept;
};

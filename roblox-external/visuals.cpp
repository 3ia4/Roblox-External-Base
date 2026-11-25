#include "visuals.hpp"

#include <algorithm>
#include <chrono>
#include <thread>

#include "framework/cache/cache.hpp"
#include "framework/memory/memory.hpp"
#include "framework/roblox/visual_engine.hpp"
#include "memory/mem.h"

namespace
{
	constexpr ImU32 box_outline = IM_COL32( 0, 0, 0, 255 );
	constexpr ImU32 box_color = IM_COL32( 255, 255, 255, 255 );
}

bool visuals_t::calculate_esp( const visual_engine_t& engine, const entity_t& entity, esp_info_t& out )
{
	out = esp_info_t{};

	bool any = false;

	for ( const auto& bone : entity.bones )
	{
		if ( !bone.primitive )
			continue;

		const primitive_t prim = g_memory.read< primitive_t >( bone.primitive );

		if ( prim.size.x <= 0.f || prim.size.x > 100.f ||
			prim.size.y <= 0.f || prim.size.y > 100.f ||
			prim.size.z <= 0.f || prim.size.z > 100.f )
			continue;

		const vector3 half{ prim.size.x * 0.5f, prim.size.y * 0.5f, prim.size.z * 0.5f };

		vector2 corners[8]{};
		int count = 0;
		bool valid = true;

		for ( int x = -1; x <= 1 && valid; x += 2 )
		{
			for ( int y = -1; y <= 1 && valid; y += 2 )
			{
				for ( int z = -1; z <= 1 && valid; z += 2 )
				{
					const vector3 local{ half.x * x, half.y * y, half.z * z };
					const vector3 world = prim.position + prim.rotation.transform( local );

					if ( !engine.world_to_screen( world, corners[count] ) )
					{
						valid = false;
						break;
					}

					++count;
				}
			}
		}

		if ( !valid )
			continue;

		any = true;

		for ( int i = 0; i < count; ++i )
		{
			out.box.Min.x = (std::min)( out.box.Min.x, corners[i].x );
			out.box.Min.y = (std::min)( out.box.Min.y, corners[i].y );
			out.box.Max.x = (std::max)( out.box.Max.x, corners[i].x );
			out.box.Max.y = (std::max)( out.box.Max.y, corners[i].y );
		}
	}

	return any && out.box.Max.x > out.box.Min.x && out.box.Max.y > out.box.Min.y;
}

void visuals_t::update( )
{
	std::vector< ImRect > next{};

	if ( boxes && mem::get().base )
	{
		const auto engine = visual_engine_t::get( );

		if ( engine.valid( ) )
		{
			game.dimensions = engine.get_member< vector2 >( OFFSET( VisualEngine, Dimensions ) );
			game.view_matrix = engine.get_member< matrix4 >( OFFSET( VisualEngine, ViewMatrix ) );

			if ( game.dimensions.x >= 1.f && game.dimensions.y >= 1.f )
			{
				const auto& entities = snapshot::get_snapshot( );
				next.reserve( entities.size( ) );

				for ( const auto& entity : entities )
				{
					esp_info_t info{};
					if ( calculate_esp( engine, entity, info ) )
						next.push_back( info.box );
				}
			}
		}
	}

	std::lock_guard< std::mutex > lock( box_mutex );

	if ( !next.empty( ) )
	{
		current_boxes = std::move( next );
		stale_frames = 0;
	}
	else if ( ++stale_frames > 5 )
	{
		current_boxes.clear( );
		stale_frames = 0;
	}
}

void visuals_t::draw_box( ImDrawList* draw, const ImRect& bb ) const
{
	draw->AddRect( bb.Min, bb.Max, box_outline, 0.f, 0, 3.f );
	draw->AddRect( bb.Min, bb.Max, box_color, 0.f, 0, 1.f );
}

void visuals_t::render( )
{
	if ( !boxes )
		return;

	ImDrawList* draw = ImGui::GetBackgroundDrawList( );
	if ( !draw )
		return;

	std::lock_guard< std::mutex > lock( box_mutex );
	for ( const auto& bb : current_boxes )
		draw_box( draw, bb );
}

void visuals_update_loop( )
{
	int dead = 0;

	while ( true )
	{
		const auto engine = visual_engine_t::get( );
		if ( !engine.valid( ) )
		{
			if ( ++dead > 60 )
			{
				dead = 0;
				mem::get().attach( L"RobloxPlayerBeta.exe" );
			}
		}
		else
		{
			dead = 0;
		}

		visuals.update( );
		std::this_thread::sleep_for( std::chrono::milliseconds( 16 ) );
	}
}

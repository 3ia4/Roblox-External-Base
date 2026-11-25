#include "visual_engine.hpp"
#include "../globals.hpp"

bool visual_engine_t::world_to_screen( const vector3& world, vector2& out ) const noexcept
{
	const float x = world.x;
	const float y = world.y;
	const float z = world.z;

	const auto& m = game.view_matrix.m;
	const float ndc_x = m[0][0] * x + m[0][1] * y + m[0][2] * z + m[0][3];
	const float ndc_y = m[1][0] * x + m[1][1] * y + m[1][2] * z + m[1][3];
	const float w = m[3][0] * x + m[3][1] * y + m[3][2] * z + m[3][3];

	if ( w <= 0.001f )
		return false;

	out.x = ( ndc_x / w * 0.5f + 0.5f ) * game.dimensions.x;
	out.y = ( 0.5f - ndc_y / w * 0.5f ) * game.dimensions.y;
	return true;
}

#ifndef visuals_hpp
#define visuals_hpp

#include <cfloat>
#include <mutex>
#include <vector>

#include "framework/globals.hpp"
#include "framework/roblox/visual_engine.hpp"
#include "framework/entities/entities.hpp"
#include "deps/imgui/imgui_internal.h"

struct esp_info_t
{
	ImRect box{ FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX };
};

struct visuals_t
{
	bool boxes = true;
	bool menu_open = false;
	int stale_frames = 0;

	std::vector< ImRect > current_boxes{};
	std::mutex box_mutex{};

	[[nodiscard]] bool calculate_esp( const visual_engine_t& engine, const entity_t& entity, esp_info_t& out );

	void update( );
	void render( );

private:
	void draw_box( ImDrawList* draw, const ImRect& bb ) const;
};

inline visuals_t visuals{};

void visuals_update_loop( );

#endif  // visuals_hpp

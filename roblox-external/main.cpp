#include <chrono>
#include <thread>

#include <windows.h>

#include "framework/render.hpp"
#include "memory/mem.h"
#include "offsets/updater.h"
#include "visuals.hpp"

auto main( ) -> int
{
	ShowWindow( GetConsoleWindow( ), SW_HIDE );

	if ( !_updater::get()._static( ) )
		return 1;

	mem::get().attach( L"RobloxPlayerBeta.exe" );

	std::thread esp( visuals_update_loop );
	std::thread overlay( render_loop );

	esp.detach( );
	overlay.detach( );

	while ( true )
		std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
}

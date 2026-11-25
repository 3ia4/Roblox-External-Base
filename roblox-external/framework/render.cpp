#include "render.hpp"

#include "../deps/imgui/imgui.h"
#include "../deps/imgui/backends/imgui_impl_win32.h"
#include "../deps/imgui/backends/imgui_impl_dx11.h"
#include "../visuals.hpp"

#include <mutex>

#include <d3d11.h>
#include <dxgi1_2.h>
#include <dcomp.h>
#include <windows.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam );

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dcomp.lib")

namespace
{
	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* context = nullptr;
	IDXGISwapChain1* swap_chain = nullptr;
	ID3D11RenderTargetView* target = nullptr;

	IDCompositionDevice* composition = nullptr;
	IDCompositionTarget* composition_target = nullptr;
	IDCompositionVisual* composition_visual = nullptr;

	void enable_dpi_awareness( )
	{
		using fn = BOOL( WINAPI* )( void* );
		const auto set_context = reinterpret_cast< fn >( GetProcAddress( GetModuleHandleW( L"user32.dll" ), "SetProcessDpiAwarenessContext" ) );
		if ( set_context )
			set_context( reinterpret_cast< void* >( -4 ) );
	}

	void destroy_target( )
	{
		if ( target )
		{
			target->Release( );
			target = nullptr;
		}
	}

	void release_device( )
	{
		destroy_target( );

		if ( composition_visual ) { composition_visual->Release( ); composition_visual = nullptr; }
		if ( composition_target ) { composition_target->Release( ); composition_target = nullptr; }
		if ( composition ) { composition->Release( ); composition = nullptr; }

		if ( swap_chain ) { swap_chain->Release( ); swap_chain = nullptr; }
		if ( context ) { context->Release( ); context = nullptr; }
		if ( device ) { device->Release( ); device = nullptr; }
	}

	HRESULT create_target( )
	{
		ID3D11Texture2D* back_buffer = nullptr;
		HRESULT result = swap_chain->GetBuffer( 0, IID_PPV_ARGS( &back_buffer ) );
		if ( FAILED( result ) )
			return result;

		result = device->CreateRenderTargetView( back_buffer, nullptr, &target );
		back_buffer->Release( );
		return result;
	}

	HRESULT create_device( HWND window, int width, int height )
	{
		const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

		HRESULT result = D3D11CreateDevice(
			nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
			D3D11_CREATE_DEVICE_BGRA_SUPPORT,
			levels, static_cast< UINT >( sizeof( levels ) / sizeof( levels[0] ) ),
			D3D11_SDK_VERSION, &device, nullptr, &context );

		if ( FAILED( result ) )
			return result;

		IDXGIDevice* dxgi_device = nullptr;
		result = device->QueryInterface( IID_PPV_ARGS( &dxgi_device ) );
		if ( FAILED( result ) )
		{
			release_device( );
			return result;
		}

		IDXGIFactory2* factory = nullptr;
		result = CreateDXGIFactory1( IID_PPV_ARGS( &factory ) );
		if ( FAILED( result ) )
		{
			dxgi_device->Release( );
			release_device( );
			return result;
		}

		DXGI_SWAP_CHAIN_DESC1 desc{};
		desc.Width = static_cast< UINT >( width );
		desc.Height = static_cast< UINT >( height );
		desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.BufferCount = 2;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
		desc.Flags = 0;

		result = factory->CreateSwapChainForComposition( dxgi_device, &desc, nullptr, &swap_chain );
		factory->Release( );
		dxgi_device->Release( );

		if ( FAILED( result ) )
		{
			release_device( );
			return result;
		}

		result = DCompositionCreateDevice( nullptr, IID_PPV_ARGS( &composition ) );
		if ( FAILED( result ) )
		{
			release_device( );
			return result;
		}

		result = composition->CreateTargetForHwnd( window, TRUE, &composition_target );
		if ( FAILED( result ) )
		{
			release_device( );
			return result;
		}

		result = composition->CreateVisual( &composition_visual );
		if ( FAILED( result ) )
		{
			release_device( );
			return result;
		}

		result = composition_visual->SetContent( swap_chain );
		if ( FAILED( result ) )
		{
			release_device( );
			return result;
		}

		result = composition_target->SetRoot( composition_visual );
		if ( FAILED( result ) )
		{
			release_device( );
			return result;
		}

		result = composition->Commit( );
		if ( FAILED( result ) )
		{
			release_device( );
			return result;
		}

		return create_target( );
	}

	LRESULT CALLBACK wnd_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
	{
		if ( ImGui_ImplWin32_WndProcHandler( hwnd, msg, wparam, lparam ) )
			return true;

		switch ( msg )
		{
		case WM_NCHITTEST:
			return visuals.menu_open ? DefWindowProcW( hwnd, msg, wparam, lparam ) : HTTRANSPARENT;
		case WM_ERASEBKGND:
			return TRUE;
		case WM_SIZE:
			if ( swap_chain && wparam != SIZE_MINIMIZED )
			{
				destroy_target( );
				swap_chain->ResizeBuffers( 0, static_cast< UINT >( LOWORD( lparam ) ), static_cast< UINT >( HIWORD( lparam ) ), DXGI_FORMAT_UNKNOWN, 0 );
				create_target( );
			}
			return 0;
		case WM_DESTROY:
			PostQuitMessage( 0 );
			return 0;
		}

		return DefWindowProcW( hwnd, msg, wparam, lparam );
	}

	void set_menu_state( HWND window, bool open )
	{
		LONG_PTR ex = GetWindowLongPtrW( window, GWL_EXSTYLE );
		ex |= WS_EX_LAYERED;

		if ( open )
			ex &= ~( WS_EX_TRANSPARENT | WS_EX_NOACTIVATE );
		else
			ex |= WS_EX_TRANSPARENT | WS_EX_NOACTIVATE;

		SetWindowLongPtrW( window, GWL_EXSTYLE, ex );
		SetWindowPos( window, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED );

		if ( !open )
			ReleaseCapture( );
	}

	void cleanup( HWND window, HINSTANCE instance )
	{
		ImGui_ImplDX11_Shutdown( );
		ImGui_ImplWin32_Shutdown( );
		ImGui::DestroyContext( );

		release_device( );

		if ( window )
			DestroyWindow( window );

		UnregisterClassW( L"framework_overlay", instance );
	}
}

void render_loop( )
{
	enable_dpi_awareness( );

	HINSTANCE instance = GetModuleHandleW( nullptr );

	WNDCLASSW wc{};
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = wnd_proc;
	wc.hInstance = instance;
	wc.hbrBackground = nullptr;
	wc.lpszClassName = L"framework_overlay";

	if ( !RegisterClassW( &wc ) && GetLastError( ) != ERROR_ALREADY_EXISTS )
		return;

	const int width = GetSystemMetrics( SM_CXSCREEN );
	const int height = GetSystemMetrics( SM_CYSCREEN );

	HWND window = CreateWindowExW(
		WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
		L"framework_overlay", nullptr, WS_POPUP,
		0, 0, width, height,
		nullptr, nullptr, instance, nullptr );

	if ( !window )
		return;

	ShowWindow( window, SW_SHOW );
	UpdateWindow( window );

	RECT client{};
	GetClientRect( window, &client );
	const int buffer_width = client.right - client.left;
	const int buffer_height = client.bottom - client.top;

	const HRESULT device_result = create_device( window, buffer_width, buffer_height );
	if ( FAILED( device_result ) )
	{
		DestroyWindow( window );
		UnregisterClassW( L"framework_overlay", instance );
		return;
	}

	IMGUI_CHECKVERSION( );
	ImGui::CreateContext( );

	ImGuiIO& io = ImGui::GetIO( );
	io.IniFilename = nullptr;

	if ( !ImGui_ImplWin32_Init( window ) || !ImGui_ImplDX11_Init( device, context ) )
	{
		cleanup( window, instance );
		return;
	}

	const float clear_color[4] = { 0.f, 0.f, 0.f, 0.f };
	bool running = true;
	bool last_toggle = false;
	MSG msg{};

	while ( running )
	{
		while ( PeekMessageW( &msg, nullptr, 0, 0, PM_REMOVE ) )
		{
			if ( msg.message == WM_QUIT )
			{
				running = false;
				break;
			}

			TranslateMessage( &msg );
			DispatchMessageW( &msg );
		}

		if ( !running || !target )
			break;

		const bool toggle = ( GetAsyncKeyState( VK_INSERT ) & 0x8000 ) != 0;
		if ( toggle && !last_toggle )
		{
			visuals.menu_open = !visuals.menu_open;
			set_menu_state( window, visuals.menu_open );
		}
		last_toggle = toggle;

		ImGui_ImplWin32_NewFrame( );
		ImGui_ImplDX11_NewFrame( );
		ImGui::NewFrame( );

		visuals.render( );

		if ( visuals.menu_open )
		{
			ImGui::SetNextWindowPos( ImVec2( 40.f, 40.f ), ImGuiCond_Always );
			ImGui::SetNextWindowSize( ImVec2( 140.f, 80.f ), ImGuiCond_Always );
			ImGui::Begin( "$", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize );
			ImGui::Checkbox( "box", &visuals.boxes );
			ImGui::End( );
		}

		ImGui::Render( );

		context->OMSetRenderTargets( 1, &target, nullptr );
		context->ClearRenderTargetView( target, clear_color );
		ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData( ) );
		swap_chain->Present( 0, 0 );
	}

	cleanup( window, instance );
}

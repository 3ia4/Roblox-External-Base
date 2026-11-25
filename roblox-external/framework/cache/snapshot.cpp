#include "cache.hpp"

#include <chrono>

#include "../../memory/mem.h"
#include "../../offsets/updater.h"

namespace snapshot
{
	namespace
	{
		std::vector< entity_t > cached{};
		std::chrono::steady_clock::time_point last_refresh{};

		std::vector< entity_t > build_snapshot( )
		{
			std::vector< entity_t > snapshot{};

			const std::uintptr_t base = mem::get().base;
			if ( !base )
				return snapshot;

			const std::uintptr_t fake_datamodel = mem::get().read< std::uintptr_t >( base + OFFSET( FakeDataModel, Pointer ) );
			if ( !fake_datamodel )
				return snapshot;

			const std::uintptr_t datamodel = mem::get().read< std::uintptr_t >( fake_datamodel + OFFSET( FakeDataModel, RealDataModel ) );
			if ( !datamodel )
				return snapshot;

			const std::uintptr_t players = mem::get().find_child( datamodel, "Players" );
			if ( !players )
				return snapshot;

			for ( const auto player : mem::get().get_children( players ) )
			{
				const std::uintptr_t model = mem::get().read< std::uintptr_t >( player + OFFSET( Player, ModelInstance ) );
				if ( !model )
					continue;

				entity_t entity{};

				for ( const auto part : mem::get().get_children( model ) )
				{
					if ( !part )
						continue;

					const std::uintptr_t class_descriptor = mem::get().read< std::uintptr_t >( part + OFFSET( Instance, ClassDescriptor ) );
					if ( !class_descriptor )
						continue;

					const std::uintptr_t class_name_ptr = mem::get().read< std::uintptr_t >( class_descriptor + OFFSET( Instance, ClassName ) );
					if ( !class_name_ptr )
						continue;

					const std::string class_name = mem::get().read_string( class_name_ptr );
					if ( class_name != "Part" && class_name != "MeshPart" )
						continue;

					const std::uintptr_t primitive = mem::get().read< std::uintptr_t >( part + OFFSET( BasePart, Primitive ) );
					if ( !primitive )
						continue;

					bone_t bone{};
					bone.primitive = primitive;
					entity.bones.push_back( bone );
				}

				if ( !entity.bones.empty( ) )
					snapshot.push_back( entity );
			}

			return snapshot;
		}
	}

	const std::vector< entity_t >& get_snapshot( )
	{
		const auto now = std::chrono::steady_clock::now( );
		if ( now - last_refresh >= std::chrono::milliseconds( 250 ) )
		{
			last_refresh = now;
			cached = build_snapshot( );
		}

		return cached;
	}
}

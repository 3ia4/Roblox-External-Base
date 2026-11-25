#pragma once
#include <string>
#include <cstdint>
#include "../deps/nlohmann/json.hpp"

using json = nlohmann::json;

struct _updater
{
	// < globals >
	std::string _version;
	std::string _cached_version;
	json _offsets;

	static _updater& get();

	bool _static();
	uintptr_t offset(const char* cls, const char* field);
};

/* < lookup > */
#define OFFSET(cls, field) _updater::get().offset(#cls, #field)

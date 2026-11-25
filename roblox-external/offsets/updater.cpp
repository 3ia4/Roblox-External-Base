#include "updater.h"

#include <windows.h>
#include <winhttp.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <vector>
#include <cstdio>

#pragma comment(lib, "winhttp.lib")

namespace fs = std::filesystem;

static std::string http_get(const wchar_t* host, const wchar_t* path)
{
	HINTERNET session = WinHttpOpen(L"OffsetsClient/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!session)
		throw std::runtime_error("WinHttpOpen failed");

	HINTERNET connect = WinHttpConnect(session, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
	if (!connect)
	{
		WinHttpCloseHandle(session);
		throw std::runtime_error("WinHttpConnect failed");
	}

	HINTERNET request = WinHttpOpenRequest(connect, L"GET", path, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
	if (!request)
	{
		WinHttpCloseHandle(connect);
		WinHttpCloseHandle(session);
		throw std::runtime_error("WinHttpOpenRequest failed");
	}

	WinHttpSetTimeouts(request, 5000, 5000, 10000, 10000);

	BOOL ok = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
	if (!ok || !WinHttpReceiveResponse(request, nullptr))
	{
		WinHttpCloseHandle(request);
		WinHttpCloseHandle(connect);
		WinHttpCloseHandle(session);
		throw std::runtime_error("request failed");
	}

	DWORD status = 0;
	DWORD status_size = sizeof(status);
	WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size, WINHTTP_NO_HEADER_INDEX);

	if (status != 200)
	{
		WinHttpCloseHandle(request);
		WinHttpCloseHandle(connect);
		WinHttpCloseHandle(session);
		throw std::runtime_error("HTTP " + std::to_string(status));
	}

	std::string body;
	DWORD available = 0;
	while (WinHttpQueryDataAvailable(request, &available) && available > 0)
	{
		if (body.size() + available > 4 * 1024 * 1024)
		{
			WinHttpCloseHandle(request);
			WinHttpCloseHandle(connect);
			WinHttpCloseHandle(session);
			throw std::runtime_error("response too big");
		}

		std::vector<char> buf(available);
		DWORD read = 0;
		if (!WinHttpReadData(request, buf.data(), available, &read))
			break;
		body.append(buf.data(), read);
	}

	WinHttpCloseHandle(request);
	WinHttpCloseHandle(connect);
	WinHttpCloseHandle(session);
	return body;
}

static fs::path cache_path(const char* name)
{
	static fs::path dir = []() {
		char path[MAX_PATH] = { 0 };
		GetModuleFileNameA(nullptr, path, MAX_PATH);
		return fs::path(path).parent_path();
	}();

	return dir / name;
}

static std::string read_cached(const char* name)
{
	std::ifstream file(cache_path(name), std::ios::binary);
	if (!file) return {};

	std::string body((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	return body;
}

static void write_cached(const char* name, const std::string& body)
{
	std::ofstream file(cache_path(name), std::ios::trunc | std::ios::binary);
	file << body;
}
_updater& _updater::get()
{
	static _updater instance;
	return instance;
}

bool _updater::_static()
{
	std::string live_version;
	try
	{
		live_version = http_get(L"offsets.imtheo.lol", L"/roblox/version");
	}
	catch (const std::exception& e)
	{
		std::printf("[-] version check failed -> %s\n", e.what());
	}

	if (!live_version.empty())
	{
		_cached_version = read_cached("offsets.version");

		if (live_version != _cached_version || !fs::exists(cache_path("offsets.json")))
		{
			try
			{
				std::string body = http_get(L"offsets.imtheo.lol", L"/offsets.json");
				json parsed = json::parse(body);

				if (!parsed.contains("Offsets") || !parsed["Offsets"].is_object())
					throw std::runtime_error("bad offsets.json");

				write_cached("offsets.json", body);
				write_cached("offsets.version", live_version);
				_cached_version = live_version;
			}
			catch (const std::exception& e)
			{
				std::printf("[-] offsets download failed -> %s\n", e.what());
			}
		}
	}

	try
	{
		std::string body = read_cached("offsets.json");
		if (body.empty())
			return false;

		_offsets = json::parse(body);
		_version = _offsets.value("Roblox Version", _cached_version);
		std::printf("[+] offsets -> %s\n", _version.c_str());
		return true;
	}
	catch (const std::exception& e)
	{
		std::printf("[-] offsets load failed -> %s\n", e.what());
		return false;
	}
}

uintptr_t _updater::offset(const char* cls, const char* field)
{
	try
	{
		if (!_offsets.contains("Offsets")) return 0;

		const auto& classes = _offsets["Offsets"];
		if (!classes.is_object() || !classes.contains(cls)) return 0;

		const auto& entry = classes[cls];
		if (!entry.is_object() || !entry.contains(field)) return 0;

		const auto& value = entry[field];
		if (!value.is_number_unsigned() && !value.is_number_integer()) return 0;

		return value.get<uintptr_t>();
	}
	catch (...)
	{
		return 0;
	}
}


// MIT License
//
// Copyright (c) 2019 degski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the \"Software\"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#if _WIN32
#    if defined( _DEBUG )
#        pragma comment( lib, "fmtd.lib" )
#    else
#        pragma comment( lib, "fmt.lib" )
#    endif
#endif

#include "calendar.hpp"

#ifndef NOMINMAX
#    define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#    define _CRT_SECURE_NO_WARNINGS
#endif

#include <Windows.h>

#include <filesystem>
#include <map>
#include <string>

#include <nlohmann/json.hpp>

// for convenience.
using json = nlohmann::json;

namespace fs = std::filesystem;

bool init ( );

[[nodiscard]] fs::path get_app_data_path ( std::wstring && place_ ) noexcept;

inline fs::path const g_app_data_path      = get_app_data_path ( L"timezoneinfo" );
inline fs::path const g_windows_zones_path = g_app_data_path / L"windowsZones.xml";

using tzi_t = TIME_ZONE_INFORMATION;

using Timestamps = std::map<std::string, std::uint64_t>;

inline Timestamps g_timestamps;
inline fs::path const g_timestamps_path = g_app_data_path / L"timestamps.json";

[[nodiscard]] tzi_t get_tzi ( std::string const & desc_ ) noexcept;
[[nodiscard]] bool has_dst ( tzi_t const & tzi ) noexcept;

[[nodiscard]] systime_t get_systime_in_tz ( tzi_t const & tzi_ ) noexcept;
[[nodiscard]] wintime_t get_wintime_in_tz ( tzi_t const & tzi_ ) noexcept;
[[nodiscard]] nixtime_t get_nixtime_in_tz ( tzi_t const & tzi_ ) noexcept;

void save_timestamps ( );
void load_timestamps ( );

void save_to_file ( json const & j_, std::wstring const & name_ );
void load_from_file ( json & j_, std::wstring const & name_ );
json load ( fs::path const & file_ );

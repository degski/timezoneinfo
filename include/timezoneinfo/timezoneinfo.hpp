
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

#include "calendar.hpp"
#include "ianamap.hpp"

// #define FMT_USE_GRISU 1
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <fmt/compile.h>

#include <nlohmann/json.hpp>

// for convenience.
using json = nlohmann::json;

int init ( );
int init_alt ( );

[[nodiscard]] fs::path get_app_data_path ( std::wstring && place_ ) noexcept;

inline fs::path const g_app_data_path         = get_app_data_path ( L"timezoneinfo" );
inline fs::path const g_windowszones_path     = g_app_data_path / L"windowszones.xml";
inline fs::path const g_windowszones_alt_path = g_app_data_path / L"windowszones_alt.csv.gz";

using tzi_t = TIME_ZONE_INFORMATION;

using Timestamps = std::map<std::string, std::uint64_t>;

inline Timestamps g_timestamps;
inline fs::path const g_timestamps_path = g_app_data_path / L"timestamps.json";

inline IanaMap g_iana = build_iana_to_windowszones_map ( );

[[nodiscard]] tzi_t get_tzi ( std::string const & desc_ ) noexcept;
[[nodiscard]] tzi_t const & get_tzi_utc ( ) noexcept;

[[nodiscard]] bool has_dst ( tzi_t const & tzi ) noexcept;

/*
[[nodiscard]] systime_t get_systime_in_tz ( tzi_t const & tzi_, systime_t const & system_time_ ) noexcept;
[[nodiscard]] wintime_t get_wintime_in_tz ( tzi_t const & tzi_, wintime_t const & wintime_ ) noexcept;
[[nodiscard]] nixtime_t get_nixtime_in_tz ( tzi_t const & tzi_, nixtime_t const & nixtime_ ) noexcept;
*/

// Return time-zone specific current local systime.
[[nodiscard]] systime_t get_systime_in_tz ( tzi_t const & tzi_ ) noexcept;
// Return time-zone specific current local wintime.
[[nodiscard]] wintime_t get_wintime_in_tz ( tzi_t const & tzi_ ) noexcept;
// Return time-zone specific current local nixtime.
[[nodiscard]] nixtime_t get_nixtime_in_tz ( tzi_t const & tzi_ ) noexcept;

[[nodiscard]] systime_t get_systime_in_tz ( systime_t const & system_time_ ) noexcept;
[[nodiscard]] wintime_t get_wintime_in_tz ( wintime_t const & wintime_ ) noexcept;
[[nodiscard]] nixtime_t get_nixtime_in_tz ( nixtime_t const & nixtime_ ) noexcept;

// Return system time from date in UTC.
[[nodiscard]] systime_t date_to_systime ( int const y_, int const m_, int const d_ ) noexcept;
// Return windows time from date in UTC.
[[nodiscard]] wintime_t date_to_wintime ( int const y_, int const m_, int const d_ ) noexcept;

[[nodiscard]] int days_since ( int const y_, int const m_, int const d_ ) noexcept;
[[nodiscard]] int days_since_winepoch ( ) noexcept;

[[nodiscard]] std::int64_t local_utc_offset_minutes ( ) noexcept;

void save_timestamps ( );
void load_timestamps ( );

void save_to_file ( json const & j_, std::wstring const & name_ );
void load_from_file ( json & j_, std::wstring const & name_ );
json load ( fs::path const & file_ );

void print_nixtime ( nixtime_t const rawtime_ ) noexcept;
void print_systime ( systime_t const & system_time_ ) noexcept;
void print_wintime ( wintime_t const & rawtime_ ) noexcept;

template<typename Stream>
void print_systime ( Stream & os_, systime_t const & system_time_ ) noexcept {
    // Thu Aug 22 13:41:12.256 2019
    os_ << fmt::format ( "{} {} {:2} {:02}:{:02}:{:02}.{:03} {}", dow[ system_time_.wDayOfWeek ], moy[ system_time_.wMonth - 1 ],
                         system_time_.wDay, system_time_.wHour, system_time_.wMinute, system_time_.wSecond,
                         system_time_.wMilliseconds, system_time_.wYear );
}

template<typename Stream>
void print_wintime ( Stream & os_, wintime_t const & rawtime_ ) noexcept {
    os_ << wintime_to_systime ( get_wintime_in_tz ( rawtime_ ) );
}

template<typename Stream>
[[maybe_unused]] Stream & operator<< ( Stream & os_, systime_t const & systime_ ) {
    print_systime ( os_, systime_ );
    return os_;
}

template<typename Stream>
[[maybe_unused]] Stream & operator<< ( Stream & os_, wintime_t const wintime_ ) {
    print_wintime ( os_, wintime_ );
    return os_;
}

void download ( char const url_[], fs::path const & path_ );

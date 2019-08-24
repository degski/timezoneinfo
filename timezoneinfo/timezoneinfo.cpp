
// MIT License
//
// Copyright (c) 2019 degski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "timezoneinfo.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include <fstream>
#include <sax/iostream.hpp>

#include <fmt/core.h>
#include <fmt/format.h>

bool init ( ) {
    if ( fs::exists ( g_timestamps_path ) )
        load_timestamps ( );
    if ( not fs::exists ( g_windowszones_path ) or ( wintime ( ).i - g_timestamps.at ( "last_windowszones_download" ) ) >
                                                       ( 30ULL * 24ULL * 60ULL * 60ULL * 10'000'000ULL ) ) {
        download_windowszones ( );
        g_timestamps.insert_or_assign ( "last_windowszones_download", wintime ( ).i );
        save_timestamps ( );
    }
    return true;
}

fs::path get_app_data_path ( std::wstring && place_ ) noexcept {
    wchar_t * value;
    std::size_t len;
    _wdupenv_s ( &value, &len, L"USERPROFILE" );
    fs::path return_value ( std::wstring ( value ) + std::wstring ( L"\\AppData\\Roaming\\" + place_ ) );
    fs::create_directory ( return_value ); // No error if directory exists.
    return return_value;
}

tzi_t get_tzi ( std::string const & iana_ ) noexcept {
    // The registry entry for TZI.
    struct REG_TZI_FORMAT {
        LONG Bias;
        LONG StandardBias;
        LONG DaylightBias;
        systime_t StandardDate;
        systime_t DaylightDate;
    };
    // Variables.
    HKEY key = nullptr;
    DWORD data_length;
    REG_TZI_FORMAT reg_tzi{};
    tzi_t tzi{};
    // Create URI.
    std::wstring const desc = fmt::to_wstring ( g_iana.at ( iana_ ).name );
    std::wstring const uri  = std::wstring ( L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\" ) + desc;
    auto result             = RegOpenKeyEx ( HKEY_LOCAL_MACHINE, uri.c_str ( ), 0, KEY_READ, &key );
    assert ( ERROR_SUCCESS == result );
    data_length = sizeof ( REG_TZI_FORMAT );
    RegQueryValueEx ( key, TEXT ( "TZI" ), NULL, NULL, ( LPBYTE ) &reg_tzi, &data_length );
    tzi.Bias    = reg_tzi.Bias; // UTC = local time + bias.
    data_length = 64;
    RegQueryValueEx ( key, TEXT ( "Std" ), NULL, NULL, ( LPBYTE ) &tzi.StandardName, &data_length );
    if ( reg_tzi.StandardDate.wMonth ) {
        tzi.StandardDate = reg_tzi.StandardDate;
        tzi.StandardBias = reg_tzi.StandardBias;
    }
    if ( reg_tzi.DaylightDate.wMonth ) {
        data_length = 64;
        RegQueryValueEx ( key, TEXT ( "Dlt" ), NULL, NULL, ( LPBYTE ) &tzi.DaylightName, &data_length );
        tzi.DaylightDate = reg_tzi.DaylightDate;
        tzi.DaylightBias = reg_tzi.DaylightBias;
    }
    assert ( not reg_tzi.StandardDate.wMonth == not reg_tzi.DaylightDate.wMonth );
    RegCloseKey ( key );

#if 0

    std::wstring const uri_dynamic_dst = uri + std::wstring ( L"\\Dynamic DST" );

    auto const exists = RegOpenKeyEx ( HKEY_LOCAL_MACHINE, uri_dynamic_dst.c_str ( ), 0, KEY_READ, &key );

    if ( ERROR_SUCCESS == exists ) {
        DWORD const current_year = static_cast<DWORD> ( today_year ( ) );
        DWORD first_year = 0u, last_year = 0u;

        data_length = sizeof ( DWORD );
        RegQueryValueEx ( key, TEXT ( "LastEntry" ), NULL, NULL, ( LPBYTE ) &last_year, &data_length );

        if ( last_year >= current_year ) {
            RegQueryValueEx ( key, TEXT ( "FirstEntry" ), NULL, NULL, ( LPBYTE ) &first_year, &data_length );

            data_length = sizeof ( REG_TZI_FORMAT );
            RegQueryValueEx ( key, fmt::to_wstring ( current_year ).c_str ( ), NULL, NULL, ( LPBYTE ) &reg_tzi,
                              &data_length );

            tzi.Bias         = reg_tzi.Bias; // UTC = local time + bias
            if ( reg_tzi.StandardDate.wMonth ) {
                tzi.StandardDate = reg_tzi.StandardDate;
                tzi.StandardBias = reg_tzi.StandardBias;
            }
            if ( reg_tzi.DaylightDate.wMonth ) {
                tzi.DaylightDate = reg_tzi.DaylightDate;
                tzi.DaylightBias = reg_tzi.DaylightBias;
            }
        }
    }

    RegCloseKey ( key );

#endif

    return tzi;
}

tzi_t const & get_tzi_utc ( ) noexcept {
    static tzi_t const utc = { 0, TEXT ( "Coordinated Universal Time" ), systime_t{}, 0, TEXT ( "Coordinated Universal Time" ), systime_t{}, 0 };
    return utc;
}

bool has_dst ( tzi_t const & tzi ) noexcept { return tzi.StandardDate.wMonth; }

systime_t get_systime_in_tz ( tzi_t const & tzi_, systime_t const & system_time_ ) noexcept {
    systime_t local_time;
    SystemTimeToTzSpecificLocalTime ( &tzi_, &system_time_, &local_time );
    return local_time;
}

wintime_t get_wintime_in_tz ( tzi_t const & tzi_, wintime_t const & wintime_ ) noexcept {
    return systime_to_wintime ( get_systime_in_tz ( tzi_, wintime_to_systime ( wintime_ ) ) );
}

nixtime_t get_nixtime_in_tz ( tzi_t const & tzi_, nixtime_t const & nixtime_ ) noexcept {
    return systime_to_nixtime ( get_systime_in_tz ( tzi_, nixtime_to_systime ( nixtime_ ) ) );
}

systime_t get_systime_in_tz ( tzi_t const & tzi_ ) noexcept {
    return get_systime_in_tz ( tzi_, systime ( ) );
}

wintime_t get_wintime_in_tz ( tzi_t const & tzi_ ) noexcept {
    return systime_to_wintime ( get_systime_in_tz ( tzi_ ) );
}

nixtime_t get_nixtime_in_tz ( tzi_t const & tzi_ ) noexcept {
    return systime_to_nixtime ( get_systime_in_tz ( tzi_ ) );
}

systime_t get_systime_in_tz ( systime_t const & system_time_ ) noexcept {
    return get_systime_in_tz ( get_tzi_utc ( ), system_time_ );
}

wintime_t get_wintime_in_tz ( wintime_t const & wintime_ ) noexcept { return get_wintime_in_tz ( get_tzi_utc ( ), wintime_ ); }

nixtime_t get_nixtime_in_tz ( nixtime_t const & nixtime_ ) noexcept { return get_nixtime_in_tz ( get_tzi_utc ( ), nixtime_ ); }

int today_year_in_tz ( tzi_t const & tzi_ ) noexcept { return get_systime_in_tz ( tzi_ ).wYear; }
int today_month_in_tz ( tzi_t const & tzi_ ) noexcept { return get_systime_in_tz ( tzi_ ).wMonth; }

void save_timestamps ( ) {
    json const j = g_timestamps;
    std::ofstream o ( g_timestamps_path );
    o << j.dump ( 4 ) << std::endl;
    o.flush ( );
    o.close ( );
}

void load_timestamps ( ) {
    json j;
    std::ifstream i ( g_timestamps_path );
    i >> j;
    i.close ( );
    g_timestamps = j.get<Timestamps> ( );
}

void save_to_file ( json const & j_, std::wstring const & name_ ) {
    std::ofstream o ( g_app_data_path / ( name_ + L".json" ) );
    o << std::setw ( 4 ) << j_ << std::endl;
    o.flush ( );
    o.close ( );
}

void load_from_file ( json & j_, std::wstring const & name_ ) {
    std::ifstream i ( g_app_data_path / ( name_ + L".json" ) );
    i >> j_;
    i.close ( );
}

json load ( fs::path const & file_ ) {
    json a;
    std::ifstream i ( file_ );
    i >> a;
    i.close ( );
    return a;
}

// Return system time from date in UTC.
systime_t date_to_systime ( int const y_, int const m_, int const d_ ) noexcept {
    systime_t st{};
    st.wYear  = y_;
    st.wMonth = m_;
    st.wDay   = d_;
    return get_systime_in_tz ( st );
}

// Return windows time from date in UTC.
wintime_t date_to_wintime ( int const y_, int const m_, int const d_ ) noexcept {
    return systime_to_wintime ( date_to_systime ( y_, m_, d_ ) );
}

int days_since ( int const y_, int const m_, int const d_ ) noexcept {
    return static_cast<int> ( ( get_wintime_in_tz ( wintime ( ) ).i - date_to_wintime ( y_, m_, d_ ).i ) / ( 24ULL * 60ULL * 60ULL * 10'000'000ULL ) );
}

int days_since_winepoch ( ) noexcept {
    return static_cast<int> ( get_wintime_in_tz ( wintime ( ) ).i / ( 24ULL * 60ULL * 60ULL * 10'000'000ULL ) );
}

std::int64_t local_utc_offset_minutes ( ) noexcept {
    wintime_t ft = wintime ( ), lt;
    FileTimeToLocalFileTime ( &ft.filetime, &lt.filetime );
    if ( ft.i > lt.i )
        return +static_cast<std::int64_t> ( ( ft.i - lt.i ) / ( 60ULL * 10'000'000ULL ) );
    else
        return -static_cast<std::int64_t> ( ( lt.i - ft.i ) / ( 60ULL * 10'000'000ULL ) );
}


/*

typedef struct _SYSTEMTIME {

    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;

} systime_t, *PSYSTEMTIME;


typedef struct _TIME_ZONE_INFORMATION {
    LONG       Bias;
    WCHAR      StandardName[32];
    systime_t StandardDate;
    LONG       StandardBias;
    WCHAR      DaylightName[32];
    systime_t DaylightDate;
    LONG       DaylightBias;
} tzi_t, *PTIME_ZONE_INFORMATION, *LPTIME_ZONE_INFORMATION;


typedef struct _TIME_DYNAMIC_ZONE_INFORMATION {
    // The bias is the difference, in minutes, between Coordinated Universal Time (UTC) and local time.
    // UTC = local time + bias.
    LONG Bias;
    WCHAR StandardName[ 32 ];
    systime_t StandardDate;
    LONG StandardBias;
    WCHAR DaylightName[ 32 ];
    systime_t DaylightDate;
    LONG DaylightBias;
    WCHAR TimeZoneKeyName[ 128 ];
    BOOLEAN DynamicDaylightTimeDisabled;
} DYNAMIC_TIME_ZONE_INFORMATION, *PDYNAMIC_TIME_ZONE_INFORMATION;

*/

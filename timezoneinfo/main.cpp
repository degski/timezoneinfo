
// MIT License
//
// Copyright (c) 2019 degski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in_ the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in_ all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

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

#include <array>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <list>
#include <map>
#include <random>
#include <sax/iostream.hpp>
#include <sax/stl.hpp>
#include <sax/string_split.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <fmt/core.h>
#include <fmt/format.h>

#include <tinyxml2.h>

#include "calendar.hpp"
#include "timezoneinfo.hpp"

namespace fs = std::filesystem;

char const * elementToCStr ( tinyxml2::XMLElement const * const element_, char const name_[] ) noexcept {
    char const * out;
    element_->QueryStringAttribute ( name_, &out );
    return out;
}

struct Info {
    std::string name, code;
};

using KeyValue = sax::pair<std::string, Info>;
using Map      = std::vector<KeyValue>;

[[nodiscard]] std::map<std::string, Info> buildIanaToWindowsZonesMap ( fs::path const & path_ ) {
    std::map<std::string, Info> map;
    tinyxml2::XMLDocument doc;
    doc.LoadFile ( path_.string ( ).c_str ( ) );
    tinyxml2::XMLElement const * element = doc.FirstChildElement ( "supplementalData" )
                                               ->FirstChildElement ( "windowsZones" )
                                               ->FirstChildElement ( "mapTimezones" )
                                               ->FirstChildElement ( "mapZone" );
    tinyxml2::XMLElement const * const last_element = element->Parent ( )->LastChildElement ( "mapZone" );
    while ( true ) {
        auto const other     = elementToCStr ( element, "other" );
        auto const territory = elementToCStr ( element, "territory" );
        if ( "001" != territory ) {
            for ( auto & ia : sax::string_split ( std::string_view{ elementToCStr ( element, "type" ) }, " " ) ) {
                if ( "Etc" == ia.substr ( 0u, 3u ) )
                    ia = ia.substr ( 4u, ia.size ( ) - 4 );
                std::string ais{ ia };
                auto it = map.find ( ais );
                if ( std::end ( map ) == it )
                    map.emplace ( std::move ( ais ), Info{ std::string{ other }, std::string{ territory } } );
                else if ( "001" == it->second.code )
                    it->second.code = std::string{ territory };
            }
        }
        if ( element != last_element )
            element = element->NextSiblingElement ( );
        else
            break;
    }
    return map;
}

/*
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

[[nodiscard]] tzi_t get_tzi ( std::string const & desc_ ) noexcept {
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
    REG_TZI_FORMAT reg_tzi_format{};
    tzi_t tzi{};
    // Create URI.
    std::wstring const desc = fmt::to_wstring ( desc_ );
    std::wstring const uri  = std::wstring ( L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\" ) + desc;
    auto result             = RegOpenKeyEx ( HKEY_LOCAL_MACHINE, uri.c_str ( ), 0, KEY_READ, &key );
    assert ( ERROR_SUCCESS == result );
    data_length = sizeof ( REG_TZI_FORMAT );
    RegQueryValueEx ( key, TEXT ( "TZI" ), NULL, NULL, ( LPBYTE ) &reg_tzi_format, &data_length );
    tzi.Bias    = reg_tzi_format.Bias; // UTC = local time + bias.
    data_length = 64;
    RegQueryValueEx ( key, TEXT ( "Std" ), NULL, NULL, ( LPBYTE ) &tzi.StandardName, &data_length );
    if ( reg_tzi_format.StandardDate.wMonth ) {
        tzi.StandardDate = reg_tzi_format.StandardDate;
        tzi.StandardBias = reg_tzi_format.StandardBias;
    }
    if ( reg_tzi_format.DaylightDate.wMonth ) {
        data_length = 64;
        RegQueryValueEx ( key, TEXT ( "Dlt" ), NULL, NULL, ( LPBYTE ) &tzi.DaylightName, &data_length );
        tzi.DaylightDate = reg_tzi_format.DaylightDate;
        tzi.DaylightBias = reg_tzi_format.DaylightBias;
    }
    assert ( not reg_tzi_format.StandardDate.wMonth == not reg_tzi_format.DaylightDate.wMonth );
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
            RegQueryValueEx ( key, fmt::to_wstring ( current_year ).c_str ( ), NULL, NULL, ( LPBYTE ) &reg_tzi_format,
                              &data_length );

            tzi.Bias         = reg_tzi_format.Bias; // UTC = local time + bias
            if ( reg_tzi_format.StandardDate.wMonth ) {
                tzi.StandardDate = reg_tzi_format.StandardDate;
                tzi.StandardBias = reg_tzi_format.StandardBias;
            }
            if ( reg_tzi_format.DaylightDate.wMonth ) {
                tzi.DaylightDate = reg_tzi_format.DaylightDate;
                tzi.DaylightBias = reg_tzi_format.DaylightBias;
            }
        }
    }

    RegCloseKey ( key );

#endif

    return tzi;
}

[[nodiscard]] bool has_dst ( tzi_t const & tzi ) noexcept { return tzi.StandardDate.wMonth; }

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

*/

[[nodiscard]] systime_t get_systime_in_tz ( tzi_t const & tzi_ ) noexcept {
    systime_t system_time = systime ( ), local_time;
    SystemTimeToTzSpecificLocalTime ( &tzi_, &system_time, &local_time );
    return local_time;
}

[[nodiscard]] wintime_t get_wintime_in_tz ( tzi_t const & tzi_ ) noexcept {
    return systime_to_wintime ( get_systime_in_tz ( tzi_ ) );
}

[[nodiscard]] nixtime_t get_nixtime_in_tz ( tzi_t const & tzi_ ) noexcept {
    return wintime_to_nixtime ( get_wintime_in_tz ( tzi_ ) );
}

/*

typedef struct _TIME_ZONE_INFORMATION {
LONG       Bias;
WCHAR      StandardName[32];
systime_t StandardDate;
LONG       StandardBias;
WCHAR      DaylightName[32];
systime_t DaylightDate;
LONG       DaylightBias;
} tzi_t, *PTIME_ZONE_INFORMATION, *LPTIME_ZONE_INFORMATION;

*/

int main ( ) {

    // https://raw.githubusercontent.com/unicode-org/cldr/master/common/supplemental/windowsZones.xml

    // std::map<std::string, Info> map{ buildIanaToWindowsZonesMap ( "Y:/REPOS/timezoneinfo/windowsZones.xml" ) };

    // std::cout << map.size ( ) << nl;

    // for ( auto const & e : map )
    //  std::cout << e.first << " - " << e.second.name << " - " << e.second.code << nl;

    tzi_t tzi1 = get_tzi ( "W. Australia Standard Time" );

    print_nixtime ( get_nixtime_in_tz ( tzi1 ) );

    tzi_t const tzi2 = get_tzi ( "Pacific Standard Time" );

    print_nixtime ( get_nixtime_in_tz ( tzi2 ) );
    print_systime ( get_systime_in_tz ( tzi2 ) );

    return EXIT_SUCCESS;
}


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

#if _WIN32
#    if defined( _DEBUG )
#        pragma comment( lib, "tinyxml2d.lib" )
#        pragma comment( lib, "fmtd.lib" )
#    else
#        pragma comment( lib, "tinyxml2.lib" )
#        pragma comment( lib, "fmt.lib" )
#    endif
#endif

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
    LONG Bias;
    WCHAR StandardName[ 32 ];
    SYSTEMTIME StandardDate;
    LONG StandardBias;
    WCHAR DaylightName[ 32 ];
    SYSTEMTIME DaylightDate;
    LONG DaylightBias;
    WCHAR TimeZoneKeyName[ 128 ];
    BOOLEAN DynamicDaylightTimeDisabled;
} DYNAMIC_TIME_ZONE_INFORMATION, *PDYNAMIC_TIME_ZONE_INFORMATION;
*/

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

} SYSTEMTIME, *PSYSTEMTIME;

*/

[[nodiscard]] TIME_ZONE_INFORMATION get_tzi ( std::string const & desc_ ) {
    // The registry entry for TZI.
    struct REG_TZI_FORMAT {
        LONG Bias;
        LONG StandardBias;
        LONG DaylightBias;
        SYSTEMTIME StandardDate;
        SYSTEMTIME DaylightDate;
    };
    // Variables.
    HKEY key = nullptr;
    DWORD data_length;
    REG_TZI_FORMAT reg_tzi_format{};
    TIME_ZONE_INFORMATION tzi{};
    // Create URI.
    std::wstring const desc = fmt::to_wstring ( desc_ );
    std::wstring const uri  = std::wstring ( L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\" ) + desc;

    RegOpenKeyEx ( HKEY_LOCAL_MACHINE, uri.c_str ( ), 0, KEY_READ, &key );

    data_length = sizeof ( REG_TZI_FORMAT );
    RegQueryValueEx ( key, TEXT ( "TZI" ), NULL, NULL, ( LPBYTE ) &reg_tzi_format, &data_length );

    tzi.Bias = reg_tzi_format.Bias; // UTC = local time + bias

    data_length = 64;
    RegQueryValueEx ( key, TEXT ( "Std" ), NULL, NULL, ( LPBYTE ) &tzi.StandardName, &data_length );

    tzi.StandardDate = reg_tzi_format.StandardDate;
    tzi.StandardBias = reg_tzi_format.StandardBias;

    data_length = 64;
    RegQueryValueEx ( key, TEXT ( "Dlt" ), NULL, NULL, ( LPBYTE ) &tzi.DaylightName, &data_length );

    tzi.DaylightDate = reg_tzi_format.DaylightDate;
    tzi.DaylightBias = reg_tzi_format.DaylightBias;

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
            tzi.StandardDate = reg_tzi_format.StandardDate;
            tzi.StandardBias = reg_tzi_format.StandardBias;
            tzi.DaylightDate = reg_tzi_format.DaylightDate;
            tzi.DaylightBias = reg_tzi_format.DaylightBias;
        }
    }

    RegCloseKey ( key );

#endif

    return tzi;
}

[[nodiscard]] std::tm get_tzi_date ( SYSTEMTIME const & t ) noexcept {
    std::tm date{};
    date.tm_year = today_year ( );
    date.tm_mon  = t.wMonth - 1;
    date.tm_mday = weekday_day ( 0, date.tm_year, t.wMonth, t.wDayOfWeek ) + ( t.wDay - 1 ) * 7;
    if ( date.tm_mday > number_of_days_month ( t.wYear, t.wMonth ) )
        date.tm_mday -= 7;
    date.tm_year -= 1'900;
    date.tm_hour = t.wHour;
    date.tm_min  = t.wMinute;
    date.tm_sec  = 0;
    return date;
}

int main ( ) {

    // https://raw.githubusercontent.com/unicode-org/cldr/master/common/supplemental/windowsZones.xml

    std::map<std::string, Info> map{ buildIanaToWindowsZonesMap ( "Y:/REPOS/timezoneinfo/windowsZones.xml" ) };

    // std::cout << map.size ( ) << nl;

    // for ( auto const & e : map )
    // std::cout << e.first << " - " << e.second.name << " - " << e.second.code << nl;

    TIME_ZONE_INFORMATION tzi = get_tzi ( "W. Europe Standard Time" );

    std::cout << tzi.Bias << nl;

    std::wcout << tzi.StandardName << nl;
    std::cout << tzi.StandardBias << nl;
    std::cout << tzi.StandardDate << nl;

    std::wcout << tzi.DaylightName << nl;
    std::cout << tzi.DaylightBias << nl;
    std::cout << tzi.DaylightDate << nl;

    std::tm t1 = get_tzi_date ( tzi.StandardDate );
    std::cout << std::put_time ( &t1, "%c" ) << '\n';
    std::tm t2 = get_tzi_date ( tzi.DaylightDate );
    std::cout << std::put_time ( &t2, "%c" ) << '\n';

    return EXIT_SUCCESS;
}

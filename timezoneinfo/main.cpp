
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

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <Windows.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <list>
#include <map>
#include <random>
#include <sax/iostream.hpp>
#include <sax/stl.hpp>
#include <sax/string_split.hpp>
#include <sax/utf8conv.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <fmt/core.h>

#include <tinyxml2.h>

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

std::map<std::string, Info> buildIanaToWindowsZonesMap ( fs::path const & path_ ) {
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

template<typename Stream>
[[maybe_unused]] Stream & operator<< ( Stream & os_, SYSTEMTIME const & st_ ) {
    os_ << fmt::format ( "{:04}:{:02}:{:02} {:02}:{:02}:{:02}", st_.wYear, st_.wMonth, st_.wDay, st_.wHour, st_.wMinute,
                         st_.wSecond );
    return os_;
}

TIME_ZONE_INFORMATION get_tzi ( std::string const & desc_ ) {
    // The registry entry for TZI.
    struct REG_TZI_FORMAT {
        LONG Bias;
        LONG StandardBias;
        LONG DaylightBias;
        SYSTEMTIME StandardDate;
        SYSTEMTIME DaylightDate;
    };
    // Variables.
    HKEY hKey = nullptr;
    DWORD dwDataLen;
    REG_TZI_FORMAT reg_tzi_format{};
    TIME_ZONE_INFORMATION tzi{};
    // Create URI.
    std::wstring const desc = sax::utf8_to_utf16 ( desc_ );
    std::wstring const uri  = std::wstring ( L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\" ) + desc;

    RegOpenKeyEx ( HKEY_LOCAL_MACHINE, uri.c_str ( ), 0, KEY_ALL_ACCESS, &hKey );

    dwDataLen = sizeof ( REG_TZI_FORMAT );
    RegQueryValueEx ( hKey, TEXT ( "TZI" ), NULL, NULL, ( LPBYTE ) &reg_tzi_format, &dwDataLen );

    tzi.Bias = reg_tzi_format.Bias; // UTC = local time + bias

    dwDataLen = 64;
    RegQueryValueEx ( hKey, TEXT ( "Std" ), NULL, NULL, ( LPBYTE ) &tzi.StandardName, &dwDataLen );

    tzi.StandardDate = reg_tzi_format.StandardDate;
    tzi.StandardBias = reg_tzi_format.StandardBias;

    dwDataLen = 64;
    RegQueryValueEx ( hKey, TEXT ( "Dlt" ), NULL, NULL, ( LPBYTE ) &tzi.DaylightName, &dwDataLen );

    tzi.DaylightDate = reg_tzi_format.DaylightDate;
    tzi.DaylightBias = reg_tzi_format.DaylightBias;

    RegCloseKey ( hKey );

    return tzi;
}

int main ( ) {
    /*
    std::map<std::string, Info> map{ buildIanaToWindowsZonesMap ( "Y:/REPOS/timezoneinfo/windowsZones.xml" ) };

    std::cout << map.size ( ) << nl;

    for ( auto const & e : map ) {
        std::cout << e.first << " - " << e.second.name << " - " << e.second.code << nl;
    }
    */
    TIME_ZONE_INFORMATION tzi = get_tzi ( "GTB Standard Time" );

    std::cout << tzi.Bias << nl;
    std::cout << tzi.DaylightBias << nl;
    std::cout << tzi.DaylightDate << nl;
    std::wcout << tzi.DaylightName << nl;
    std::cout << tzi.StandardBias << nl;
    std::cout << tzi.StandardDate << nl;
    std::wcout << tzi.StandardName << nl;

    return EXIT_SUCCESS;
}

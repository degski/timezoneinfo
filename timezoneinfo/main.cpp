
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

#include <tinyxml2.h>

#include "timezoneinfo.hpp"

#if _WIN32
#    if defined( _DEBUG )
#        pragma comment( lib, "tinyxml2d.lib" )
#    else
#        pragma comment( lib, "tinyxml2.lib" )
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

// "Y:/REPOS/timezoneinfo/windowsZones.xml"

int main ( ) {

    std::map<std::string, Info> map{ buildIanaToWindowsZonesMap ( "Y:/REPOS/timezoneinfo/windowsZones.xml" ) };

    std::cout << map.size ( ) << nl;

    for ( auto const & e : map ) {
        std::cout << e.first << " - " << e.second.name << " - " << e.second.code << nl;
    }

    return EXIT_SUCCESS;
}

typedef struct _REG_TZI_FORMAT {
    LONG Bias;
    LONG StandardBias;
    LONG DaylightBias;
    SYSTEMTIME StandardDate;
    SYSTEMTIME DaylightDate;
} REG_TZI_FORMAT;
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
// https://stackoverflow.com/a/14184787/646940

std::wstring widen1 ( std::string const & in_ ) {
    // Calculate target buffer size (not including the zero terminator).
    int len = MultiByteToWideChar ( CP_UTF8, MB_ERR_INVALID_CHARS, in_.c_str ( ), ( int ) in_.size ( ), nullptr, 0 );
    if ( len == 0 )
        throw std::runtime_error ( "Invalid character sequence." );
    std::wstring out ( len, 0 );
    // No error checking. We already know, that the conversion will succeed.
    MultiByteToWideChar ( CP_UTF8, MB_ERR_INVALID_CHARS, in_.c_str ( ), ( int ) in_.size ( ), out.data ( ), ( int ) out.size ( ) );
    return out;
}

std::wstring widen2 ( _In_ std::string const & ansi_ ) {
    constexpr std::string::size_type limit = std::numeric_limits<int>::max ( );
    assert ( ansi_.size ( ) < limit );
    int const ansi_byte_size  = static_cast<int> ( ansi_.size ( ) );
    auto const wide_char_size = ::MultiByteToWideChar ( CP_UTF8, MB_PRECOMPOSED, ansi_.c_str ( ), ansi_byte_size, nullptr, 0 );
    std::wstring wide_string ( wide_char_size, L'\0' );
    auto const final_char_size =
        ::MultiByteToWideChar ( CP_UTF8, MB_PRECOMPOSED, ansi_.c_str ( ), ansi_byte_size, wide_string.data ( ), wide_char_size );
    assert ( final_char_size == wide_char_size );
    return wide_string;
}

TIME_ZONE_INFORMATION initTZI ( std::string const & desc_ ) {

    HKEY hKey = nullptr;
    DWORD dwDataLen;
    REG_TZI_FORMAT LOCAL_TZI{};
    TIME_ZONE_INFORMATION TZI{};

#if defined _UNICODE
    auto uri = TEXT ( "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\" ) + widen2 ( desc_ );
#else
    auto uri = TEXT ( "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\" ) + desc_;
#endif

    RegOpenKeyEx ( HKEY_LOCAL_MACHINE, TEXT ( "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\" ), 0, KEY_ALL_ACCESS,
                   &hKey );

    dwDataLen = sizeof ( TZI.DaylightName );
    RegQueryValueEx ( hKey, TEXT ( "Dlt" ), NULL, NULL, ( LPBYTE ) TZI.DaylightName, &dwDataLen );

    dwDataLen = sizeof ( TZI.StandardName );
    RegQueryValueEx ( hKey, TEXT ( "Std" ), NULL, NULL, ( LPBYTE ) TZI.StandardName, &dwDataLen );

    dwDataLen = sizeof ( REG_TZI_FORMAT );
    RegQueryValueEx ( hKey, TEXT ( "TZI" ), NULL, NULL, ( LPBYTE ) &LOCAL_TZI, &dwDataLen );

    TZI.Bias = LOCAL_TZI.Bias;

    TZI.DaylightBias = LOCAL_TZI.DaylightBias;
    TZI.DaylightDate = LOCAL_TZI.DaylightDate;

    TZI.StandardBias = LOCAL_TZI.StandardBias;
    TZI.StandardDate = LOCAL_TZI.StandardDate;

    RegCloseKey ( hKey );

    return TZI;
}

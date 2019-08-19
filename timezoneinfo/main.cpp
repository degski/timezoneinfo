
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

#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <list>
#include <map>
#include <random>
#include <sax/iostream.hpp>
#include <sax/string_split.hpp>
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
    std::string iana, code;
};

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
        auto const other_view     = elementToCStr ( element, "other" );
        auto const territory_view = elementToCStr ( element, "territory" );
        for ( auto & ia : sax::string_split ( std::string_view{ elementToCStr ( element, "type" ) }, " " ) ) {
            if ( "Etc" == ia.substr ( 0u, 3u ) )
                ia = ia.substr ( 4u, ia.size ( ) - 4 );
            map.emplace ( std::string{ ia }, Info{ std::string{ other_view }, std::string{ territory_view } } );
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

    return EXIT_SUCCESS;
}

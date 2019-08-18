
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

std::string_view elementToStringView ( tinyxml2::XMLElement const * element_, char const name_[] ) {
    char const * out;
    element_->QueryStringAttribute ( name_, &out );
    return { out };
}

int main ( ) {

    tinyxml2::XMLDocument doc;

    doc.LoadFile ( "Y:/REPOS/timezoneinfo/windowsZones.xml" );

    tinyxml2::XMLElement const * element = doc.FirstChildElement ( "supplementalData" )
                                               ->FirstChildElement ( "windowsZones" )
                                               ->FirstChildElement ( "mapTimezones" )
                                               ->FirstChildElement ( "mapZone" );

    tinyxml2::XMLElement const * const last = doc.FirstChildElement ( "supplementalData" )
                                                  ->FirstChildElement ( "windowsZones" )
                                                  ->FirstChildElement ( "mapTimezones" )
                                                  ->LastChildElement ( "mapZone" );

    struct Info {
        std::string iana, code;
    };

    std::map<std::string, Info> db;

    while ( true ) {

        auto const other_view     = elementToStringView ( element, "other" );
        auto const territory_view = elementToStringView ( element, "territory" );

        for ( auto const & ia : sax::string_split ( elementToStringView ( element, "type" ), " " ) )
            db.emplace ( std::string{ ia }, Info{ std::string{ other_view }, std::string{ territory_view } } );

        if ( element == last )
            break;
        else
            element = element->NextSiblingElement ( );
    }

    std::cout << db.size ( ) << nl;

    return EXIT_SUCCESS;
}

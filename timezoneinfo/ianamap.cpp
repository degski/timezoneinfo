
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

#include "timezoneinfo.hpp"
#include "zfstream.hpp"

#include <fstream>
#include <map>
#include <sax/iostream.hpp>
#include <sax/stl.hpp>
#include <sax/string_split.hpp>
#include <string>
#include <string_view>

#include <tinyxml2.h>

void download_windowszones ( ) {
    download ( "https://raw.githubusercontent.com/unicode-org/cldr/master/common/supplemental/windowsZones.xml",
               g_windowszones_path );
}

void download_windowszones_alt ( ) {
    download ( "https://raw.githubusercontent.com/mj1856/TimeZoneConverter/master/src/TimeZoneConverter/Data/Mapping.csv.gz",
               g_windowszones_alt_path );
}

char const * element_to_cstr ( tinyxml2::XMLElement const * const element_, char const name_[] ) noexcept {
    char const * out;
    element_->QueryStringAttribute ( name_, &out );
    return out;
}

[[nodiscard]] IanaMap build_iana_to_windowszones_map ( ) {
    IanaMap map;
    tinyxml2::XMLDocument doc;
    if ( not fs::exists ( g_windowszones_path ) ) {
        download_windowszones ( );
        g_timestamps.insert_or_assign ( "last_windowszones_download", wintime ( ).as_uint64 ( ) );
        save_timestamps ( );
    }
    doc.LoadFile ( g_windowszones_path.string ( ).c_str ( ) );
    tinyxml2::XMLElement const * element = doc.FirstChildElement ( "supplementalData" )
                                               ->FirstChildElement ( "windowsZones" )
                                               ->FirstChildElement ( "mapTimezones" )
                                               ->FirstChildElement ( "mapZone" );
    tinyxml2::XMLElement const * const last_element = element->Parent ( )->LastChildElement ( "mapZone" );
    while ( true ) {
        auto const other     = element_to_cstr ( element, "other" );
        auto const territory = element_to_cstr ( element, "territory" );
        if ( std::strncmp ( "001", territory, 3 ) ) {
            for ( auto & ia : sax::string_split ( std::string_view{ element_to_cstr ( element, "type" ) }, " " ) ) {
                // if ( "Etc" == ia.substr ( 0u, 3u ) )
                //     ia = ia.substr ( 4u, ia.size ( ) - 4 );
                IanaMapKey ais{ ia };
                auto it = map.find ( ais );
                if ( std::end ( map ) == it )
                    map.emplace ( std::move ( ais ), IanaMapValue{ std::string{ other }, std::string{ territory } } );
                else if ( std::strncmp ( "001", it->second.code.c_str ( ), 3 ) )
                    it->second.code = std::string{ "001" };
            }
        }
        if ( element != last_element )
            element = element->NextSiblingElement ( );
        else
            break;
    }
    return map;
}

[[nodiscard]] IanaMap build_iana_to_windowszones_alt_map ( ) {
    IanaMap map;
    gzifstream inf;
    char buf[ 512 ];
    if ( not fs::exists ( g_windowszones_alt_path ) ) {
        download_windowszones_alt ( );
        g_timestamps.insert_or_assign ( "last_windowszones_alt_download", wintime ( ).as_uint64 ( ) );
        save_timestamps ( );
    }
    inf.rdbuf ( )->pubsetbuf ( 0, 0 ); // Unbuffered.
    inf.open ( g_windowszones_alt_path.string ( ).c_str ( ), std::ifstream::in );
    while ( inf.getline ( buf, 512 ) ) {
        std::string_view buf_view = buf;
        if ( '\r' == buf_view.back ( ) ) // If \r\n.
            buf_view.remove_suffix ( 1u );
        auto const line = sax::string_split ( buf_view, ',' );
        for ( auto const & ia : sax::string_split ( line[ 2 ], ' ' ) ) {
            IanaMapKey ais{ ia };
            auto const it = map.find ( ais );
            if ( std::end ( map ) == it )
                map.emplace ( std::move ( ais ), IanaMapValue{ std::string{ line[ 0 ] }, std::string{ line[ 1 ] } } );
            else if ( std::strncmp ( "001", line[ 1 ].data ( ), 3 ) )
                it->second.code = std::string{ "001" };
        }
    }
    inf.close ( );
    return map;
}

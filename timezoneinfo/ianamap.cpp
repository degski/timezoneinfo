
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

#include "ianamap.hpp"

#include <fstream>
#include <map>
#include <sax/iostream.hpp>
#include <sax/stl.hpp>
#include <sax/string_split.hpp>
#include <string>
#include <string_view>

#include <curlpp/cURLpp.hpp>

#include <curlpp/Easy.hpp>
#include <curlpp/Infos.hpp>
#include <curlpp/Options.hpp>

#include <tinyxml2.h>

char const * elementToCStr ( tinyxml2::XMLElement const * const element_, char const name_[] ) noexcept {
    char const * out;
    element_->QueryStringAttribute ( name_, &out );
    return out;
}

[[nodiscard]] IanaMap buildIanaToWindowsZonesMap ( ) {
    IanaMap map;
    tinyxml2::XMLDocument doc;
    if ( not fs::exists ( g_windows_zones_path ) ) {
        download_windows_zones ( );
        g_timestamps.insert_or_assign ( "last_windowszones_download", wintime ( ).i );
        save_timestamps ( );
    }
    doc.LoadFile ( g_windows_zones_path.string ( ).c_str ( ) );
    tinyxml2::XMLElement const * element = doc.FirstChildElement ( "supplementalData" )
                                               ->FirstChildElement ( "windowsZones" )
                                               ->FirstChildElement ( "mapTimezones" )
                                               ->FirstChildElement ( "mapZone" );
    tinyxml2::XMLElement const * const last_element = element->Parent ( )->LastChildElement ( "mapZone" );
    while ( true ) {
        auto const other     = elementToCStr ( element, "other" );
        auto const territory = elementToCStr ( element, "territory" );
        if ( std::strncmp ( "001", territory, 3 ) ) {
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

void download_windows_zones ( ) {
    std::ofstream o ( g_windows_zones_path );
    try {
        curlpp::Easy request;
        request.setOpt<curlpp::options::WriteStream> ( &o );
        request.setOpt<curlpp::options::Encoding> ( "deflate" );
        request.setOpt<curlpp::options::Url> (
            "https://raw.githubusercontent.com/unicode-org/cldr/master/common/supplemental/windowsZones.xml" );
        request.perform ( );
    }
    catch ( curlpp::RuntimeError & e ) {
        std::cout << e.what ( ) << std::endl;
    }
    catch ( curlpp::LogicError & e ) {
        std::cout << e.what ( ) << std::endl;
    }
    o.flush ( );
    o.close ( );
}

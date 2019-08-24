
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
#include <cstdlib>

#include <array>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <list>
#include <map>
#include <random>
#include <sax/iostream.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

int main ( ) {

    init ( );

    for ( auto const & e : g_iana )
        std::cout << e.first << " - " << e.second.name << " - " << e.second.code << nl;

    tzi_t tzi1 = get_tzi ( "Australia/Perth" );

    print_wintime ( get_wintime_in_tz ( tzi1 ) );

    tzi_t const tzi2 = get_tzi ( "America/Los_Angeles" );

    nixtime_t const nt = get_nixtime_in_tz ( tzi2 );

    print_nixtime ( nt );
    print_wintime ( get_wintime_in_tz ( tzi2 ) );
    print_systime ( get_systime_in_tz ( tzi2 ) );


    std::cout << local_utc_offset_minutes ( ) << nl;

    return EXIT_SUCCESS;
}

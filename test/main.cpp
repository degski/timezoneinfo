
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
#include <sax/string_split.hpp>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

int main ( ) {

    init ( );

    std::cout << g_iana.size ( ) << nl;

    for ( auto const & e : g_iana )
        std::cout << e.first << " - " << e.second.name << " - " << e.second.code << nl;

    tzi_t tzi1 = get_tzi ( "Australia/Perth" );

    print_wintime ( get_wintime_in_tz ( tzi1 ) );

    tzi_t const tzi2 = get_tzi ( "America/Los_Angeles" );

    nixtime_t const nt = get_nixtime_in_tz ( tzi2 );

    print_nixtime ( nt );
    print_wintime ( get_wintime_in_tz ( tzi2 ) );
    print_systime ( get_systime_in_tz ( tzi2 ) );

    wintime_t t1 = wintime ( );

    std::cout << t1 << nl;

    t1.set_offset ( 165 );

    std::cout << t1 << nl;

    std::cout << t1.get_offset ( ) << nl;

    return EXIT_SUCCESS;
}

std::uint16_t fletcher16 ( std::uint8_t const * const data, int const count ) {
    std::uint16_t sum1 = 0, sum2 = 0;
    for ( int index = 0; index < count; ++index ) {
        sum1 = ( sum1 + data[ index ] ) % 255;
        sum2 = ( sum2 + sum1 ) % 255;
    }
    return ( sum2 << 8 ) | sum1;
}

std::uint8_t fletcher8 ( std::uint8_t const * const data, int const count ) {
    std::uint8_t sum1 = 0, sum2 = 0;
    for ( int index = 0; index < count; ++index ) {
        sum1 = ( sum1 + data[ index ] ) % 255;
        sum2 = ( sum2 + sum1 ) % 255;
    }
    return ( sum2 << 4 ) | sum1;
}

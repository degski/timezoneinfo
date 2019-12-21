
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

void print_calendar ( int const y_, int const m_ ) noexcept { std::cout << calendar ( y_, m_ ); }

struct Week {

    int week_num;
    int days[ 7 ]{ };
};

struct Month {

    int year, month;
    char name[ 16 ]{ };
    std::vector<Week> weeks;

    Month ( ) { weeks.reserve ( 6 ); }
};

[[nodiscard]] Month calendar_ ( int const y_, int const m_ ) noexcept {
    Month m;
    m.year  = y_;
    m.month = m_;
    std::memcpy ( m.name, month_of_the_year[ m_ - 1 ], std::strlen ( month_of_the_year[ m_ - 1 ] ) );
    int w = year_weeks ( y_, m_, 1 ), f = day_week ( y_, m_, 1 );
    // first line.
    m.weeks.emplace_back ( );
    m.weeks.back ( ).week_num = w++;
    int c                     = 1;
    for ( ; f < 7; ++f )
        m.weeks.back ( ).days[ f ] = c++;
    // middle lines.
    int const l = days_month ( y_, m_ );
    while ( c <= ( l - 7 ) ) {
        m.weeks.emplace_back ( );
        m.weeks.back ( ).week_num = w++;
        for ( int i = 0; i < 7; ++i, ++c )
            m.weeks.back ( ).days[ i ] = c;
    }
    // last line (iff applicable).
    if ( c <= l ) {
        m.weeks.emplace_back ( );
        m.weeks.back ( ).week_num = w++;
        int i                     = 0;
        do {
            m.weeks.back ( ).days[ i++ ] = c++;
        } while ( c <= l );
    }
    return m;
}

int main ( ) {

    init_alt ( );

    Month m = calendar_ ( 2020, 5 );

    std::cout << m.name << nl;
    std::cout << m.year << ' ' << m.month << nl;
    std::cout << m.weeks.size ( ) << nl;
    std::cout << m.weeks[ 0 ].week_num << nl;

    /*

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

    */

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

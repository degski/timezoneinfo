
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

#include "calendar.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include <sax/iostream.hpp>

#include <fmt/core.h>
#include <fmt/format.h>

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

    } systime_t, *PSYSTEMTIME;
*/

#define NS_SINCE_1970 116'444'736'000'000'000ULL
#define U_10M 10'000'000ULL
#define S_10M 10'000'000LL

nixtime_t wintime_to_nixtime ( wintime_t const wintime_ ) noexcept {
    return ( wintime_.i > NS_SINCE_1970 ? wintime_.i - NS_SINCE_1970 : NS_SINCE_1970 - wintime_.i ) / U_10M;
}

wintime_t nixtime_to_wintime ( nixtime_t const nixtime_ ) noexcept {
    wintime_t wt;
    wt.i = static_cast<std::uint64_t> ( static_cast<std::int64_t> ( nixtime_ ) * S_10M ) + NS_SINCE_1970;
    return wt;
}

wintime_t wintime ( ) noexcept {
    wintime_t wt;
    GetSystemTimeAsFileTime ( &wt.filetime );
    return wt;
}

systime_t systime ( ) noexcept {
    systime_t st{};
    GetSystemTime ( &st );
    return st;
}

systime_t localtime ( ) noexcept {
    systime_t lt{};
    GetLocalTime ( &lt );
    return lt;
}

nixtime_t nixtime ( ) noexcept { return wintime_to_nixtime ( wintime ( ) ); }

systime_t wintime_to_systime ( wintime_t const wintime_ ) noexcept {
    systime_t st{};
    FileTimeToSystemTime ( &wintime_.filetime, &st );
    return st;
}

systime_t nixtime_to_systime ( nixtime_t const nixtime_ ) noexcept {
    return wintime_to_systime ( nixtime_to_wintime ( nixtime_ ) );
}

wintime_t systime_to_wintime ( systime_t const & systime_ ) noexcept {
    wintime_t wt;
    SystemTimeToFileTime ( &systime_, &wt.filetime );
    return wt;
}

nixtime_t systime_to_nixtime ( systime_t const & systime_ ) noexcept {
    return wintime_to_nixtime ( systime_to_wintime ( systime_ ) );
}

filtime_t wintime_to_filtime ( wintime_t const wintime_ ) noexcept { return wintime_.filetime; }
filtime_t nixtime_to_filtime ( nixtime_t const nixtime_ ) noexcept { return nixtime_to_wintime ( nixtime_ ).filetime; }

std::tm systime_to_tm ( systime_t const & systime_ ) noexcept {
    std::tm tmp{};
    tmp.tm_sec                        = systime_.wSecond;
    tmp.tm_min                        = systime_.wMinute;
    tmp.tm_hour                       = systime_.wHour;
    tmp.tm_mday                       = systime_.wDay;
    tmp.tm_mon                        = systime_.wMonth - 1;
    int const y_                      = systime_.wYear;
    tmp.tm_year                       = y_ - 1900;
    tmp.tm_wday                       = systime_.wDayOfWeek;
    constexpr int const cum_dim[ 12 ] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    tmp.tm_yday                       = cum_dim[ tmp.tm_mon ] + tmp.tm_mday +
                  ( ( tmp.tm_mon > 1 ) * ( ( ( y_ % 4 == 0 ) and ( y_ % 100 != 0 ) ) or ( y_ % 400 == 0 ) ) );
    tmp.tm_isdst = 0;
    return tmp;
}

systime_t tm_to_systime ( std::tm const & tm_ ) noexcept {
    systime_t tmp{};
    tmp.wYear         = tm_.tm_year + 1'900;
    tmp.wMonth        = tm_.tm_mon + 1;
    tmp.wDayOfWeek    = tm_.tm_wday;
    tmp.wDay          = tm_.tm_mday;
    tmp.wHour         = tm_.tm_hour;
    tmp.wMinute       = tm_.tm_min;
    tmp.wSecond       = tm_.tm_sec;
    tmp.wMilliseconds = 0;
    return tmp;
}

#undef NS_SINCE_1970
#undef U_10M
#undef S_10M

/*
typedef struct _TIME_DYNAMIC_ZONE_INFORMATION {
    LONG Bias;
    WCHAR StandardName[ 32 ];
    systime_t StandardDate;
    LONG StandardBias;
    WCHAR DaylightName[ 32 ];
    systime_t DaylightDate;
    LONG DaylightBias;
    WCHAR TimeZoneKeyName[ 128 ];
    BOOLEAN DynamicDaylightTimeDisabled;
} DYNAMIC_TIME_ZONE_INFORMATION, *PDYNAMIC_TIME_ZONE_INFORMATION;
*/

#if _WIN32
#    define timegm _mkgmtime
#endif

int today_year ( ) noexcept { return systime ( ).wYear; }
int today_month ( ) noexcept { return systime ( ).wMonth; }
int today_day ( ) noexcept { return systime ( ).wDay; }

bool is_leap_year ( int const y_ ) noexcept { return ( ( y_ % 4 == 0 ) and ( y_ % 100 != 0 ) ) or ( y_ % 400 == 0 ); }

// Returns the number of days for the given m_ (month) in the given y_ (year).
int days_month ( int const y_, int const m_ ) noexcept {
    return m_ != 2 ? 30 + ( ( m_ + ( m_ > 7 ) ) % 2 ) : 28 + is_leap_year ( y_ );
}

int year_days ( int const y_, int const m_, int const d_ ) noexcept { // normal counting.
    assert ( d_ <= days_month ( y_, m_ ) );
    constexpr short const cum_dim[ 12 ] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    return cum_dim[ m_ - 1 ] + d_ + ( ( m_ > 2 ) * ( ( ( y_ % 4 == 0 ) and ( y_ % 100 != 0 ) ) or ( y_ % 400 == 0 ) ) );
}

int year_weeks ( int const y_, int const m_, int const d_ ) noexcept { // normal counting.
    assert ( d_ <= days_month ( y_, m_ ) );
    constexpr short const cum_dim[ 12 ] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    return ( ( ( cum_dim[ m_ - 1 ] + d_ + ( ( m_ > 2 ) * ( ( ( y_ % 4 == 0 ) and ( y_ % 100 != 0 ) ) or ( y_ % 400 == 0 ) ) ) ) -
               weekday_day ( 0, y_, 1, 0 ) ) /
             7 ) +
           1;
}

int day_week ( int y_, int m_, int d_ ) noexcept {
    // calendar_system = 1 for Gregorian Calendar, 0 for Julian Calendar
    constexpr int const calendar_system = 1;
    if ( m_ < 3 )
        m_ += 12, y_ -= 1;
    return ( d_ + ( m_ << 1 ) + ( 6 * ( m_ + 1 ) / 10 ) + y_ + ( y_ >> 2 ) - ( y_ / 100 ) + ( y_ / 400 ) + calendar_system ) % 7;
}

int first_weekday ( int const y_, int const m_ ) noexcept { return day_week ( y_, m_, 1 ); }

int next_first_weekday ( int y_, int m_ ) noexcept { return m_ != 12 ? day_week ( y_, ++m_, 1 ) : day_week ( ++y_, 1, 1 ); }

int last_weekday ( int y_, int m_ ) noexcept {
    return ( ( m_ != 12 ) ? ( ( day_week ( y_, ++m_, 1 ) + 6 ) % 7 ) : ( ( day_week ( ++y_, 1, 1 ) + 6 ) % 7 ) );
}

std::time_t last_weekday_time ( int const y_, int const m_, int const w_ ) noexcept {
    std::tm date{};
    date.tm_year = y_ - 1'900; // two digit y_!
    date.tm_mon  = m_ - 1;
    date.tm_mday = last_weekday_day ( y_, m_, w_ );
    return timegm ( &date );
}

bool is_weekend ( int const y_, int const m_, int const d_ ) noexcept {
    int const dow = day_week ( y_, m_, d_ );
    return dow == 0 or dow == 6;
}

bool is_weekday ( int const y_, int const m_, int const d_ ) noexcept { return not is_weekend ( y_, m_, d_ ); }

bool is_today_weekend ( ) noexcept {
    systime_t const st = systime ( );
    return is_weekend ( st.wYear, st.wMonth, st.wDay );
}

bool is_today_weekday ( ) noexcept {
    systime_t const st = systime ( );
    return is_weekday ( st.wYear, st.wMonth, st.wDay );
}

void print_nixtime ( nixtime_t const rawtime_ ) noexcept { std::printf ( "%s", std::asctime ( std::gmtime ( &rawtime_ ) ) ); }

void print_wintime ( wintime_t const rawtime_ ) noexcept { print_nixtime ( wintime_to_nixtime ( rawtime_ ) ); }

void print_systime ( systime_t const & st_ ) noexcept {
    print_systime ( std::cout, st_ );
    std::cout << nl;
}

template<typename Stream>
void print_systime ( Stream & os_, systime_t const & st_ ) noexcept {
    // Thu Aug 22 13:41:12 2019
    os_ << fmt::format ( "{} {} {} {:02}:{:02}:{:02} {}", dow[ st_.wDayOfWeek ], moy[ st_.wMonth - 1 ], st_.wDay, st_.wHour,
                         st_.wMinute, st_.wSecond, st_.wYear );
}

int first_weekday_day ( int const y_, int const m_, int const w_ ) noexcept {
    return ( 1 + ( ( 7 - first_weekday ( y_, m_ ) + w_ ) % 7 ) );
}

// Get the month day for the n_-th [ 1, 5 ] day_week w_.
int weekday_day ( int const n_, int const y_, int const m_, int const w_ ) noexcept {
    assert ( n_ >= 0 );
    assert ( n_ <= 5 );
    int const day = 1 + ( 7 - first_weekday ( y_, m_ ) + w_ ) % 7 + ( n_ - 1 ) * 7;
    return n_ < 5 ? day : day > days_month ( y_, m_ ) ? day - 7 : day;
}

int last_weekday_day ( int const y_, int const m_, int const w_ ) noexcept { return weekday_day ( 5, y_, m_, w_ ); }

int days_since ( int const y_, int const m_, int const d_ ) noexcept {
    std::time_t now = std::time ( nullptr );
    std::tm date{};
    date.tm_year     = y_ - 1'900;
    date.tm_mon      = m_ - 1;
    date.tm_mday     = d_;
    std::time_t then = std::mktime ( &date );
    return ( int ) std::difftime ( now, then ) / ( 24 * 60 * 60 );
}

// Return epoch from date.
std::time_t date_to_epoch ( int const y_, int const m_, int const d_ ) noexcept {
    std::tm tm{};
    tm.tm_year = y_ - 1900;
    tm.tm_mon  = m_ - 1;
    tm.tm_mday = d_;
    return timegm ( &tm );
}

int local_utc_offset_minutes ( ) noexcept {
    std::time_t t  = std::time ( nullptr );
    std::tm * locg = std::localtime ( &t );
    std::tm locl;
    std::memcpy ( &locl, locg, sizeof ( std::tm ) );
    return static_cast<int> ( timegm ( locg ) - mktime ( &locl ) ) / 60;
}

#if _WIN32
#    undef timegm
#endif

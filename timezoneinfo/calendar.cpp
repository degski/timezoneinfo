﻿
// MIT License
//
// Copyright (c) 2019 degski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in_ the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in_ all
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

#define NS_SINCE_1970 116444736000000000ULL
#define U_10M 10000000ULL
#define S_10M 10000000LL

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

nixtime_t nixtime ( ) noexcept { return wintime_to_nixtime ( wintime ( ) ); }

SYSTEMTIME wintime_to_systemtime ( wintime_t const wintime_ ) noexcept {
    SYSTEMTIME st{};
    FileTimeToSystemTime ( &wintime_.filetime, &st );
    return st;
}

SYSTEMTIME nixtime_to_systemtime ( nixtime_t const nixtime_ ) noexcept {
    return wintime_to_systemtime ( nixtime_to_wintime ( nixtime_ ) );
}

wintime_t systemtime_to_wintime ( SYSTEMTIME const & systemtime_ ) noexcept {
    wintime_t wt;
    SystemTimeToFileTime ( &systemtime_, &wt.filetime );
    return wt;
}

nixtime_t systemtime_to_nixtime ( SYSTEMTIME const & systemtime_ ) noexcept {
    return wintime_to_nixtime ( systemtime_to_wintime ( systemtime_ ) );
}

FILETIME wintime_to_filetime ( wintime_t const wintime_ ) noexcept { return wintime_.filetime; }
FILETIME nixtime_to_filetime ( nixtime_t const nixtime_ ) noexcept { return nixtime_to_wintime ( nixtime_ ).filetime; }

wintime_t filetime_to_wintime ( FILETIME const filetime_ ) noexcept { return { filetime_ }; }
nixtime_t filetime_to_nixtime ( FILETIME const filetime_ ) noexcept {
    return wintime_to_nixtime ( filetime_to_wintime ( filetime_ ) );
}

std::tm systemtime_to_tm ( SYSTEMTIME const & systemtime_ ) noexcept {
    std::tm tmp{};
    tmp.tm_sec                        = systemtime_.wSecond;
    tmp.tm_min                        = systemtime_.wMinute;
    tmp.tm_hour                       = systemtime_.wHour;
    tmp.tm_mday                       = systemtime_.wDay;
    tmp.tm_mon                        = systemtime_.wMonth - 1;
    int const y_                      = systemtime_.wYear;
    tmp.tm_year                       = y_ - 1900;
    tmp.tm_wday                       = systemtime_.wDayOfWeek;
    constexpr int const cum_dim[ 12 ] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    tmp.tm_yday                       = cum_dim[ tmp.tm_mon ] + tmp.tm_mday +
                  ( ( tmp.tm_mon > 1 ) * ( ( ( y_ % 4 == 0 ) and ( y_ % 100 != 0 ) ) or ( y_ % 400 == 0 ) ) );
    tmp.tm_isdst = 0;
    return tmp;
}

SYSTEMTIME tm_to_systemtime ( std::tm const & tm_ ) noexcept {
    SYSTEMTIME tmp{};
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
    SYSTEMTIME StandardDate;
    LONG StandardBias;
    WCHAR DaylightName[ 32 ];
    SYSTEMTIME DaylightDate;
    LONG DaylightBias;
    WCHAR TimeZoneKeyName[ 128 ];
    BOOLEAN DynamicDaylightTimeDisabled;
} DYNAMIC_TIME_ZONE_INFORMATION, *PDYNAMIC_TIME_ZONE_INFORMATION;
*/

#if _WIN32
#    define timegm _mkgmtime
#endif

int today_year ( ) noexcept {
    std::time_t const now      = std::time ( nullptr );
    std::tm const * const date = std::gmtime ( &now );
    return date->tm_year + 1'900;
}

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

void set_tm_utc ( std::tm * tm_ ) noexcept {
    std::time_t rawtime = std::time ( nullptr );
    tm_                 = std::gmtime ( &rawtime );
}

bool is_weekday ( int const y_, int const m_, int const d_ ) noexcept {
    int const dow = day_week ( y_, m_, d_ );
    return not( dow == 0 or dow == 6 );
}

bool is_weekend ( int const y_, int const m_, int const d_ ) noexcept { return not is_weekday ( y_, m_, d_ ); }

int today_monthday ( ) noexcept {
    std::time_t now = std::time ( nullptr );
    std::tm date;
    localtime_s ( &date, &now );
    return date.tm_wday;
}

bool is_today_weekday ( ) noexcept {
    int const dow = today_monthday ( );
    return not( dow == 0 or dow == 6 );
}

void print_date_time_t ( std::time_t const rawtime ) noexcept {
    std::tm * ptm = std::gmtime ( &rawtime );
    std::printf ( "%02i:%02i:%02i, %s %02i.%02i.%4i", ptm->tm_hour, ptm->tm_min, ptm->tm_sec, dow[ ptm->tm_wday ], ptm->tm_mday,
                  ptm->tm_mon + 1, ptm->tm_year + 1'900 );
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

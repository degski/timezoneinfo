
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

#pragma once

#if _WIN32
#    if defined( _DEBUG )
#        pragma comment( lib, "fmtd.lib" )
#    else
#        pragma comment( lib, "fmt.lib" )
#    endif
#endif

#ifndef NOMINMAX
#    define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#    define _CRT_SECURE_NO_WARNINGS
#endif

#include <Windows.h>

#include <cassert>
#include <cstdint>
#include <ctime>

using nixtime_t = std::time_t; // Signed 64-bit value on Windows x64.

using systime_t = SYSTEMTIME;
using filtime_t = FILETIME;

struct alignas ( 8 ) wintime_t {

    filtime_t * data ( ) noexcept { return &value; }
    filtime_t const * data ( ) const noexcept { return &value; }

    inline std::uint64_t & as_uint64 ( ) noexcept { return *reinterpret_cast<std::uint64_t *> ( this ); }
    inline std::uint64_t const & as_uint64 ( ) const noexcept { return *reinterpret_cast<std::uint64_t const *> ( this ); }

    void set_utc ( ) noexcept { *reinterpret_cast<char *> ( this ) = 0; }

    void set_offset ( int const minutes_ ) noexcept {
        assert ( not( minutes_ % 15 ) );
        *reinterpret_cast<char *> ( this ) = static_cast<char> ( minutes_ / 15 );
    }
    int get_offset ( ) const noexcept { return static_cast<int> ( *reinterpret_cast<char const *> ( this ) ) * 15; }

    private:
    filtime_t value{};
};

[[nodiscard]] nixtime_t wintime_to_nixtime ( wintime_t const wintime_ ) noexcept;
[[nodiscard]] wintime_t nixtime_to_wintime ( nixtime_t const nixtime_ ) noexcept;

[[nodiscard]] wintime_t wintime ( ) noexcept;
[[nodiscard]] nixtime_t nixtime ( ) noexcept;
[[nodiscard]] systime_t systime ( ) noexcept;
[[nodiscard]] systime_t localtime ( ) noexcept;

[[nodiscard]] systime_t wintime_to_systime ( wintime_t const wintime_ ) noexcept;
[[nodiscard]] systime_t nixtime_to_systime ( nixtime_t const nixtime_ ) noexcept;

[[nodiscard]] wintime_t systime_to_wintime ( systime_t const & systime_ ) noexcept;
[[nodiscard]] nixtime_t systime_to_nixtime ( systime_t const & systime_ ) noexcept;

[[nodiscard]] std::tm systime_to_tm ( systime_t const & systime_ ) noexcept;
[[nodiscard]] systime_t tm_to_systime ( std::tm const & tm_ ) noexcept;

inline constexpr char const * dow[ 7 ]             = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
inline constexpr char const * day_of_the_week[ 7 ] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saterday"
};
inline constexpr char const * moy[ 12 ] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
inline constexpr char const * month_of_the_year[ 12 ] = { "January", "February", "March",     "April",   "May",      "June",
                                                          "July",    "August",   "September", "October", "November", "December" };
// Returns the year (UTC).
[[nodiscard]] int today_year ( ) noexcept;
[[nodiscard]] int today_month ( ) noexcept;
[[nodiscard]] int today_day ( ) noexcept;

[[nodiscard]] bool is_leap_year ( int const y_ ) noexcept;
// Returns the number of days for the given m_ (month) in the given y_ (year).
[[nodiscard]] int days_month ( int const y_, int const m_ ) noexcept;
// Returns the number of days YTD.
[[nodiscard]] int year_days ( int const y_, int const m_, int const d_ ) noexcept;
// Returns the number of weeks YTD.
[[nodiscard]] int year_weeks ( int const y_, int const m_, int const d_ ) noexcept;
// Returns the day of the week for a certain date, 0 == Sunday.
[[nodiscard]] int day_week ( int y_, int m_, int d_ ) noexcept;
// Return the first non-weekend day [a weekday, as no good antonym exists] in the month.
[[nodiscard]] int first_weekday ( int const y_, int const m_ ) noexcept;
// Return the last non-weekend day [a weekday, as no good antonym exists] in the month.
[[nodiscard]] int last_weekday ( int y_, int m_ ) noexcept;
// Return the first non-weekend day [a weekday, as no good antonym exists] in the month in UTC.
[[nodiscard]] std::time_t last_weekday_time ( int const y_, int const m_, int const w_ ) noexcept;

[[nodiscard]] bool is_weekday ( int const y_, int const m_, int const d_ ) noexcept;
[[nodiscard]] bool is_weekend ( int const y_, int const m_, int const d_ ) noexcept;

[[nodiscard]] bool is_today_weekend ( ) noexcept;
[[nodiscard]] bool is_today_weekday ( ) noexcept;

// Get the month day for the n_-th [ 1, 5 ] day_week w_.
[[nodiscard]] int weekday_day ( int const n_, int const y_, int const m_, int const w_ ) noexcept;
[[nodiscard]] int last_weekday_day ( int const y_, int const m_, int const w_ ) noexcept;

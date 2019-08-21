
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

#include <ctime>

#include <fmt/core.h>

union wintime_t {
    FILETIME filetime;
    std::uint64_t i;
};

using nixtime_t = std::time_t; // Signed 64-bit value on Windows x64.

[[nodiscard]] nixtime_t wintime_to_nixtime ( wintime_t const wintime_ ) noexcept;
[[nodiscard]] wintime_t nixtime_to_wintime ( nixtime_t const nixtime_ ) noexcept;

[[nodiscard]] wintime_t wintime ( ) noexcept;
[[nodiscard]] nixtime_t nixtime ( ) noexcept;

[[nodiscard]] SYSTEMTIME wintime_to_systemtime ( wintime_t const wintime_ ) noexcept;
[[nodiscard]] SYSTEMTIME nixtime_to_systemtime ( nixtime_t const nixtime_ ) noexcept;

[[nodiscard]] FILETIME wintime_to_filetime ( wintime_t const wintime_ ) noexcept;
[[nodiscard]] FILETIME nixtime_to_filetime ( nixtime_t const nixtime_ ) noexcept;

[[nodiscard]] wintime_t systemtime_to_wintime ( SYSTEMTIME const & systemtime_ ) noexcept;
[[nodiscard]] nixtime_t systemtime_to_nixtime ( SYSTEMTIME const & systemtime_ ) noexcept;

[[nodiscard]] wintime_t filetime_to_wintime ( FILETIME const filetime_ ) noexcept;
[[nodiscard]] nixtime_t filetime_to_nixtime ( FILETIME const filetime_ ) noexcept;

[[nodiscard]] std::tm systemtime_to_tm ( SYSTEMTIME const & systemtime_ ) noexcept;
[[nodiscard]] SYSTEMTIME tm_to_systemtime ( std::tm const & tm_ ) noexcept;

inline constexpr char const * dow[ 7 ]             = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
inline constexpr char const * day_of_the_week[ 7 ] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saterday"
};
inline constexpr char const * moy[ 12 ] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
inline constexpr char const * month_of_the_year[ 12 ] = { "January", "February", "March",     "April",   "May",      "June",
                                                          "July",    "August",   "September", "October", "November", "December" };
// Returns the year.
[[nodiscard]] int today_year ( ) noexcept;
// Returns todays day in the month.
[[nodiscard]] int today_monthday ( ) noexcept;

[[nodiscard]] bool is_leap_year ( int const y_ ) noexcept;
// Returns the number of days for the given m_ (month) in_ the given y_ (year).
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
void set_tm_utc ( std::tm * tm_ ) noexcept;
[[nodiscard]] bool is_weekday ( int const y_, int const m_, int const d_ ) noexcept;
[[nodiscard]] bool is_weekend ( int const y_, int const m_, int const d_ ) noexcept;

[[nodiscard]] bool is_today_weekday ( ) noexcept;
void print_date_time_t ( std::time_t const rawtime ) noexcept;
// Get the month day for the n_-th [ 1, 5 ] day_week w_.
[[nodiscard]] int weekday_day ( int const n_, int const y_, int const m_, int const w_ ) noexcept;
[[nodiscard]] int last_weekday_day ( int const y_, int const m_, int const w_ ) noexcept;
[[nodiscard]] int days_since ( int const y_, int const m_, int const d_ ) noexcept;

// Return epoch (unix-time) from date.
[[nodiscard]] std::time_t date_to_epoch ( int const y_, int const m_, int const d_ ) noexcept;

[[nodiscard]] int local_utc_offset_minutes ( ) noexcept;

template<typename Stream>
[[maybe_unused]] Stream & operator<< ( Stream & os_, SYSTEMTIME const & systemtime_ ) {
    os_ << fmt::format ( "{:04} {:02} {:1} {:02} {:02}:{:02}", systemtime_.wYear, systemtime_.wMonth, systemtime_.wDayOfWeek,
                         systemtime_.wDay, systemtime_.wHour, systemtime_.wMinute );
    return os_;
}

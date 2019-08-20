
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

inline constexpr char const * dow[ 7 ]             = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
inline constexpr char const * day_of_the_week[ 7 ] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saterday"
};
inline constexpr char const * moy[ 12 ] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
inline constexpr char const * month_of_the_year[ 12 ] = { "January", "February", "March",     "April",   "May",      "June",
                                                          "July",    "August",   "September", "October", "November", "December" };

[[nodiscard]] int today_year ( ) noexcept;
[[nodiscard]] bool is_leap_year ( int const y_ ) noexcept;
// Returns the number of days for the given m_ (month) in_ the given y_ (year).
[[nodiscard]] int number_of_days_month ( int const y_, int const m_ ) noexcept;
[[nodiscard]] int number_of_days_ytd ( int const y_, int const m_, int const d_ ) noexcept;
[[nodiscard]] int number_of_weeks_ytd ( int const y_, int const m_, int const d_ ) noexcept;
[[nodiscard]] int weekday ( int y_, int m_, int d_ ) noexcept;
[[nodiscard]] int first_weekday_month ( int const y_, int const m_ ) noexcept;
[[nodiscard]] int last_weekday_month ( int y_, int m_ ) noexcept;
[[nodiscard]] std::time_t time_last_weekday_month ( int const y_, int const m_, int const w_ ) noexcept;
void set_tm_utc ( std::tm * tm_ ) noexcept;
[[nodiscard]] bool is_workweek ( int const y_, int const m_, int const d_ ) noexcept;
[[nodiscard]] bool is_weekend ( int const y_, int const m_, int const d_ ) noexcept;
[[nodiscard]] int today_weekday ( ) noexcept;
[[nodiscard]] bool is_today_workweek ( ) noexcept;
void print_date_time_t ( std::time_t const rawtime ) noexcept;
// Get the month day for the n_-th (base 0) weekday w_.
[[nodiscard]] int weekday_day ( int const n_, int const y_, int const m_, int const w_ ) noexcept;
[[nodiscard]] int last_weekday_day ( int const y_, int const m_, int const w_ ) noexcept;
[[nodiscard]] int number_of_days_since ( int const y_, int const m_, int const d_ ) noexcept;
[[nodiscard]] std::tm systemtime_to_tm ( SYSTEMTIME const & in_ ) noexcept;
[[nodiscard]] SYSTEMTIME tm_to_systemtime ( std::tm const & in_ ) noexcept;

template<typename Stream>
[[maybe_unused]] Stream & operator<< ( Stream & os_, SYSTEMTIME const & st_ ) {
    os_ << fmt::format ( "{:04} {:02} {:1} {:02} {:02}:{:02}", st_.wYear, st_.wMonth, st_.wDayOfWeek, st_.wDay, st_.wHour,
                         st_.wMinute );
    return os_;
}

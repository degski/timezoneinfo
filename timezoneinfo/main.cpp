
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

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <Windows.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <list>
#include <map>
#include <random>
#include <sax/iostream.hpp>
#include <sax/stl.hpp>
#include <sax/string_split.hpp>
#include <sax/utf8conv.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include <fmt/core.h>

#include <tinyxml2.h>

#include "timezoneinfo.hpp"

#if _WIN32
#    if defined( _DEBUG )
#        pragma comment( lib, "tinyxml2d.lib" )
#        pragma comment( lib, "fmtd.lib" )
#    else
#        pragma comment( lib, "tinyxml2.lib" )
#        pragma comment( lib, "fmt.lib" )
#    endif
#endif

namespace fs = std::filesystem;

char const * elementToCStr ( tinyxml2::XMLElement const * const element_, char const name_[] ) noexcept {
    char const * out;
    element_->QueryStringAttribute ( name_, &out );
    return out;
}

struct Info {
    std::string name, code;
};

using KeyValue = sax::pair<std::string, Info>;
using Map      = std::vector<KeyValue>;

[[nodiscard]] std::map<std::string, Info> buildIanaToWindowsZonesMap ( fs::path const & path_ ) {
    std::map<std::string, Info> map;
    tinyxml2::XMLDocument doc;
    doc.LoadFile ( path_.string ( ).c_str ( ) );
    tinyxml2::XMLElement const * element = doc.FirstChildElement ( "supplementalData" )
                                               ->FirstChildElement ( "windowsZones" )
                                               ->FirstChildElement ( "mapTimezones" )
                                               ->FirstChildElement ( "mapZone" );
    tinyxml2::XMLElement const * const last_element = element->Parent ( )->LastChildElement ( "mapZone" );
    while ( true ) {
        auto const other     = elementToCStr ( element, "other" );
        auto const territory = elementToCStr ( element, "territory" );
        if ( "001" != territory ) {
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

template<typename Stream>
[[maybe_unused]] Stream & operator<< ( Stream & os_, SYSTEMTIME const & st_ ) {
    os_ << fmt::format ( "{:04} {:02} {:1} {:02} {:02}:{:02}", st_.wYear, st_.wMonth, st_.wDayOfWeek, st_.wDay, st_.wHour,
                         st_.wMinute );
    return os_;
}

[[nodiscard]] TIME_ZONE_INFORMATION get_tzi ( std::string const & desc_ ) {
    // The registry entry for TZI.
    struct REG_TZI_FORMAT {
        LONG Bias;
        LONG StandardBias;
        LONG DaylightBias;
        SYSTEMTIME StandardDate;
        SYSTEMTIME DaylightDate;
    };
    // Variables.
    HKEY hKey = nullptr;
    DWORD dwDataLen;
    REG_TZI_FORMAT reg_tzi_format{};
    TIME_ZONE_INFORMATION tzi{};
    // Create URI.
    std::wstring const desc = sax::utf8_to_utf16 ( desc_ );
    std::wstring const uri  = std::wstring ( L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\" ) + desc;

    RegOpenKeyEx ( HKEY_LOCAL_MACHINE, uri.c_str ( ), 0, KEY_ALL_ACCESS, &hKey );

    dwDataLen = sizeof ( REG_TZI_FORMAT );
    RegQueryValueEx ( hKey, TEXT ( "TZI" ), NULL, NULL, ( LPBYTE ) &reg_tzi_format, &dwDataLen );

    tzi.Bias = reg_tzi_format.Bias; // UTC = local time + bias

    dwDataLen = 64;
    RegQueryValueEx ( hKey, TEXT ( "Std" ), NULL, NULL, ( LPBYTE ) &tzi.StandardName, &dwDataLen );

    tzi.StandardDate = reg_tzi_format.StandardDate;
    tzi.StandardBias = reg_tzi_format.StandardBias;

    dwDataLen = 64;
    RegQueryValueEx ( hKey, TEXT ( "Dlt" ), NULL, NULL, ( LPBYTE ) &tzi.DaylightName, &dwDataLen );

    tzi.DaylightDate = reg_tzi_format.DaylightDate;
    tzi.DaylightBias = reg_tzi_format.DaylightBias;

    RegCloseKey ( hKey );

    return tzi;
}

int main ( ) {
    /*
    std::map<std::string, Info> map{ buildIanaToWindowsZonesMap ( "Y:/REPOS/timezoneinfo/windowsZones.xml" ) };

    std::cout << map.size ( ) << nl;

    for ( auto const & e : map ) {
        std::cout << e.first << " - " << e.second.name << " - " << e.second.code << nl;
    }
    */
    TIME_ZONE_INFORMATION tzi = get_tzi ( "Central Brazilian Standard Time" );

    std::cout << tzi.Bias << nl;
    std::cout << tzi.DaylightBias << nl;
    std::cout << tzi.DaylightDate << nl;
    std::wcout << tzi.DaylightName << nl;
    std::cout << tzi.StandardBias << nl;
    std::cout << tzi.StandardDate << nl;
    std::wcout << tzi.StandardName << nl;

    return EXIT_SUCCESS;
}

constexpr char const * dow[ 7 ]             = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
constexpr char const * day_of_the_week[ 7 ] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saterday" };
constexpr char const * moy[ 12 ] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
constexpr char const * month_of_the_year[ 12 ] = { "January", "February", "March",     "April",   "May",      "June",
                                                   "July",    "August",   "September", "October", "November", "December" };
[[nodiscard]] int today_year ( ) noexcept {
    std::time_t now;
    std::time ( &now );
    std::tm * date = std::gmtime ( &now );
    return date->tm_year + 1'900;
}

[[nodiscard]] bool is_leap_year ( int const y ) noexcept { return ( ( y % 4 == 0 ) and ( y % 100 != 0 ) ) or ( y % 400 == 0 ); }

[[nodiscard]] int number_of_days_month ( int const y, int const m ) noexcept {
    //  This function returns the number of days for the given m (month) in the given y (year)
    return ( 30 + ( ( ( m & 9 ) == 8 ) or ( ( m & 9 ) == 1 ) ) - ( m == 2 ) -
             ( !( ( ( y % 4 ) == 0 ) and ( ( ( y % 100 ) != 0 ) or ( ( y % 400 ) == 0 ) ) ) and ( m == 2 ) ) );
}

[[nodiscard]] int number_of_days_ytd ( int const y, int const m, int const d ) noexcept { // normal counting
    if ( d > number_of_days_month ( y, m ) )
        return -1;
    constexpr int const cum_dim[ 12 ] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    return cum_dim[ m - 1 ] + d + ( ( m > 2L ) * ( ( ( y % 4 == 0 ) and ( y % 100 != 0 ) ) or ( y % 400 == 0 ) ) );
}

[[nodiscard]] int number_of_weeks_ytd ( int const y, int const m, int const d ) noexcept { // normal counting
    if ( d > number_of_days_month ( y, m ) )
        return -1;
    constexpr int const cum_dim[ 12 ] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    return ( ( ( cum_dim[ m - 1 ] + d + ( ( m > 2L ) * ( ( ( y % 4 == 0 ) and ( y % 100 != 0 ) ) or ( y % 400 == 0 ) ) ) ) -
               first_weekday_day ( y, 1, 0 ) ) /
             7 ) +
           1;
}

[[nodiscard]] int weekday ( int y, int m, int d ) noexcept {
    // calendar_system = 1 for Gregorian Calendar, 0 for Julian Calendar
    int const calendar_system = 1;
    if ( m < 3 ) {
        m += 12;
        y -= 1;
    }
    return ( d + ( m << 1 ) + ( 6 * ( m + 1 ) / 10 ) + y + ( y >> 2 ) - ( y / 100 ) + ( y / 400 ) + calendar_system ) % 7;
}

[[nodiscard]] int first_weekday_month ( int y, int m ) noexcept { return weekday ( y, m, 1 ); }

[[nodiscard]] int first_weekday_next_month ( int y, int m ) noexcept {
    return ( ( m != 12 ) ? weekday ( y, ++m, 1 ) : weekday ( ++y, 1, 1 ) );
}

[[nodiscard]] int last_weekday_month ( int y, int m ) noexcept {
    return ( ( m != 12 ) ? ( ( weekday ( y, ++m, 1 ) + 6 ) % 7 ) : ( ( weekday ( ++y, 1, 1 ) + 6 ) % 7 ) );
}

[[nodiscard]] std::time_t time_last_weekday_month ( int const y, int const m, int const w ) noexcept {
    std::tm date{};
    date.tm_year = y - 1'900; // two digit y!
    date.tm_mon  = m - 1;
    date.tm_mday = last_weekday_day ( y, m, w );
    return std::mktime ( &date );
}

void get_utc ( std::tm * ptm ) noexcept {
    std::time_t rawtime;
    std::time ( &rawtime );
    ptm = std::gmtime ( &rawtime );
}

[[nodiscard]] bool is_workweek ( int const y, int const m, int const d ) noexcept {
    int const dow = weekday ( y, m, d );
    return not( dow == 0 or dow == 6 );
}

[[nodiscard]] bool is_weekend ( int const y, int const m, int const d ) noexcept { return not is_workweek ( y, m, d ); }

[[nodiscard]] int today_weekday ( ) noexcept {
    std::time_t now;
    std::time ( &now );
    std::tm date;
    localtime_s ( &date, &now );
    return date.tm_wday;
}

[[nodiscard]] bool is_today_workweek ( ) noexcept {
    int const dow = today_weekday ( );
    return not( dow == 0 or dow == 6 );
}

void print_date_time_t ( std::time_t rawtime ) noexcept {
    std::tm * ptm = std::gmtime ( &rawtime );
    std::printf ( "%02i:%02i:%02i, %s %02i.%02i.%4i", ptm->tm_hour, ptm->tm_min, ptm->tm_sec, dow[ ptm->tm_wday ], ptm->tm_mday,
                  ptm->tm_mon + 1, ptm->tm_year + 1'900 );
}

[[nodiscard]] int first_weekday_day ( int const y, int const m, int const w ) noexcept {
    return 1 + ( ( 7 - first_weekday_month ( y, m ) + w ) % 7 );
}

[[nodiscard]] int second_weekday_day ( int const y, int const m, int const w ) noexcept {
    return first_weekday_day ( y, m, w ) + 7;
}

[[nodiscard]] int third_weekday_day ( int const y, int const m, int const w ) noexcept {
    return first_weekday_day ( y, m, w ) + 14;
}

[[nodiscard]] int fourth_weekday_day ( int const y, int const m, int const w ) noexcept {
    return first_weekday_day ( y, m, w ) + 21;
}

int fifth_weekday_day ( int const y, int const m, int const w ) noexcept {
    int r = first_weekday_day ( y, m, w ) + 28;
    if ( r <= number_of_days_month ( y, m ) )
        return r;
    return -1;
}

[[nodiscard]] int last_weekday_day ( int const y, int const m, int const w ) noexcept {
    int r = first_weekday_day ( y, m, w ) + 28;
    if ( r <= number_of_days_month ( y, m ) )
        return r;
    return r - 7;
}

[[nodiscard]] int number_of_days_since ( int const y, int const m, int const d ) noexcept {
    std::time_t now;
    if ( std::time ( &now ) != ( std::time_t ) -1 ) {
        std::tm date{};
        date.tm_year     = y - 1'900;
        date.tm_mon      = m - 1;
        date.tm_mday     = d;
        std::time_t then = std::mktime ( &date );
        if ( then != ( std::time_t ) ( -1 ) )
            return ( int ) ( std::difftime ( now, then ) / ( 24 * 60 * 60 ) );
    }
    return 0;
}

[[nodiscard]] std::tm convert_PSYSTEMTIME_tm ( SYSTEMTIME const * in ) noexcept {
    std::tm tmp{};
    tmp.tm_sec              = in->wSecond;
    tmp.tm_min              = in->wMinute;
    tmp.tm_hour             = in->wHour;
    tmp.tm_mday             = in->wDay;
    tmp.tm_mon              = in->wMonth - 1;
    int const y             = in->wYear;
    tmp.tm_year             = y - 1900;
    tmp.tm_wday             = in->wDayOfWeek;
    int const cum_dim[ 12 ] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
    tmp.tm_yday             = cum_dim[ tmp.tm_mon ] + tmp.tm_mday +
                  ( ( tmp.tm_mon > 1 ) * ( ( ( y % 4 == 0 ) and ( y % 100 != 0 ) ) or ( y % 400 == 0 ) ) );
    // tmp.tm_isdst = 0;
    return tmp;
}

[[nodiscard]] SYSTEMTIME convert_ptm_SYSTEMTIME ( const std::tm * in ) noexcept {
    SYSTEMTIME tmp{};
    tmp.wYear      = in->tm_year + 1'900;
    tmp.wMonth     = in->tm_mon + 1;
    tmp.wDayOfWeek = in->tm_wday;
    tmp.wDay       = in->tm_mday;
    tmp.wHour      = in->tm_hour;
    tmp.wMinute    = in->tm_min;
    tmp.wSecond    = in->tm_sec;
    // tmp.wMilliseconds = 0;
    return tmp;
}

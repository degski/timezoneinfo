
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
#        pragma comment( lib, "tinyxml2d.lib" )
#        pragma comment( lib, "fmtd.lib" )
#    else
#        pragma comment( lib, "tinyxml2.lib" )
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

#include <string>

using tzi_t = TIME_ZONE_INFORMATION;

[[nodiscard]] tzi_t get_tzi ( std::string const & desc_ ) noexcept;
[[nodiscard]] bool has_dst ( tzi_t const & tzi ) noexcept;

[[nodiscard]] systime_t get_systime_in_tz ( tzi_t const & tzi_ ) noexcept;
[[nodiscard]] wintime_t get_wintime_in_tz ( tzi_t const & tzi_ ) noexcept;
[[nodiscard]] nixtime_t get_nixtime_in_tz ( tzi_t const & tzi_ ) noexcept;

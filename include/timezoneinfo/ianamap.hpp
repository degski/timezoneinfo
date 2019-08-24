
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
#    pragma comment( lib, "ws2_32.lib" )
#    pragma comment( lib, "Crypt32.lib" )
#    pragma comment( lib, "winmm.lib" )
#    pragma comment( lib, "wldap32.lib" )
#    if defined( _DEBUG )
#        pragma comment( lib, "zlibd.lib" )
#        pragma comment( lib, "libcurl-d.lib" )
#        pragma comment( lib, "tinyxml2d.lib" )
#    else
#        pragma comment( lib, "zlib.lib" )
#        pragma comment( lib, "libcurl.lib" )
#        pragma comment( lib, "tinyxml2.lib" )
#    endif
#    pragma comment( lib, "libcurlpp.lib" )
#endif

#include <filesystem>
#include <map>
#include <sax/stl.hpp>
#include <string>

namespace fs = std::filesystem;

using IanaMapKey  = std::string;

struct IanaMapValue {
    std::string name, code;
};

using IanaMap = std::map<IanaMapKey, IanaMapValue>;

[[nodiscard]] IanaMap build_iana_to_windowszones_map ( );

void download_windowszones ( );

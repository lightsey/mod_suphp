/*
    suPHP - (c)2002-2005 Sebastian Marsching <sebastian@marsching.com>

    This file is part of suPHP.

    suPHP is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    suPHP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with suPHP; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <sstream>

#include "Util.hpp"

using namespace suPHP;

std::string suPHP::Util::intToStr(const int i) {
    std::ostringstream ostr;
    ostr << i;
    return ostr.str();
}

int suPHP::Util::strToInt(const std::string str) {
    int i = 0;
    std::istringstream istr;
    istr.str(str);
    istr >> i;
    return i;
}

int suPHP::Util::octalStrToInt(const std::string str) {
    int result = 0;
    for (int i=0; i<str.length(); i++) {
        int d;
        result *= 8;
        switch (str[i]) {
        case '0':
            d = 0;
            break;
        case '1':
            d = 1;
            break;
        case '2':
            d = 2;
            break;
        case '3':
            d = 3;
            break;
        case '4':
            d = 4;
            break;
        case '5':
            d = 5;
            break;
        case '6':
            d = 6;
            break;
        case '7':
            d = 7;
            break;
        default:
            // Should not happen
            continue;
        }
        result += d; 
    }
    return result;
}

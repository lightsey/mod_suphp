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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef SUPHP_UTIL_H

namespace suPHP {
    class Util;
};

#define SUPHP_UTIL_H

#include <string>

namespace suPHP {
    /**
     * Class containing useful utility functions
     */
    class Util {
	
    public:
	static std::string intToStr(const int i);
	static int strToInt(const std::string istr);
	static int octalStrToInt(const std::string istr);
    };
};

#endif // SUPHP_UTIL_H

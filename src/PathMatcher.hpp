/*
    suPHP - (c)2002-2013 Sebastian Marsching <sebastian@marsching.com>

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

#ifndef SUPHP_PATHMATCHER_H

namespace suPHP {
    class PathMatcher;
};

#define SUPHP_PATHMATCHER_H

#include <string>

#include "UserInfo.hpp"
#include "GroupInfo.hpp"
#include "KeyNotFoundException.hpp"
#include "ParsingException.hpp"

namespace suPHP {
    class PathMatcher {
    private:
        UserInfo user;
        GroupInfo group;
        std::string lookupVariable(std::string str)
            throw (KeyNotFoundException);
        
    public:
        /**
         * Contructor
         */
        PathMatcher(const UserInfo& user, const GroupInfo& group);
        
        /**
         * Checks wheter a path matches a pattern
         */
        bool matches(std::string pattern, std::string path) 
            throw (KeyNotFoundException, ParsingException);
        
        /**
         * Resolves variables in a string
         */
        std::string resolveVariables(std::string str)
            throw (KeyNotFoundException, ParsingException);
    };
};

#endif // SUPHP_PATHMATCHER_H

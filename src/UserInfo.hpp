/*
    suPHP - (c)2002-2008 Sebastian Marsching <sebastian@marsching.com>

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

#ifndef SUPHP_USERINFO_H

namespace suPHP {
    class UserInfo;
};

#define SUPHP_USERINFO_H

#include <string>
#include <vector>

#include "LookupException.hpp"
#include "GroupInfo.hpp"

namespace suPHP {
    /**
     * Class encapsulating user information.
     */
    class UserInfo {
    private:
        int uid;
    public:
        /**
         * Constructor without arguments.
         * Does not create a "valid" object, since it has no well defined UID
         */
        UserInfo();
        
        /**
         * Constructor (takes UID)
         */
        UserInfo(int uid);
        
        /**
         * Returns username
         */
        std::string getUsername() const throw (LookupException);
        
        /**
         * Returns UID
         */
        int getUid() const;

        /**
         * Returns primary group
         */
        GroupInfo getGroupInfo() const throw (LookupException);
        
        /**
         * Returns home directory
         */
        std::string getHomeDirectory() const throw (LookupException);
        
        /**
         * Checks wheter user is super-user
         */
        bool isSuperUser();
        
        /**
         * Compares to UserInfo objects for equality (same UID)
         */
        bool operator==(const UserInfo& uinfo) const;

        /**
         * Overloaded operator
         */
        bool operator!=(const UserInfo& uinfo) const;
        
    };
};

#endif // SUPHP_USERINFO_H

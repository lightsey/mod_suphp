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

#ifndef SUPHP_GROUPINFO_H

namespace suPHP {
    class GroupInfo;
};

#define SUPHP_GROUPINFO_H

#include <string>
#include <vector>

#include "LookupException.hpp"

namespace suPHP {
    /**
     * Class encapsulating group information.
     */
    class GroupInfo {
    private:
	int gid;

    public:
	/**
	 * Constructor without arguments.
	 * Does not create a "valid" object, since it has no well defined GID
	 */
	GroupInfo();

	/**
	 * Contructor (creates group object from GID)
	 */
	GroupInfo(int gid);

	/**
	 * Returns groupname
	 */
	std::string getGroupname() const throw (LookupException);
	
	/**
	 * Returns GID
	 */
	int getGid() const;

	/**
	 * Compares to GroupInfo objects for equality
	 */
	bool operator==(const GroupInfo& ginfo) const;
	
	/**
	 * Overloaded operator !=
	 */
	bool operator!=(const GroupInfo& ginfo) const; 
    };
};

#endif // SUPHP_GROUPINFO_H

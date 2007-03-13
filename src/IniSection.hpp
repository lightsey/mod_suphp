#/*
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

#ifndef SUPHP_INISECTION_H

namespace suPHP {
    class IniSection;
};

#define SUPHP_INISECTION_H

#include <string>
#include <vector>
#include <map>

#include "KeyNotFoundException.hpp"
#include "File.hpp"
#include "IniFile.hpp"

namespace suPHP {
    /**
     * Class providing access to configuration in a INI file.
     */
    class IniSection {
    private:
        std::multimap<std::string, std::string> entries;
        void putValue(std::string key, std::string value);

    public:
        /**
         * Returns values corresponding to key
         */
        std::vector<std::string> getValues(std::string key) 
            throw (KeyNotFoundException);

        /**
         * Returns first value corresponding to a key
         */
        std::string getValue(std::string key) throw (KeyNotFoundException);

        /**
         * Returns keys appearing in this section
         */
        std::vector<std::string> getKeys();
        /**
         * Overloaded index operator, calls getValues()
         */
        std::vector<std::string> operator[](std::string key)
            throw (KeyNotFoundException);

        friend class IniFile;
        
        /**
         * Check wheter key is existing within section
         */
        bool hasKey(std::string name);
    };
};

#endif // SUPHP_INISECTION_H

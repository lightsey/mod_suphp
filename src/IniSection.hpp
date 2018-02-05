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
        std::multimap<const std::string, const std::string> entries;
        void putValue(const std::string key, const std::string value);
        void removeValues(const std::string& key);

    public:
        /**
         * Returns values corresponding to key
         */
        const std::vector<std::string> getValues(const std::string& key) const
            throw (KeyNotFoundException);

        /**
         * Returns first value corresponding to a key
         */
        std::string getValue(const std::string& key) const throw (KeyNotFoundException);

        /**
         * Returns keys appearing in this section
         */
        const std::vector<std::string> getKeys() const;
        /**
         * Overloaded index operator, calls getValues()
         */
        const std::vector<std::string> operator[](const std::string& key) const
            throw (KeyNotFoundException);

        friend class IniFile;
        
        /**
         * Check wheter key is existing within section
         */
        bool hasKey(const std::string& name) const;
    };
};

#endif // SUPHP_INISECTION_H

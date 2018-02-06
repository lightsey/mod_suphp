/*
    suPHP - (c)2002-2013 Sebastian Marsching <sebastian@marsching.com>
            (c)2018 John Lightsey <john@nixnuts.net>

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

#ifndef SUPHP_COMMANDLINE_H

namespace suPHP {
    class CommandLine;
};

#define SUPHP_COMMANDLINE_H

#include <string>
#include <vector>

#include "OutOfRangeException.hpp"

namespace suPHP {
    /**
     * Class containing command-line arguments.
     */
    class CommandLine {
    private:
        std::vector<std::string> arguments;

    public:
        /**
         * Size typedef
         */
        typedef std::vector<std::string>::size_type size_type;

        /**
         * Constructer
         */
        CommandLine();

        /**
         * Return number of arguments
         */
        size_type count() const;

        /**
         * Returns (copy of) argument at at position
         */
        std::string getArgument(size_type pos) const throw (OutOfRangeException);

        /**
         * Set argument at position
         */
        void setArgument(size_type pos, std::string arg);

        /**
         * Add argument at the end of commandline
         */
        void putArgument(std::string arg);

        /**
         * Returns reference to string at index
         */
        std::string& operator[](size_type index) throw (OutOfRangeException);

        /**
         * Returns number of elements
         */
        size_type size() const;
    };
};

#endif // SUPHP_COMMANDLINE_H

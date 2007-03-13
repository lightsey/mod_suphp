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

#ifndef SUPHP_PARSINGEXCEPTION_H

namespace suPHP {
    class ParsingException;
};

#define SUPHP_PARSINGEXCEPTION_H

#include <string>
#include <iostream>

#include "Exception.hpp"

namespace suPHP {
    /**
     * Exception showing that an error ocurred while parsing some input
     */
    class ParsingException : public Exception {
    private:
        std::string getName() const;
    public:
        /**
         * Constructor without message.
         */
        ParsingException(std::string file, int line);

        /**
         * Constructor with message.
         */
        ParsingException(std::string message, std::string file, int line);
        
        /**
         * Constructor without message but with cause.
         */
        ParsingException(Exception& cause, std::string file, int line);
        
        /**
         * Constructor with message and cause.
         */
        ParsingException(std::string message, Exception& cause, std::string file, int line);
        
    };
};

#endif // SUPHP_PARSINGEXCEPTION_H

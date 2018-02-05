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

#include "ParsingException.hpp"

using namespace suPHP;

std::string suPHP::ParsingException::getName() const {
    return "ParsingException";
}

suPHP::ParsingException::ParsingException(std::string file, int line) : 
    Exception(file, line) {}

suPHP::ParsingException::ParsingException(std::string message, std::string file, int line) :
    Exception(message, file, line) {}

suPHP::ParsingException::ParsingException(Exception& cause, std::string file, int line) : 
    Exception(cause, file, line) {}

suPHP::ParsingException::ParsingException(std::string message, Exception& cause, std::string file, int line) : 
    Exception(message, cause, file, line) {}

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

#include <sstream>

#include "Exception.hpp"

using namespace suPHP;

std::string suPHP::Exception::getName() const {
    return "Exception";
}

suPHP::Exception::Exception(std::string file, int line) {
    this->file = file;
    this->line = line;
}

suPHP::Exception::Exception(std::string message, std::string file, int line) {
    this->message = message;
    this->file = file;
    this->line = line;
}

suPHP::Exception::Exception(Exception& cause, std::string file, int line) {
    this->backtrace = cause.toString();
    this->file = file;
    this->line = line;
}

suPHP::Exception::Exception(std::string message, Exception& cause, std::string file, int line) {
    this->message = message;
    this->backtrace = cause.toString();
    this->file = file;
    this->line = line;
}

std::string suPHP::Exception::getMessage() {
    return this->message;
}

std::string suPHP::Exception::toString() const {
    std::ostringstream ostr;
    ostr << std::string(this->getName()) << " in " << this->file 
	 << ":" << this->line << ": "
	 << this->message << "\n";
    if (this->backtrace.length() > 0) {
	ostr << "Caused by " << this->backtrace;
    }
    return ostr.str();
}

std::ostream& suPHP::operator<<(std::ostream& os, const Exception& e) {
    os << e.toString();
    return os;
}

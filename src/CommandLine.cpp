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

#include <stdexcept>
#include <string>

#include "OutOfRangeException.hpp"

#include "CommandLine.hpp"

using namespace suPHP;

suPHP::CommandLine::CommandLine() {
    /* do nothing */
}

int suPHP::CommandLine::count() const {
    return this->arguments.size();
}

std::string suPHP::CommandLine::getArgument(int pos) const
    throw (OutOfRangeException) {
    try {
	return this->arguments.at(pos);
    } catch (std::out_of_range& e) {
	throw OutOfRangeException("Index out of range", __FILE__, __LINE__);
    }
}

void suPHP::CommandLine::setArgument(int pos, std::string arg) {
    if (pos >= this->arguments.size()) {
	for (int i=0; i<(this->arguments.size() - pos); i++) {
	    this->arguments.push_back(std::string(""));
	}
    }
    this->arguments.at(pos) = arg;
}

void suPHP::CommandLine::putArgument(std::string arg) {
    this->arguments.push_back(arg);
}

std::string& suPHP::CommandLine::operator[](int index) 
    throw (OutOfRangeException) {
    try {
	return this->arguments.at(index);
    } catch (std::out_of_range& ex) {
	throw OutOfRangeException("Index out of range", __FILE__, __LINE__);
    }
}


int suPHP::CommandLine::size() const {
    return this->arguments.size();
}

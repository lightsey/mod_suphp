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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <stdexcept>
#include <string>

#include "KeyNotFoundException.hpp"

#include "Environment.hpp"

using namespace suPHP;

std::string suPHP::Environment::getVar(const std::string& name) const
    throw (KeyNotFoundException) {
    if (this->vars.find(name) != this->vars.end()) {
        return this->vars.find(name)->second;
    } else {
        throw KeyNotFoundException("Key " + name + " not found", 
                                   __FILE__, __LINE__);
    }
}

void suPHP::Environment::setVar(const std::string name, 
                                const std::string content) 
    throw (KeyNotFoundException) {
    if (this->vars.find(name) != this->vars.end()) {
        this->vars.find(name)->second = content;
    } else {
        throw KeyNotFoundException("Key " + name + " not found", 
                                   __FILE__, __LINE__);
    }
}

void suPHP::Environment::putVar(const std::string name, 
                                const std::string content) {
    if (this->vars.find(name) != this->vars.end()) {
        this->vars.find(name)->second = content;
    } else {
        std::pair<std::string, std::string> p;
        p.first = name;
        p.second = content;
        this->vars.insert(p);
    }
    
}

void suPHP::Environment::deleteVar(const std::string& name) 
    throw (KeyNotFoundException) {
    if (this->vars.find(name) != this->vars.end()) {
        this->vars.erase(name);
    } else {
        throw KeyNotFoundException("Key " + name + " not found",
                                   __FILE__, __LINE__);
    }
}
 
bool suPHP::Environment::hasVar(const std::string& name) const {
    if (this->vars.find(name) != this->vars.end()) {
        return true;
    } else {
        return false;
    }
    
}

std::string& suPHP::Environment::operator[](const std::string& name) 
    throw (KeyNotFoundException) {
    if (this->vars.find(name) != this->vars.end()) {
        return this->vars.find(name)->second;
    } else {
        throw KeyNotFoundException("Key " + name + " not found", 
                                   __FILE__, __LINE__);
    }
}


const std::map<std::string, std::string>& suPHP::Environment::getBackendMap() 
    const {
    return this->vars;
}

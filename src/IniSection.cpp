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

#include <string>

#include "KeyNotFoundException.hpp"

#include "IniSection.hpp"

using namespace suPHP;

void suPHP::IniSection::putValue(std::string key, std::string value) {
    std::pair<std::string, std::string> p;
    p.first = key;
    p.second = value;
    this->entries.insert(p);
}

std::vector<std::string> suPHP::IniSection::getValues(std::string key)
    throw (KeyNotFoundException) {
    std::vector<std::string> values;
    for (std::multimap<std::string, std::string>::iterator pos = 
             this->entries.find(key); 
         pos != this->entries.end(); pos++) {
        values.push_back(pos->second);
    }
    if (values.size() == 0) {
        throw KeyNotFoundException("No value for key " + key + " found", 
                                   __FILE__, __LINE__);
    }
    return values;
}

std::string suPHP::IniSection::getValue(std::string key)
    throw (KeyNotFoundException) {
    std::vector<std::string> values;
    if (this->entries.find(key) != this->entries.end()) {
        return this->entries.find(key)->second;
    } else {
        throw KeyNotFoundException("No value for key " + key + " found", 
                                   __FILE__, __LINE__);
    }
    
}

std::vector<std::string> suPHP::IniSection::getKeys() {
    std::vector<std::string> keys;
    for (std::multimap<std::string, std::string>::iterator pos =
             this->entries.begin();
         pos != this->entries.end(); pos++) {
        keys.push_back(pos->first);
    }
    return keys;
}

bool suPHP::IniSection::hasKey(std::string key) {
    if (this->entries.find(key) != this->entries.end()) {
        return true;
    } else {
        return false;
    }
    
}

std::vector<std::string> suPHP::IniSection::operator[](std::string key) 
    throw (KeyNotFoundException) {
    return this->getValues(key);
}

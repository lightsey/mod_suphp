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
  along with suPHP; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <string>

#include "KeyNotFoundException.hpp"

#include "IniSection.hpp"

using namespace suPHP;

void suPHP::IniSection::putValue(const std::string key,
                                 const std::string value) {
  std::pair<std::string, std::string> p;
  p.first = key;
  p.second = value;
  this->entries.insert(p);
}

const std::vector<std::string> suPHP::IniSection::getValues(
    const std::string& key) const {
  std::vector<std::string> values;
  std::pair<std::multimap<const std::string, const std::string>::const_iterator,
            std::multimap<const std::string, const std::string>::const_iterator>
      range = this->entries.equal_range(key);
  for (std::multimap<const std::string, const std::string>::const_iterator pos =
           range.first;
       pos != range.second; pos++) {
    values.push_back(pos->second);
  }
  if (values.size() == 0) {
    throw KeyNotFoundException("No value for key " + key + " found", __FILE__,
                               __LINE__);
  }
  return values;
}

std::string suPHP::IniSection::getValue(const std::string& key) const {
  std::vector<std::string> values;
  if (this->entries.find(key) != this->entries.end()) {
    return this->entries.find(key)->second;
  } else {
    throw KeyNotFoundException("No value for key " + key + " found", __FILE__,
                               __LINE__);
  }
}

const std::vector<std::string> suPHP::IniSection::getKeys() const {
  std::vector<std::string> keys;
  for (std::multimap<const std::string, const std::string>::const_iterator pos =
           this->entries.begin();
       pos != this->entries.end(); pos++) {
    keys.push_back(pos->first);
  }
  return keys;
}

bool suPHP::IniSection::hasKey(const std::string& key) const {
  if (this->entries.find(key) != this->entries.end()) {
    return true;
  } else {
    return false;
  }
}

const std::vector<std::string> suPHP::IniSection::operator[](
    const std::string& key) const {
  return this->getValues(key);
}

void suPHP::IniSection::removeValues(const std::string& key) {
  this->entries.erase(key);
}

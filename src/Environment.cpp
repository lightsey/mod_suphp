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

#include <stdexcept>
#include <string>

#include "KeyNotFoundException.hpp"

#include "Environment.hpp"

using namespace suPHP;

std::string suPHP::Environment::getVar(const std::string& name) const
    throw(KeyNotFoundException) {
  auto found = this->vars.find(name);
  if (found != this->vars.end()) {
    return found->second;
  } else {
    throw KeyNotFoundException("Key " + name + " not found", __FILE__,
                               __LINE__);
  }
}

void suPHP::Environment::setVar(
    const std::string name,
    const std::string content) throw(KeyNotFoundException) {
  auto found = this->vars.find(name);
  if (found != this->vars.end()) {
    found->second = content;
  } else {
    throw KeyNotFoundException("Key " + name + " not found", __FILE__,
                               __LINE__);
  }
}

void suPHP::Environment::putVar(const std::string name,
                                const std::string content) {
  auto inserted = this->vars.insert(
      std::map<const std::string, std::string>::value_type(name, content));
  if (!inserted.second) {
    inserted.first->second = content;
  }
}

void suPHP::Environment::deleteVar(const std::string& name) {
  this->vars.erase(name);
}

bool suPHP::Environment::hasVar(const std::string& name) const {
  if (this->vars.find(name) != this->vars.end()) {
    return true;
  } else {
    return false;
  }
}

std::string& suPHP::Environment::operator[](const std::string& name) throw(
    KeyNotFoundException) {
  auto found = this->vars.find(name);
  if (found != this->vars.end()) {
    return found->second;
  } else {
    throw KeyNotFoundException("Key " + name + " not found", __FILE__,
                               __LINE__);
  }
}

const std::map<std::string, std::string>& suPHP::Environment::getBackendMap()
    const {
  return this->vars;
}

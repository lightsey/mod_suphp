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

#include "API.hpp"
#include "API_Helper.hpp"

#include "GroupInfo.hpp"

using namespace suPHP;

suPHP::GroupInfo::GroupInfo() { this->gid = -1; }

suPHP::GroupInfo::GroupInfo(int gid) { this->gid = gid; }

std::string suPHP::GroupInfo::getGroupname() const {
  API& api = API_Helper::getSystemAPI();
  return api.GroupInfo_getGroupname(*this);
}

int suPHP::GroupInfo::getGid() const { return this->gid; }

bool suPHP::GroupInfo::operator==(const GroupInfo& ginfo) const {
  if (this->getGid() == ginfo.getGid())
    return true;
  else
    return false;
}

bool suPHP::GroupInfo::operator!=(const GroupInfo& ginfo) const {
  return !this->operator==(ginfo);
}

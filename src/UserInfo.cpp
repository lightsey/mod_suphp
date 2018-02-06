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

#include <string>

#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

#include "API.hpp"
#include "API_Helper.hpp"

#include "UserInfo.hpp"

using namespace suPHP;

suPHP::UserInfo::UserInfo() { this->uid = -1; }

suPHP::UserInfo::UserInfo(int uid) { this->uid = uid; }

std::string suPHP::UserInfo::getUsername() const throw(LookupException) {
  API& api = API_Helper::getSystemAPI();
  return api.UserInfo_getUsername(*this);
}

int suPHP::UserInfo::getUid() const { return this->uid; }

GroupInfo suPHP::UserInfo::getGroupInfo() const throw(LookupException) {
  API& api = API_Helper::getSystemAPI();
  return api.UserInfo_getGroupInfo(*this);
}

std::string suPHP::UserInfo::getHomeDirectory() const throw(LookupException) {
  API& api = API_Helper::getSystemAPI();
  return api.UserInfo_getHomeDirectory(*this);
}

bool suPHP::UserInfo::isSuperUser() {
  return API_Helper::getSystemAPI().UserInfo_isSuperUser(*this);
}

bool suPHP::UserInfo::operator==(const UserInfo& uinfo) const {
  if (this->getUid() == uinfo.getUid())
    return true;
  else
    return false;
}

bool suPHP::UserInfo::operator!=(const UserInfo& uinfo) const {
  return !this->operator==(uinfo);
}

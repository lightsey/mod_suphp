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

#include <fnmatch.h>
#include "PathMatcher.hpp"
#include "Util.hpp"

namespace suPHP {

template <class TUserInfo, class TGroupInfo>
bool PathMatcher<TUserInfo, TGroupInfo>::matches(
    std::string pattern, std::string path) {
  std::string interpolatedPattern = resolveVariables(pattern, false);
  if (interpolatedPattern.length() == 0) {
    return true;
  }
  if (interpolatedPattern.at(interpolatedPattern.length() - 1) == '/') {
    interpolatedPattern.append("*");
  }
  if (fnmatch(interpolatedPattern.c_str(), path.c_str(),
              FNM_FILE_NAME | FNM_LEADING_DIR) == 0) {
    return true;
  }
  return false;
}

template <class TUserInfo, class TGroupInfo>
std::string PathMatcher<TUserInfo, TGroupInfo>::lookupVariable(
    std::string str) {
  std::string rv;
  if (str == "USERNAME") {
    rv = user.getUsername();
  } else if (str == "UID") {
    rv = Util::intToStr(user.getUid());
  } else if (str == "HOME") {
    rv = user.getHomeDirectory();
  } else if (str == "GROUPNAME") {
    rv = group.getGroupname();
  } else if (str == "GID") {
    rv = Util::intToStr(group.getGid());
  } else {
    throw KeyNotFoundException(
        "Key \"" + str + "\" does not represent a valid variable name",
        __FILE__, __LINE__);
  }
  return rv;
}

template <class TUserInfo, class TGroupInfo>
std::string PathMatcher<TUserInfo, TGroupInfo>::resolveVariables(
    std::string str, bool unescape) {
  std::string out;
  bool escapeNext = false;
  for (std::string::size_type i = 0; i < str.length(); i++) {
    char c = str.at(i);
    if (escapeNext) {
      escapeNext = false;
      if (unescape && (c == '\\' || c == '$')) {
        // Backslash was used as an escape character
        out += c;
      } else {
        out += '\\';
        out += c;
      }
    } else {
      if (c == '\\') {
        escapeNext = true;
      } else if (c == '$') {
        if (str.length() < i + 3) {
          throw ParsingException(
              "Incorrect use of $ in string \"" + str + "\".", __FILE__,
              __LINE__);
        }
        if (str.at(i + 1) != '{') {
          throw ParsingException(
              "Incorrect use of $ in string \"" + str + "\".", __FILE__,
              __LINE__);
        }
        std::string::size_type closingBrace = str.find('}', i);
        if (closingBrace == std::string::npos) {
          throw ParsingException(
              "Incorrect use of $ in string \"" + str + "\".", __FILE__,
              __LINE__);
        }
        std::string varName = str.substr(i + 2, closingBrace - i - 2);
        out += lookupVariable(varName);
        i = closingBrace + 1;
      } else {
        out += c;
      }
    }
  }
  return out;
}

template class PathMatcher<UserInfo, GroupInfo>;
}

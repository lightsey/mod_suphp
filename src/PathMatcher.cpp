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

#include "PathMatcher.hpp"
#include "Util.hpp"

namespace suPHP {

template <class TUserInfo, class TGroupInfo>
bool PathMatcher<TUserInfo, TGroupInfo>::matches(
    std::string pattern, std::string path) throw(KeyNotFoundException,
                                                 ParsingException) {
  std::string remainingPath = path;
  std::string remainingPattern = pattern;

  while (remainingPath.length() > 0 && remainingPattern.length() > 0) {
    bool escapeNext = false;
    for (std::string::size_type i = 0; i < remainingPattern.length(); i++) {
      char c = remainingPattern.at(i);
      if (escapeNext) {
        if (c == '\\' || c == '*' || c == '$') {
          // Backslash was used as an escape character
          if (remainingPath.at(i - 1) == c) {
            remainingPattern = remainingPattern.substr(i + 1);
            remainingPath = remainingPath.substr(i);
            break;
          } else {
            return false;
          }
        } else {
          if (remainingPath.at(i - 1) == '\\') {
            remainingPattern = remainingPattern.substr(i);
            remainingPath = remainingPath.substr(i);
            break;
          } else {
            return false;
          }
        }
      } else {
        if (c == '\\') {
          escapeNext = true;
        } else if (c == '*') {
          remainingPattern = remainingPattern.substr(i + 1);
          remainingPath = remainingPath.substr(i);
          if (matches(remainingPattern, remainingPath)) {
            return true;
          }
          std::string testPrefix;
          for (std::string::size_type j = 0; j < remainingPath.length(); j++) {
            char c2 = remainingPath.at(j);
            if (c2 == '/') {
              return false;
            }
            if (c2 == '\\' || c2 == '*' || c2 == '$') {
              testPrefix += "\\";
            }
            testPrefix += c2;
            if (matches(testPrefix + remainingPattern, remainingPath)) {
              return true;
            }
          }
        } else if (c == '$') {
          if (remainingPattern.length() < i + 3) {
            throw ParsingException(
                "Incorrect use of $ in pattern \"" + pattern + "\".", __FILE__,
                __LINE__);
          }
          if (remainingPattern.at(i + 1) != '{') {
            throw ParsingException(
                "Incorrect use of $ in pattern \"" + pattern + "\".", __FILE__,
                __LINE__);
          }
          std::string::size_type closingBrace = remainingPattern.find('}', i);
          if (closingBrace == std::string::npos) {
            throw ParsingException(
                "Incorrect use of $ in pattern \"" + pattern + "\".", __FILE__,
                __LINE__);
          }
          std::string varName =
              remainingPattern.substr(i + 2, closingBrace - i - 2);
          remainingPattern = lookupVariable(varName) +
                             remainingPattern.substr(closingBrace + 1);
          break;
        } else {
          if (i >= remainingPath.length() || c != remainingPath.at(i)) {
            return false;
          }
          if (i == remainingPattern.length() - 1) {
            if (c == '/' || (i + 1 < remainingPath.length() &&
                             remainingPath.at(i + 1) == '/')) {
              // Path represents file in subdirectory
              return true;
            } else if (remainingPath.length() == remainingPattern.length()) {
              // Exact match
              return true;
            } else {
              return false;
            }
          }
        }
      }
    }
  }
  return false;
}

template <class TUserInfo, class TGroupInfo>
std::string PathMatcher<TUserInfo, TGroupInfo>::lookupVariable(
    std::string str) throw(KeyNotFoundException) {
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
    std::string str) throw(KeyNotFoundException, ParsingException) {
  std::string out;
  bool escapeNext = false;
  for (std::string::size_type i = 0; i < str.length(); i++) {
    char c = str.at(i);
    if (escapeNext) {
      escapeNext = false;
      if (c == '\\' || c == '$') {
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

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

#include "Logger.hpp"

using namespace suPHP;

LogLevel suPHP::Logger::getLogLevel() { return this->logLevel; }

void suPHP::Logger::setLogLevel(LogLevel level) { this->logLevel = level; }

void suPHP::Logger::logInfo(const std::string& message) {
  if (this->getLogLevel() == LOGLEVEL_INFO) this->log("info", message);
}

void suPHP::Logger::logWarning(const std::string& message) {
  if (this->getLogLevel() == LOGLEVEL_WARN ||
      this->getLogLevel() == LOGLEVEL_INFO)
    this->log("warn", message);
}

void suPHP::Logger::logError(const std::string& message) {
  if (this->getLogLevel() == LOGLEVEL_ERROR ||
      this->getLogLevel() == LOGLEVEL_WARN ||
      this->getLogLevel() == LOGLEVEL_INFO)
    this->log("error", message);
}

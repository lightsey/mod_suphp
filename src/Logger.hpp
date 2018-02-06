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
    along with suPHP; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef SUPHP_LOGGER_H

namespace suPHP {
class Logger;

enum LogLevel { LOGLEVEL_NONE, LOGLEVEL_ERROR, LOGLEVEL_WARN, LOGLEVEL_INFO };
};

#define SUPHP_LOGGER_H

#include "Configuration.hpp"
//#include "API_Helper.hpp"

namespace suPHP {
/**
 * Class containing logging facility.
 * This is only an interface class.
 * It contains the code doing the formatting
 * but the API implementation has to provide
 * the file access code.
 */
class Logger {
 private:
  LogLevel logLevel;

  /**
   * Internal log function
   */
  virtual void log(const std::string& classification,
                   const std::string& message) = 0;

 protected:
  /**
   * Set log level
   */
  virtual void setLogLevel(LogLevel level);

 public:
  /**
   * Virtual destructor so that derived class destructors are invoked
   */
  virtual ~Logger() = default;

  /***
   * Get log level
   */
  virtual LogLevel getLogLevel();

  /**
   * Initialize (open logfile)
   */
  virtual void init(const Configuration& config) throw(IOException) = 0;

  /**
   * Check wheter Logger has been initialized
   */
  virtual bool isInitialized() = 0;

  /**
   * Logs info message
   */
  virtual void logInfo(const std::string& message);

  /**
   * Logs warning
   */
  virtual void logWarning(const std::string& message);

  /**
   * Logs error
   */
  virtual void logError(const std::string& message);
};
};

#endif  // SUPHP_LOGGER_H

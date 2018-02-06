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

#ifndef SUPHP_EXCEPTION_H

namespace suPHP {
class Exception;
};

#define SUPHP_EXCEPTION_H

#include <iostream>
#include <string>

namespace suPHP {
/**
 * Parent class for exceptions.
 * All exceptions are derived from this class.
 */
class Exception {
 private:
  std::string message;
  std::string backtrace;
  int line;
  std::string file;
  virtual std::string getName() const = 0;

 public:
  /**
   * Constructor without message.
   */
  Exception(std::string file, int line);

  /**
   * Constructor with message.
   */
  Exception(std::string message, std::string file, int line);

  /**
   * Constructor without message but with cause.
   */
  Exception(Exception& cause, std::string file, int line);

  /**
   * Constructor with message and cause.
   */
  Exception(std::string message, Exception& cause, std::string file, int line);

  /**
   * Get the message
   */
  std::string getMessage();

  /**
   * Get string representing the exception
   */
  std::string toString() const;
};

std::ostream& operator<<(std::ostream& os, const Exception& e);
};

#endif  // SUPHP_EXCEPTION_H

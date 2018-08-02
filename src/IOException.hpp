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

#ifndef SUPHP_IOEXCEPTION_H
#define SUPHP_IOEXCEPTION_H

#include <iostream>
#include <string>

#include "Exception.hpp"

namespace suPHP {
/**
 * Exception showing problems during I/O operation
 */
class IOException : public Exception {
 private:
  std::string getName() const;

 public:
  /**
   * Constructor without message.
   */
  IOException(std::string file, int line);

  /**
   * Constructor with message.
   */
  IOException(std::string message, std::string file, int line);

  /**
   * Constructor without message but with cause.
   */
  IOException(Exception& cause, std::string file, int line);

  /**
   * Constructor with message and cause.
   */
  IOException(std::string message, Exception& cause, std::string file,
              int line);
};
}

#endif  // SUPHP_IOEXCEPTION_H

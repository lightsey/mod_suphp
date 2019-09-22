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

#ifndef SUPHP_FILE_H

namespace suPHP {
class File;

enum FileMode {
  FILEMODE_USER_READ,
  FILEMODE_USER_WRITE,
  FILEMODE_USER_EXEC,
  FILEMODE_GROUP_READ,
  FILEMODE_GROUP_WRITE,
  FILEMODE_GROUP_EXEC,
  FILEMODE_OTHERS_READ,
  FILEMODE_OTHERS_WRITE,
  FILEMODE_OTHERS_EXEC
};
}  // namespace suPHP

#define SUPHP_FILE_H

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "GroupInfo.hpp"
#include "IOException.hpp"
#include "SystemException.hpp"
#include "UserInfo.hpp"

namespace suPHP {
/**
 * Class encapsulating file information and access.
 */
class File {
 private:
  std::string path;
  bool hasPermissionBit(FileMode perm) const;

 public:
  /**
   * Constructor
   */
  File(std::string path);

  /**
   * Returns path to file
   */
  std::string getPath() const;

  /**
   * Returns input stream to read from file
   */
  std::unique_ptr<std::ifstream> getInputStream() const;

  /**
   * Does file exists?
   */
  bool exists() const;

  /**
   * Returns real path to file (without symlinks in path)
   */
  std::string getRealPath() const;

  /**
   * Returns File object representing parent directory
   */
  File getParentDirectory() const;

  /**
   * Returns permission bit
   */
  bool hasUserReadBit() const;

  /**
   * Returns permission bit
   */
  bool hasUserWriteBit() const;

  /**
   * Returns permission bit
   */
  bool hasUserExecuteBit() const;

  /**
   * Returns permission bit
   */
  bool hasGroupReadBit() const;

  /**
   * Returns permission bit
   */
  bool hasGroupWriteBit() const;

  /**
   * Returns permission bit
   */
  bool hasGroupExecuteBit() const;

  /**
   * Returns permission bit
   */
  bool hasOthersReadBit() const;

  /**
   * Returns permission bit
   */
  bool hasOthersWriteBit() const;

  /**
   * Returns permission bit
   */
  bool hasOthersExecuteBit() const;

  /**
   * Returns owner (user) of file
   */
  UserInfo getUser() const;

  /**
   * Returns owning group of file
   */
  GroupInfo getGroup() const;

  /**
   * Checks whether this file is a symlink
   */
  bool isSymlink() const;
};
}  // namespace suPHP

#endif  // SUPHP_FILE_H

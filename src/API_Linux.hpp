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

#ifndef SUPHP_API_LINUX_H

namespace suPHP {
class API_Linux;
};

#define SUPHP_API_LINUX_H

#include <string>

#include "API.hpp"
#include "API_Linux_Logger.hpp"
#include "Environment.hpp"
#include "File.hpp"
#include "SmartPtr.hpp"

namespace suPHP {
/**
 * Class encapsulating system-specific API for Linux.
 */
class API_Linux : public API {
 private:
  static SmartPtr<API_Linux_Logger> logger;
  /**
   * Internal function for checking wheter path
   * points to a symlink
   */
  bool isSymlink(const std::string path) const throw(SystemException);

  /**
   * Internal function to read the target of a symlink
   */
  std::string readSymlink(const std::string path) const throw(SystemException);

 public:
  /**
   * Get environment variable
   */
  virtual Environment getProcessEnvironment();

  /**
   * Get UserInfo from username
   */
  virtual UserInfo getUserInfo(const std::string username) throw(
      LookupException);

  /**
   * Get UserInfo from UID
   */
  virtual UserInfo getUserInfo(const int uid);

  /**
   * Get GroupInfo from groupname
   */
  virtual GroupInfo getGroupInfo(const std::string groupname) throw(
      LookupException);

  /**
   * Get GroupInfo from GID
   */
  virtual GroupInfo getGroupInfo(const int gid);

  /**
   * Get UserInfo for effective UID
   */
  virtual UserInfo getEffectiveProcessUser();

  /**
   * Get UserInfo for real UID
   */
  virtual UserInfo getRealProcessUser();

  /**
   * Get GroupInfo for effective GID
   */
  virtual GroupInfo getEffectiveProcessGroup();

  /**
   * Get GroupInfo for real GID
   */
  virtual GroupInfo getRealProcessGroup();

  virtual Logger& getSystemLogger();

  /**
   * Set UID of current process
   */
  virtual void setProcessUser(const UserInfo& user) const
      throw(SystemException);

  /**
   * Set GID of current process
   */
  virtual void setProcessGroup(const GroupInfo& group) const
      throw(SystemException);

  /**
   * Returns username from UserInfo
   */
  virtual std::string UserInfo_getUsername(const UserInfo& uinfo) const
      throw(LookupException);

  /**
   * Returns group from UserInfo
   */
  virtual GroupInfo UserInfo_getGroupInfo(const UserInfo& uinfo) const
      throw(LookupException);

  /**
   * Returns home directory from UserInfo
   */
  virtual std::string UserInfo_getHomeDirectory(const UserInfo& uinfo) const
      throw(LookupException);

  /**
   * Checks whether UserInfo objects represents the super-user
   */
  virtual bool UserInfo_isSuperUser(const UserInfo& uinfo) const;

  /**
   * Returns groupname from GroupInfo
   */
  std::string GroupInfo_getGroupname(const GroupInfo& ginfo) const
      throw(LookupException);

  /**
   * Checks whether file exists
   */
  virtual bool File_exists(const File& file) const;

  /**
   * Returns real path to file
   */
  virtual std::string File_getRealPath(const File& file) const
      throw(SystemException);

  /**
   * Checks for a permission bit
   */
  virtual bool File_hasPermissionBit(const File& file, FileMode perm) const
      throw(SystemException);

  /**
   * Returns UID of file
   */
  virtual UserInfo File_getUser(const File& file) const throw(SystemException);

  /**
   * Returns GID of file
   */
  virtual GroupInfo File_getGroup(const File& file) const
      throw(SystemException);

  /**
   * Checks whether a file is a symlink
   */
  virtual bool File_isSymlink(const File& file) const throw(SystemException);

  /**
   * Runs another program (replaces current process)
   */
  virtual void execute(std::string program, const CommandLine& cline,
                       const Environment& env) const throw(SystemException);

  /**
   * Returns current working directory
   */
  virtual std::string getCwd() const throw(SystemException);

  /**
   * Sets current working directory
   */
  virtual void setCwd(const std::string& dir) const throw(SystemException);

  /**
   * Sets umask
   */
  virtual void setUmask(int umask) const throw(SystemException);

  /**
   * Sets new root directory for current process
   */
  virtual void chroot(const std::string& dir) const throw(SystemException);
};
};

#endif  // SUPHP_API_LINUX_H

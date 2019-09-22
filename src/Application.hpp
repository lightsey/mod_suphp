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

#ifndef SUPHP_APPLICATION_H
#define SUPHP_APPLICATION_H

#include <iostream>
#include <string>

#include "CommandLine.hpp"
#include "Environment.hpp"
#include "GroupInfo.hpp"
#include "SecurityException.hpp"
#include "SoftException.hpp"
#include "SystemException.hpp"
#include "UserInfo.hpp"

namespace suPHP {

enum TargetMode { TARGETMODE_PHP, TARGETMODE_SELFEXECUTE };

/**
 * Main application class.
 * Contains the main() function.
 */
class Application {
 private:
  /**
   * Print message containing version information
   */
  void printAboutMessage();

  /**
   * Checks wheter process has root privileges
   * and calling user is webserver user
   */
  void checkProcessPermissions(Configuration& config);

  /**
   * Checks scriptfile (first stage).
   * Includes check for VHost docroot, symbollink and permissions.
   */
  void checkScriptFileStage1(const File& scriptFile, const File& realScriptFile,
                             const Configuration& config,
                             const Environment& environment) const;

  /**
   * Checks scriptfile.
   * Includes check for paths which might be user specific
   */
  void checkScriptFileStage2(const File& scriptFile, const File& realScriptFile,
                             const Configuration& config,
                             const Environment& environment,
                             const UserInfo& targetUser,
                             const GroupInfo& targetGroup) const;

  /**
   * Determines target user and group that is to be used for script execution.
   * Uses preprocessor macros to distinguish between modes
   */
  void checkProcessPermissions(const File& scriptFile,
                               const File& realScriptFile,
                               const Configuration& config,
                               const Environment& environment,
                               UserInfo& targetUser,
                               GroupInfo& targetGroup) const;

  /**
   * Changes process permission (user and group).
   * Uses preprocessor macros to distinguish between modes
   */
  void changeProcessPermissions(const Configuration& config,
                                const UserInfo& targetUser,
                                const GroupInfo& targetGroup) const;

  /**
   * Prepares the environment before invoking the script
   */
  Environment prepareEnvironment(const Environment& sourceEnv,
                                 const Configuration& config, TargetMode mode);

  /**
   * Returns the php.ini path defined in the config for script being executed
   */
  std::string getPHPRCPath(const Environment& env, const Configuration& config);

  /**
   * Returns interpreter for script being executed
   */
  std::string getInterpreter(const Environment& env,
                             const Configuration& config);

  /**
   * Returns mode interpreter is using
   */
  TargetMode getTargetMode(const std::string& interpreter);

  /**
   * Runs script
   */
  void executeScript(const std::string& scriptFilename,
                     const std::string& interpreter, TargetMode mode,
                     const Environment& env, const Configuration& config) const;

  /**
   * Checks ownership and permissions for parent directories
   */
  void checkParentDirectories(const File& file, const UserInfo& owner,
                              const Configuration& config) const;

 public:
  /**
   * Constructer
   */
  Application();

  /**
   * Function called by the main() function
   */
  int run(CommandLine& cmdline, const Environment& env);
};
}  // namespace suPHP

int main(int argc, char** argv);

#endif  // SUPHP_APPLICATION_H

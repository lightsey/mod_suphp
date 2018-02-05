/*
    suPHP - (c)2002-2008 Sebastian Marsching <sebastian@marsching.com>

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

#ifndef SUPHP_APPLICATION_H

namespace suPHP {
    class Application;
};

enum TargetMode {
    TARGETMODE_PHP,
    TARGETMODE_SELFEXECUTE
};

#define SUPHP_APPLICATION_H

#include <string>
#include <iostream>

#include "Environment.hpp"
#include "CommandLine.hpp"
#include "SystemException.hpp"
#include "SoftException.hpp"
#include "SecurityException.hpp"
#include "UserInfo.hpp"
#include "GroupInfo.hpp"

namespace suPHP {
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
        void checkProcessPermissions(Configuration& config) 
            throw (SecurityException, LookupException);
        
        /**
         * Checks scriptfile (first stage).
         * Includes check for VHost docroot, symbollink and permissions.
         */
        void checkScriptFileStage1(const std::string& scriptFilename, 
                             const Configuration& config, 
                             const Environment& environment) const
            throw (SystemException, SoftException);
        
        /**
         * Checks scriptfile.
         * Includes check for paths which might be user specific
         */
        void checkScriptFileStage2(const std::string& scriptFilename, 
                             const Configuration& config, 
                             const Environment& environment,
                             const UserInfo& targetUser,
                             const GroupInfo& targetGroup) const
            throw (SystemException, SoftException);
        
        /**
         * Determines target user and group that is to be used for script execution.
         * Uses preprocessor macros to distinguish between modes
         */
        void checkProcessPermissions(const std::string& scriptFilename,
                                      const Configuration& config,
                                      const Environment& environment,
                                      UserInfo& targetUser,
                                      GroupInfo& targetGroup) const
            throw (SystemException, SoftException, SecurityException);
        
        /**
         * Changes process permission (user and group).
         * Uses preprocessor macros to distinguish between modes
         */
        void changeProcessPermissions(const Configuration& config,
                                      const UserInfo& targetUser,
                                      const GroupInfo& targetGroup) const
            throw (SystemException, SoftException, SecurityException);

        /**
         * Prepares the environment before invoking the script
         */
        Environment prepareEnvironment(const Environment& sourceEnv,
                                       const Configuration& config,
                                       TargetMode mode)
            throw (KeyNotFoundException);

        /**
         * Returns interpreter for script being executed
         */
        std::string getInterpreter(const Environment& env,
                                   const Configuration& config)
            throw (SecurityException);
        
        /**
         * Returns mode interpreter is using
         */
        TargetMode getTargetMode(const std::string& interpreter)
            throw (SecurityException);

        /**
         * Runs script
         */
        void executeScript(const std::string& scriptFilename,
                           const std::string& interpreter,
                           TargetMode mode,
                           const Environment& env,
                           const Configuration& config) const
            throw (SoftException);
        
        /**
         * Checks ownership and permissions for parent directories
         */
        void checkParentDirectories(const File& file,
                                    const UserInfo& owner,
                                    const Configuration& config) const
            throw (SoftException);


    public:
        /**
         * Constructer
         */
        Application();
        
        /**
         * Function called by the main() function
         */
        int run(CommandLine& cmdline, Environment& env);
    };
};

int main(int argc, char **argv);

#endif // SUPHP_APPLICATION_H

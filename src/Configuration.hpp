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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef SUPHP_CONFIGURATION_H

namespace suPHP {
    class Configuration;
};

#define SUPHP_CONFIGURATION_H

#include <string>
#include <map>
#include <vector>

#include "ParsingException.hpp"
#include "IOException.hpp"
#include "File.hpp"
#include "KeyNotFoundException.hpp"
#include "Logger.hpp"

namespace suPHP {
    /**
     * Class encapsulating run-time configuration.
     */
    class Configuration {
    private:
        std::string logfile;
        std::string webserver_user;
        std::vector<std::string> docroots;
        bool allow_file_group_writeable;
        bool allow_directory_group_writeable;
        bool allow_file_others_writeable;
        bool allow_directory_others_writeable;
        bool check_vhost_docroot;
        bool errors_to_browser;
        std::string env_path;
        std::map<std::string, std::string> handlers;
        LogLevel loglevel;
        int min_uid;
        int min_gid;
        int umask;
        std::string chroot_path;

        /**
         * Converts string to bool
         */
        bool strToBool(const std::string& str) const throw (ParsingException);

        /**
         * Converts string to LogLevel
         */
        LogLevel strToLogLevel(const std::string& str) const
            throw (ParsingException);

    public:
        /**
         * Constructor, initializes configuration with default values.
         */
        Configuration();
        
        /**
         * Reads values from INI file
         */
        void readFromFile(File& file) throw (IOException, ParsingException);
        
        /**
         * Return path to logfile;
         */
        std::string getLogfile() const;

        /**
           Return log level
        */
        LogLevel getLogLevel() const;

        /**
         * Return username of user the webserver is running as
         */
        std::string getWebserverUser() const;

        /**
         * Return document root (list of directories, scripts may be within)
         */
        const std::vector<std::string>& getDocroots() const;

        /**
         * Returns wheter suPHP should check if scripts in within the 
         * document root of the VHost
         */
        bool getCheckVHostDocroot() const;
        
        /**
         * Returns wheter suPHP should ignore the group write bit of
         * the script file
         */
        bool getAllowFileGroupWriteable() const;

        /**
         * Returns wheter suPHP should ignore the group write bit of
         * the directory the is script in
         */
        bool getAllowDirectoryGroupWriteable() const;
        
        /**
         * Returns wheter suPHP should ignore the others write bit of the
         * script file
         */
        bool getAllowFileOthersWriteable() const;

        /**
         * Returns wheter suPHP should ignore the others write bit of
         * the directory the is script in
         */
        bool getAllowDirectoryOthersWriteable() const;

        /**
         * Returns whether (minor) error message should be sent to browser
         */
        bool getErrorsToBrowser() const;
        
        /**
         * Returns the content for the PATH environment variable
         */
        std::string getEnvPath() const;

        /**
         * Returns interpreter string for specified handler
         */
        std::string getInterpreter(std::string handler) const
            throw (KeyNotFoundException);

        /**
         * Returns minimum UID allowed for scripts
         */
        int getMinUid() const;
        
        /**
         * Returns minimum GID allowed for scripts
         */
        int getMinGid() const;

        /**
         * Returns umask to set
         */
        int getUmask() const;

        /**
         * Return chroot path
         */
        std::string getChrootPath() const;
    };
};

#endif // SUPHP_CONFIGURATION_H

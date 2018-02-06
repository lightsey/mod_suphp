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

#include <string>
#include <vector>

#include "IniFile.hpp"
#include "Util.hpp"

#include "Configuration.hpp"

using namespace suPHP;

bool suPHP::Configuration::strToBool(const std::string& bstr) const
    throw (ParsingException) {
    std::string str = bstr;
    // Convert upper characters to lower characters
    for (int i=0; i<str.size(); i++) {
        if (str[i] >= 65 && str[i] <= 90)
            str[i] += 32;
    }

    if (str == std::string("true")) {
        return true;
    } else if (str == std::string("yes")) {
        return true;
    } else if (str == std::string("on")) {
        return true;
    } else if (str == std::string("enabled")) {
        return true;
    } else if (str == std::string("1")) {
        return true;
    } else if (str == std::string("false")) {
        return false;
    } else if (str == std::string("no")) {
        return false;
    } else if (str == std::string("off")) {
        return false;
    } else if (str == std::string("disabled")) {
        return false;
    } else if (str == std::string("0")) {
        return false;
    } else {
        throw ParsingException("\"" + str + "\" is not a valid boolean value",
                               __FILE__, __LINE__);
    }
        
}


LogLevel suPHP::Configuration::strToLogLevel(const std::string& str) const
    throw (ParsingException) {
    if (str == "none")
        return LOGLEVEL_NONE;
    else if (str == "error")
        return LOGLEVEL_ERROR;
    else if (str == "warn")
        return LOGLEVEL_WARN;
    else if (str == "info")
        return LOGLEVEL_INFO;
    else
        throw ParsingException("\"" + str + "\" is not a valid log level",
                               __FILE__, __LINE__);
}

suPHP::Configuration::Configuration() {
    this->logfile = "/var/log/suphp.log";
#ifdef OPT_APACHE_USER
    this->webserver_user = OPT_APACHE_USER;
#else
    this->webserver_user = "wwwrun";
#endif
    this->docroots.push_back("/");
    this->allow_file_group_writeable = false;
    this->allow_directory_group_writeable = false;
    this->allow_file_others_writeable = false;
    this->allow_directory_others_writeable = false;
#ifdef OPT_DISABLE_CHECKPATH
    this->check_vhost_docroot = false;
#else
    this->check_vhost_docroot = true;
#endif
    this->userdir_overrides_usergroup = false;
    this->errors_to_browser = false;
    this->env_path = "/bin:/usr/bin";
    this->loglevel = LOGLEVEL_INFO;
#ifdef OPT_MIN_UID
    this->min_uid = OPT_MIN_UID;
#else
    this->min_uid = 1;
#endif
#ifdef OPT_MIN_GID
    this->min_gid = OPT_MIN_GID;
#else
    this->min_gid = 1;
#endif
    this->umask = 0077;
    this->chroot_path = "";
    this->full_php_process_display = false;
}

void suPHP::Configuration::readFromFile(File& file) 
    throw (IOException, ParsingException) {
    IniFile ini;
    ini.parse(file);
    if (ini.hasSection("global")) {
        const IniSection& sect = ini.getSection("global");
        const std::vector<std::string> keys = sect.getKeys();
        std::vector<std::string>::const_iterator i;
        for (i = keys.begin(); i < keys.end(); i++) {
            std::string key = *i;
            std::string value = sect.getValue(key);

            if (key == "logfile")
                this->logfile = value;
            else if (key == "webserver_user")
                this->webserver_user = value;
            else if (key == "docroot") {
                this->docroots = sect.getValues(key);
            } else if (key == "allow_file_group_writeable")
                this->allow_file_group_writeable = this->strToBool(value);
            else if (key == "allow_directory_group_writeable")
                this->allow_directory_group_writeable = this->strToBool(value);
            else if (key == "allow_file_others_writeable")
                this->allow_file_others_writeable = this->strToBool(value);
            else if (key == "allow_directory_others_writeable")
                this->allow_directory_others_writeable = 
                    this->strToBool(value);
            else if (key == "check_vhost_docroot")
                this->check_vhost_docroot = this->strToBool(value);
	    else if (key == "userdir_overrides_usergroup")
		this->userdir_overrides_usergroup = this->strToBool(value);
            else if (key == "errors_to_browser")
                this->errors_to_browser = this->strToBool(value);
            else if (key == "env_path")
                this->env_path = value;
            else if (key == "loglevel")
                this->loglevel = this->strToLogLevel(value);
            else if (key == "min_uid")
                this->min_uid = Util::strToInt(value);
            else if (key == "min_gid")
                this->min_gid = Util::strToInt(value);
            else if (key == "umask")
                this->umask = Util::octalStrToInt(value);
            else if (key == "chroot")
                this->chroot_path = value;
	    else if (key == "full_php_process_display")
		this->full_php_process_display = this->strToBool(value);
            else 
                throw ParsingException("Unknown option \"" + key + 
                                       "\" in section [global]", 
                                       __FILE__, __LINE__);
        }
    }
    
    // Get handlers / interpreters
    if (ini.hasSection("handlers")) {
        IniSection sect = ini.getSection("handlers");
        const std::vector<std::string> keys = sect.getKeys();
        std::vector<std::string>::const_iterator i;
        for (i = keys.begin(); i < keys.end(); i++) {
            std::string key = *i;
            std::string value = sect.getValue(key);
            std::pair<std::string, std::string> p;
            p.first = key;
            p.second = value;
            this->handlers.insert(p);
        }
    }
    
}

std::string suPHP::Configuration::getLogfile() const {
    return this->logfile;
}

LogLevel suPHP::Configuration::getLogLevel() const {
    return this->loglevel;
}

std::string suPHP::Configuration::getWebserverUser() const {
    return this->webserver_user;
}

const std::vector<std::string>& suPHP::Configuration::getDocroots() const {
    return this->docroots;
}

bool suPHP::Configuration::getCheckVHostDocroot() const {
    return this->check_vhost_docroot;
}

bool suPHP::Configuration::getUserdirOverridesUsergroup() const {
    return this->userdir_overrides_usergroup;
}

bool suPHP::Configuration::getAllowFileGroupWriteable() const {
    return this->allow_file_group_writeable;
}

bool suPHP::Configuration::getAllowDirectoryGroupWriteable() const {
    return this->allow_directory_group_writeable;
}

bool suPHP::Configuration::getAllowFileOthersWriteable() const {
    return this->allow_file_others_writeable;
}

bool suPHP::Configuration::getAllowDirectoryOthersWriteable() const {
    return this->allow_directory_others_writeable;
}

bool suPHP::Configuration::getFullPHPProcessDisplay() const {
    return this->full_php_process_display;
}

bool suPHP::Configuration::getErrorsToBrowser() const {
    return this->errors_to_browser;
}

std::string suPHP::Configuration::getEnvPath() const {
    return this->env_path;
}

std::string suPHP::Configuration::getInterpreter(std::string handler) const 
    throw (KeyNotFoundException) {
    if (this->handlers.find(handler) != this->handlers.end()) {
        return this->handlers.find(handler) -> second;
    } else {
        throw KeyNotFoundException("Handler \"" + handler + "\" not found",
                                   __FILE__, __LINE__);
    }
}

int suPHP::Configuration::getMinUid() const {
    return this->min_uid;
}

int suPHP::Configuration::getMinGid() const {
    return this->min_gid;
}

int suPHP::Configuration::getUmask() const {
    return this->umask;
}

std::string suPHP::Configuration::getChrootPath() const {
    return this->chroot_path;
}

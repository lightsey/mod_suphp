/*
    suPHP - (c)2002-2005 Sebastian Marsching <sebastian@marsching.com>

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

#include <string>
#include <iostream>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <errno.h>

#include "Environment.hpp"
#include "UserInfo.hpp"
#include "GroupInfo.hpp"
#include "API_Linux_Logger.hpp"
#include "Util.hpp"

#include "API_Linux.hpp"

extern char **environ;

using namespace suPHP;

SmartPtr<API_Linux_Logger> suPHP::API_Linux::logger;

bool suPHP::API_Linux::isSymlink(const std::string path) const
    throw (SystemException) {
    struct stat temp;
    if (lstat(path.c_str(), &temp) == -1) {
        throw SystemException(std::string("Could not stat \"")
                              + path + "\": "
                              + ::strerror(errno), __FILE__, __LINE__);
    }
    if ((temp.st_mode & S_IFLNK) == S_IFLNK) {
        return true;
    } else {
        return false;
    }
}
std::string suPHP::API_Linux::readSymlink(const std::string path) const
    throw (SystemException) {
    char buf[1024] = {0};
    if (::readlink(path.c_str(), buf, 1023) == -1) {
        throw SystemException(std::string("Could not read symlink \"")
                              + path + "\": "
                              + ::strerror(errno), __FILE__, __LINE__);
    }
    
    if (buf[0] == '/') {
        return std::string(buf);
    } else {
        if (path.rfind('/') == std::string::npos)
            return std::string(buf);
        return path.substr(0, path.rfind('/') + 1) + std::string(buf);
    }
}

Environment suPHP::API_Linux::getProcessEnvironment() {
    Environment env;
    char **entry = ::environ;
    while (*entry != NULL) {
        std::string estr = std::string(*entry);
        int eqpos = estr.find("=");
        std::string name = estr.substr(0, eqpos);
        std::string content = estr.substr(eqpos + 1);
        env.putVar(name, content);
        entry++;
    }
    return env;
}

UserInfo suPHP::API_Linux::getUserInfo(const std::string username)
    throw (LookupException) {
    struct passwd *tmpuser = ::getpwnam(username.c_str());
    if (tmpuser == NULL) {
        throw LookupException(std::string("Could not lookup username \"") 
                              + username + "\"", __FILE__, __LINE__);
    }
    return UserInfo(tmpuser->pw_uid);
}

UserInfo suPHP::API_Linux::getUserInfo(const int uid) {
    return UserInfo(uid);
}

GroupInfo suPHP::API_Linux::getGroupInfo(const std::string groupname)
    throw (LookupException) {
    struct group *tmpgroup = ::getgrnam(groupname.c_str());
    if (tmpgroup == NULL) {
        throw LookupException(std::string("Could not lookup groupname \"") 
                              + groupname + "\"", __FILE__, __LINE__);
    }
    return GroupInfo(tmpgroup->gr_gid);
}

GroupInfo suPHP::API_Linux::getGroupInfo(const int gid) {
    return GroupInfo(gid);
}

UserInfo suPHP::API_Linux::getEffectiveProcessUser() {
    return UserInfo(::geteuid());
}


UserInfo suPHP::API_Linux::getRealProcessUser() {
    return UserInfo(getuid());
}


GroupInfo suPHP::API_Linux::getEffectiveProcessGroup() {
    return GroupInfo(getegid());
}


GroupInfo suPHP::API_Linux::getRealProcessGroup() {
    return GroupInfo(getgid());
}


Logger& suPHP::API_Linux::getSystemLogger() {
    if (suPHP::API_Linux::logger.get() == NULL) {
        suPHP::API_Linux::logger.reset(new API_Linux_Logger());
    }
    return *(suPHP::API_Linux::logger);
}


void suPHP::API_Linux::setProcessUser(const UserInfo& user) const
    throw (SystemException) {
    // Reset supplementary groups
    if (::setgroups(0, NULL) == -1) {
        throw SystemException(std::string("setgroups() failed: ")
                              + ::strerror(errno), __FILE__, __LINE__);
    }
    
    try {
        if (::initgroups(user.getUsername().c_str(), 
                         user.getGroupInfo().getGid()) 
            == -1) {
            throw SystemException(std::string("initgroups() failed: ")
                                  + ::strerror(errno), __FILE__, __LINE__);
        }
    } catch (LookupException &e) {
        // Ignore this exception
        // If we have a UID, which does not exist in /etc/passwd
        // we simply cannot use supplementary groups
    }

    if (::setuid(user.getUid()) == -1) {
        throw SystemException(std::string("setuid() failed: ") 
                              + ::strerror(errno), __FILE__, __LINE__);
    }
}


void suPHP::API_Linux::setProcessGroup(const GroupInfo& group) const
    throw (SystemException) {
    if (::setgid(group.getGid()) == -1) {
        throw SystemException(std::string("setgid() failed: ") 
                              + ::strerror(errno), __FILE__, __LINE__);
    }
}

std::string suPHP::API_Linux::UserInfo_getUsername(const UserInfo& uinfo) const
    throw (LookupException) {
    struct passwd *tmpuser = ::getpwuid(uinfo.getUid());
    if (tmpuser == NULL) {
        throw LookupException(std::string("Could not lookup UID ")
                              + Util::intToStr(uinfo.getUid()),
                              __FILE__, __LINE__);
    }
    return std::string(tmpuser->pw_name);
}

GroupInfo suPHP::API_Linux::UserInfo_getGroupInfo(const UserInfo& uinfo) const
    throw (LookupException) {
    struct passwd *tmpuser = NULL;
    tmpuser = getpwuid(uinfo.getUid());
    if (tmpuser == NULL) {
        throw LookupException(std::string("Could not lookup UID ") 
                              + Util::intToStr(uinfo.getUid()), 
                              __FILE__, __LINE__);
    }
    return GroupInfo(tmpuser->pw_gid);
}

bool suPHP::API_Linux::UserInfo_isSuperUser(const UserInfo& uinfo) const {
    if (uinfo.getUid() == 0)
        return true;
    else
        return false;
}

std::string suPHP::API_Linux::GroupInfo_getGroupname(const GroupInfo& ginfo) 
    const throw (LookupException) {
    struct group *tmpgroup = ::getgrgid(ginfo.getGid());
    if (tmpgroup == NULL) {
        throw LookupException(std::string("Could not lookup GID ") 
                               + Util::intToStr(ginfo.getGid()),
                              __FILE__, __LINE__);
    }
    return std::string(tmpgroup->gr_name);
}

bool suPHP::API_Linux::File_exists(const File& file) const {
    struct stat dummy;
    if (::stat(file.getPath().c_str(), &dummy) == 0)
        return true;
    else
        return false;
}

std::string suPHP::API_Linux::File_getRealPath(const File& file) const
    throw (SystemException) {
    std::string currentpath = file.getPath();
    std::string resolvedpath = "";
    bool failed = true;

    if ((currentpath.size() == 0) || (currentpath.at(0) != '/')) {
        currentpath = this->getCwd() + std::string("/") + currentpath;
    }
    
    // Limit iterations to avoid infinite symlink loops
    for (int i=0; i<512; i++) {
        // If nothing is left, we have finished
        if (currentpath.size() == 0) {
            resolvedpath = ("/" + resolvedpath);
            failed = false;
            break;
        }
        
        if (this->isSymlink(currentpath)) {
            currentpath = this->readSymlink(currentpath);
        } else {
            // We know last part is not a symlink, so it is resolved
            std::string part1 = 
                currentpath.substr(0, currentpath.rfind('/'));
            std::string part2 = 
                currentpath.substr(currentpath.rfind('/')+1);
            currentpath = part1;
            if (resolvedpath.size() == 0)
                resolvedpath = part2;
            else
                resolvedpath = part2 + "/" + resolvedpath;
        }
    } 
    
    if (failed) {
        throw SystemException("Could not resolve path \"" + 
                              file.getPath() + "\"", __FILE__, __LINE__);
    }

    while (resolvedpath.find("/./") != std::string::npos) {
        int pos = resolvedpath.find("/./");
        resolvedpath = resolvedpath.substr(0, pos)
            + resolvedpath.substr(pos + 2);
    }
    
    while (resolvedpath.find("/../") != std::string::npos) {
        int pos = resolvedpath.find("/../");
        int pos2 = resolvedpath.rfind('/', pos-1);
        resolvedpath = resolvedpath.substr(0, pos2)
            + resolvedpath.substr(pos + 3);
    }
    
    if (resolvedpath.find("/..", resolvedpath.size() - 3) 
        != std::string::npos) {
        resolvedpath = resolvedpath.substr(0, resolvedpath.size() - 3);
        resolvedpath = resolvedpath.substr(0, resolvedpath.rfind('/'));
    }

    if (resolvedpath.find("/.", resolvedpath.size() - 2) 
        != std::string::npos) {
        resolvedpath = resolvedpath.substr(0, resolvedpath.size() - 2);
    }
    
    if (resolvedpath.size() == 0)
        resolvedpath = "/";

    return resolvedpath;
}   

bool suPHP::API_Linux::File_hasPermissionBit(const File& file, FileMode perm) 
    const throw (SystemException) {
    struct stat temp;
    if (stat(file.getPath().c_str(), &temp) == -1) {
        throw SystemException(std::string("Could not stat \"")
                              + file.getPath() + "\": "
                              + ::strerror(errno), __FILE__, __LINE__);
    }
    switch (perm) {
    case FILEMODE_USER_READ:
        if ((temp.st_mode & S_IRUSR) == S_IRUSR)
            return true;
        break;

    case FILEMODE_USER_WRITE:
        if ((temp.st_mode & S_IWUSR) == S_IWUSR)
            return true;
        break;

    case FILEMODE_USER_EXEC:
        if ((temp.st_mode & S_IXUSR) == S_IXUSR)
            return true;
        break;

    case FILEMODE_GROUP_READ:
        if ((temp.st_mode & S_IRGRP) == S_IRGRP)
            return true;
        break;

    case FILEMODE_GROUP_WRITE:
        if ((temp.st_mode & S_IWGRP) == S_IWGRP)
            return true;
        break;

    case FILEMODE_GROUP_EXEC:
        if ((temp.st_mode & S_IXGRP) == S_IXGRP)
            return true;
        break;

    case FILEMODE_OTHERS_READ:
        if ((temp.st_mode & S_IROTH) == S_IROTH)
            return true;
        break;

    case FILEMODE_OTHERS_WRITE:
        if ((temp.st_mode & S_IWOTH) == S_IWOTH)
            return true;
        break;
        
    case FILEMODE_OTHERS_EXEC:
        if ((temp.st_mode & S_IXOTH) == S_IXOTH)
            return true;
        break;
    }

    return false;
}

UserInfo suPHP::API_Linux::File_getUser(const File& file) const
    throw (SystemException) {
    struct stat temp;
    if (stat(file.getPath().c_str(), &temp) == -1) {
        throw SystemException(std::string("Could not stat \"")
                              + file.getPath() + "\": "
                              + ::strerror(errno), __FILE__, __LINE__);
    }
    return UserInfo(temp.st_uid);
}

GroupInfo suPHP::API_Linux::File_getGroup(const File& file) const
    throw (SystemException) {
    struct stat temp;
    if (stat(file.getPath().c_str(), &temp) == -1) {
        throw SystemException(std::string("Could not stat \"")
                              + file.getPath() + "\": "
                              + ::strerror(errno), __FILE__, __LINE__);
    }
    return GroupInfo(temp.st_gid);
}


void suPHP::API_Linux::execute(std::string program, const CommandLine& cline,
                               const Environment& env) const
    throw (SystemException) {
    char **sysCline = NULL;
    char **sysEnv = NULL;
    char **p = NULL;
    char *sysProgram = NULL;
    std::map<std::string, std::string> map;
    int i;


    
    // Construct commandline
    sysCline = new char*[cline.size() + 1];
    for (i=0; i<cline.size(); i++) {
        std::string arg = cline.getArgument(i);
        sysCline[i] = new char[arg.size()+1];
        ::strncpy(sysCline[i], arg.c_str(), arg.size()+1);
    }
    sysCline[cline.size()] = NULL;
    
    // Construct environment
    map = env.getBackendMap();
    sysEnv = new char*[map.size() + 1];
    p = sysEnv;
    for (std::map<std::string, std::string>::iterator pos = map.begin(); 
         pos != map.end(); 
         pos++) {
        std::string var;
        var = pos->first + "=" + pos->second;
        *p = new char[var.size()+1];
        ::strncpy(*p, var.c_str(), var.size()+1);
        p++;
    }
    *p = NULL;

    // Make sure target program name is on heap
    sysProgram = new char[program.size() + 1];
    ::strncpy(sysProgram, program.c_str(), program.size()+1);
    if (execve(sysProgram, sysCline, sysEnv) == -1) {
        throw SystemException("execve() for program \"" + program 
                              + "\" failed: " + ::strerror(errno),
                              __FILE__, __LINE__);
    }
    
    // We are still here? This cannot be good..
    throw SystemException("execve() for program \"" + program 
                          + "\" failed because of unknown reason", 
                          __FILE__, __LINE__);
}

std::string suPHP::API_Linux::getCwd() const throw (SystemException) {
    char buf[4096] = {0};
    if (::getcwd(buf, 4095) == NULL)
        throw SystemException(std::string("getcwd() failed: ")
                              + ::strerror(errno), __FILE__, __LINE__);
    return std::string(buf);
}

void suPHP::API_Linux::setCwd(const std::string& dir) const
    throw (SystemException) {
    if(::chdir(dir.c_str())) {
        throw SystemException(std::string("chdir() failed: ")
                              + ::strerror(errno), __FILE__, __LINE__);
    }
}

void suPHP::API_Linux::setUmask(int mode) const throw (SystemException) {
    ::umask(mode);
}

void suPHP::API_Linux::chroot(const std::string& dir) const
    throw (SystemException) {
    if (::chroot(dir.c_str())) {
        throw SystemException(std::string("chroot() failed: ")
                              + ::strerror(errno), __FILE__, __LINE__);
    }
}

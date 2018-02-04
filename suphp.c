/*
    suPHP - (c)2002 Sebastian Marsching <sebastian@marsching.com>
    
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


#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#include "suphp.h"


void exec_script(char* scriptname)
{
 char *php_config;
 
 // Set the enviroment (for compatibility reasons)
 setenv("SCRIPT_URL", getenv("REQUEST_URI"), 1);
 setenv("PATH_TRANSLATED", getenv("SCRIPT_FILENAME"), 1);
 setenv("PATH_INFO", getenv("SCRIPT_NAME"), 1);
 setenv("REDIRECT_SCRIPT_URL", getenv("SCRIPT_URL"), 1);
 setenv("REDIRECT_STATUS", "200", 1); // PHP may need this
 setenv("REDIRECT_URL", getenv("SCRIPT_URL"), 1);
 
 // Set secure PATH
 
 setenv("PATH", "/bin:/usr/bin", 1);
 
 // Check for PHP_CONFIG environment variable
 
 if (getenv("PHP_CONFIG"))
 {
  if ((php_config = strdup(getenv("PHP_CONFIG")))==NULL)
   error_exit(ERRCODE_UNKNOWN);
  setenv("PHPRC", getenv("PHP_CONFIG"), 1);
  unsetenv("PHP_CONFIG");
 }
 
 // Exec PHP
 if (php_config)
  execl(OPT_PATH_TO_PHP, "php", "-c", php_config, NULL);
 else
  execl(OPT_PATH_TO_PHP, "php", NULL);
}

int main(int argc, char* argv[])
{
 // Check, if program has been started by Apache
 struct passwd *apacheuser;
 struct passwd *calluser;
 struct passwd *targetuser;
 struct group *targetgroup;
 char *path_translated;
 
 path_translated = getenv("SCRIPT_FILENAME");
  
 apacheuser = getpwnam(OPT_APACHE_USER);
 calluser = getpwuid(getuid());
 
 if (calluser->pw_uid!=apacheuser->pw_uid)
 {
  log_error("suPHP was called by %s (%d), not by %s (%d)", calluser->pw_name, calluser->pw_uid, apacheuser->pw_name, apacheuser->pw_uid);
  error_exit(ERRCODE_WRONG_PARENT);
 }
 
#ifdef OPT_CHECKPATH
 // Is the script in the DOCUMENT_ROOT?
 if(!check_path(path_translated))
 {
  log_error("Script %s is not in the DOCUMENT_ROOT (%s)", path_translated, getenv("DOCUMENT_ROOT"));
  error_exit(ERRCODE_WRONG_PATH);
 }
#endif  

 // Does the script exist?
 if(!file_exists(path_translated))
 {
  log_error("File %s not found", path_translated);
  error_exit(ERRCODE_FILE_NOT_FOUND);
 }
 // Check permissions for the script
 if(!check_permissions(path_translated))
  error_exit(ERRCODE_WRONG_PERMISSIONS);
 
 // Get gid and uid of the file and check it
 targetuser = getpwuid(file_get_uid(path_translated));
 if (targetuser->pw_uid < OPT_MIN_UID)
 {
  log_error ("UID of %s (%d / %s) < %d", path_translated, targetuser->pw_uid, targetuser->pw_name, OPT_MIN_UID);
  error_exit(ERRCODE_LOW_UID);
 }
 targetgroup = getgrgid(file_get_gid(path_translated));
 if (targetgroup->gr_gid < OPT_MIN_GID)
 {
  log_error ("GID of %s (%d / %s) < %d", path_translated, targetgroup->gr_gid, targetgroup->gr_name, OPT_MIN_GID);
  error_exit(ERRCODE_LOW_GID);
 }
 
 // We have to create the log entry before losing root privileges
 log_info("%s executed as user %s (%d), group %s (%d)", path_translated, targetuser->pw_name, targetuser->pw_uid, targetgroup->gr_name, targetgroup->gr_gid);
 
 // Set gid and uid
 if (setgid(targetgroup->gr_gid))
 {
  log_error("Could not change GID to %d (%s)", targetgroup->gr_gid, targetgroup->gr_name);
  error_exit(ERRCODE_UNKNOWN); 
 }
 
 if (setuid(targetuser->pw_uid))
 {
  log_error("Could not change UID to %d (%s)", targetuser->pw_uid, targetuser->pw_name);
  error_exit(ERRCODE_UNKNOWN);
 }
 
 // Execute the script with PHP
 exec_script(path_translated);
 
 // Still here? This cannot be right...
 log_error("Error on exec() PHP for %s", path_translated);
 error_exit(ERRCODE_UNKNOWN);
 return ERRCODE_UNKNOWN;
}

/*
    suPHP - (c)2002-2003 Sebastian Marsching <sebastian@marsching.com>
    
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
 execl(OPT_PATH_TO_PHP, "php", NULL);
}

int main(int argc, char* argv[])
{
 // Check, if program has been started by Apache
 struct passwd *apacheuser;
 struct passwd *calluser;
 struct passwd *targetuser;
 struct group *targetgroup;

#ifdef OPT_NO_PASSWD
 // Declare empty structure for user
 struct passwd emptyuser;
 // Declare variable for information whether to use supplementary groups
 int use_supp_groups = 1;
#endif

#ifdef OPT_NO_GROUP
 // Declare emtpy structure for group
 struct group emptygroup;
#endif

#ifdef OPT_NO_PASSWD
 // Initialize structure
 emptyuser.pw_name = "";
 emptyuser.pw_passwd = "";
 emptyuser.pw_uid = 65534;
 emptyuser.pw_gid = 65534;
 emptyuser.pw_gecos = "";
 emptyuser.pw_dir = "/dev/null";
 emptyuser.pw_shell = "/bin/false";
#endif

#ifdef OP_NO_GROUP
 // Initialize structure
 emptygroup.gr_name = "";
 emptygroup.gr_passwd = "";
 emptygroup.gr_gid = 65534;
 emptygroup.gr_mem = NULL;
#endif

 char *path_translated;
 
 path_translated = getenv("SCRIPT_FILENAME");
  
 if ((apacheuser = getpwnam(OPT_APACHE_USER))==NULL)
 {
  log_error("Could not get passwd information for Apache user (%s)", OPT_APACHE_USER);
  error_exit(ERRCODE_UNKNOWN);
 }
 
 if ((calluser = getpwuid(getuid()))==NULL)
 {
  log_error("Could not get passwd information for calling UID %d", getuid());
  error_exit(ERRCODE_UNKNOWN);
 }
 
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
 if ((targetuser = getpwuid(file_get_uid(path_translated)))==NULL)
 {
#ifdef OPT_NO_PASSWD
  emptyuser.pw_uid = file_get_uid(path_translated);
  emptyuser.pw_gid = file_get_gid(path_translated);
  emptyuser.pw_name = "NOT AVAILABLE";
  targetuser = &emptyuser;
  use_supp_groups = 0;
#else
  log_error ("Could not get passwd information for UID %d", file_get_uid(path_translated));
  error_exit(ERRCODE_UNKNOWN);
#endif
 }

 if (targetuser->pw_uid < OPT_MIN_UID)
 {
  log_error ("UID of %s or its target (%d / %s) < %d", path_translated, targetuser->pw_uid, targetuser->pw_name, OPT_MIN_UID);
  error_exit(ERRCODE_LOW_UID);
 }
 
 if ((targetgroup = getgrgid(file_get_gid(path_translated)))==NULL)
 {
#ifdef OPT_NO_GROUP
  emptygroup.gr_gid = file_get_gid(path_translated);
  emptygroup.gr_name = "NOT AVAILABLE";
  targetgroup = &emptygroup;
#else
  log_error ("Could not get group information for GID %d", file_get_gid(path_translated));
  error_exit(ERRCODE_UNKNOWN);
#endif 
 }
 
 if (targetgroup->gr_gid < OPT_MIN_GID)
 {
  log_error ("GID of %s or its target (%d / %s) < %d", path_translated, targetgroup->gr_gid, targetgroup->gr_name, OPT_MIN_GID);
  error_exit(ERRCODE_LOW_GID);
 }

 // Check if file is a symbollink
 if (file_is_symbollink(path_translated))
 {
  // Get gid and uid of the symbollink and check if it matches to the target
  if (targetuser->pw_uid != file_get_uid_l(path_translated))
  {
   log_error ("UID of symbollink %s does not match its target", path_translated);
   error_exit(ERRCODE_SYMBOLLINK_NO_MATCH);
  }
  if (targetgroup->gr_gid != file_get_gid_l(path_translated))
  {
   log_error ("GID of symbollink %s does not match its target", path_translated);
   error_exit(ERRCODE_SYMBOLLINK_NO_MATCH);
  }
 }

 // We have to create the log entry before losing root privileges
 log_info("%s executed as user %s (%d), group %s (%d)", path_translated, targetuser->pw_name, targetuser->pw_uid, targetgroup->gr_name, targetgroup->gr_gid);
 
 // Set gid and uid
 if (setgid(targetgroup->gr_gid))
 {
  log_error("Could not change GID to %d (%s)", targetgroup->gr_gid, targetgroup->gr_name);
  error_exit(ERRCODE_UNKNOWN); 
 }
 
 // Initialize supplementary groups for user
#ifdef OPT_NO_PASSWD
 if (use_supp_groups && initgroups(targetuser->pw_name, targetuser->pw_gid))
 {
  log_error("Could not initialize supplementary groups for user %s (%d)", targetuser->pw_name, targetuser->pw_uid);
  error_exit(ERRCODE_UNKNOWN);
 }
 if (!use_supp_groups)
 {
  if (setgroups(0, NULL))
  {
   log_error("Could not set supplementary groups to null");
   error_exit(ERRCODE_UNKNOWN);
  }
 }
#else
 if (initgroups(targetuser->pw_name, targetuser->pw_gid))
 {
  log_error("Could not initialize supplementary groups for user %s (%d)", targetuser->pw_name, targetuser->pw_uid);
  error_exit(ERRCODE_UNKNOWN);
 }
#endif 

 
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

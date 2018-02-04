/*
    suPHP - (c)2002-2004 Sebastian Marsching <sebastian@marsching.com>
    
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

extern char **environ;

void exec_script(char* scriptname)
{
 char *php_config;
 char *env;
 char *const argv[] = { OPT_PATH_TO_PHP, NULL };
 char *const *envp;
 
 // Set the enviroment (for compatibility reasons)

 if (getenv("SCRIPT_FILENAME") == NULL)
 {
  error_msg_exit(ERRCODE_WRONG_ENVIRONMENT, "SCRIPT_FILENAME is not set", __FILE__, __LINE__);
 } 
 env = strdup(getenv("SCRIPT_FILENAME"));
 suphp_setenv("PATH_TRANSLATED", env, 1);
 free(env);
 
 env = NULL;
 
 suphp_setenv("REDIRECT_STATUS", "200", 1); // PHP may need this
 
 // Set secure PATH
 
 suphp_setenv("PATH", "/bin:/usr/bin", 1);
 
 // Check for PHP_CONFIG environment variable
 
 if (getenv("PHP_CONFIG"))
 {
  if ((php_config = strdup(getenv("PHP_CONFIG")))==NULL)
   error_sysmsg_exit(ERRCODE_UNKNOWN, "strdup() failed", __FILE__, __LINE__);
  suphp_setenv("PHPRC", php_config, 1);
  suphp_unsetenv("PHP_CONFIG");
 }

#if (defined(OPT_USERGROUP_FORCE) || defined(OPT_USERGROUP_PARANOID))
 suphp_unsetenv("PHP_SU_USER");
 suphp_unsetenv("PHP_SU_GROUP");
#endif
 
 envp = suphp_copyenv((const char **) environ);

 // Exec PHP
 execve(OPT_PATH_TO_PHP, argv, envp);
 
 // Should never be reached
 error_sysmsg_exit(ERRCODE_UNKNOWN, "execl() failed", __FILE__, __LINE__);
}

int suphp_passwdcpy(struct passwd *target, struct passwd *source)
{
 if (!target || !source)
  return 0;
 
 if (source->pw_name)
 {
  target->pw_name = strdup(source->pw_name);
  if (!target->pw_name)
   return 0;
 }
 else
  target->pw_name = NULL;
 
 // target->pw_passwd is never needed
 target->pw_passwd = NULL;
 
 target->pw_uid = source->pw_uid;
 
 target->pw_gid = source->pw_gid;
 
 // target->pw_gecos is never needed
 target->pw_gecos = NULL;
 
 // target->pw_dir is never needed
 target->pw_dir = NULL;
 
 // target->pw_shell is never needed
 target->pw_shell = NULL;
 
 return 1;
}

int suphp_groupcpy(struct group *target, struct group *source)
{
 if (!target || !source)
  return 0;
 
 if (source->gr_name)
 {
  target->gr_name = strdup(source->gr_name);
  if (!target->gr_name)
   return 0;
 }
 else
  target->gr_name = NULL;
 
 // target->gr_passwd is never needed
 target->gr_passwd = NULL;
 
 target->gr_gid = source->gr_gid;
 
 // target->gr_mem is never needed
 target->gr_mem = NULL;
 
 return 1;
}

int main(int argc, char* argv[])
{
 // Check, if program has been started by Apache
 struct passwd apacheuser;
 struct passwd calluser;
 struct passwd targetuser;
 struct group targetgroup;
 struct passwd *ptruser;
 struct group *ptrgroup;

#if (defined(OPT_USERGROUP_FORCE) || defined(OPT_USERGROUP_PARANOID))
 char *envusername = NULL;
 char *envgroupname = NULL;
#endif

#ifdef OPT_USERGROUP_PARANOID
 struct passwd envuser;
 struct group envgroup;
#endif

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

 char *path_translated = NULL;
 
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

 // Open logfile
 suphp_init_log();
 
 if(getenv("SCRIPT_FILENAME") != NULL)
  path_translated = strdup(getenv("SCRIPT_FILENAME"));
 else
  error_msg_exit(ERRCODE_WRONG_ENVIRONMENT, "SCRIPT_FILENAME is not set", __FILE__, __LINE__);
  
 if ((ptruser = getpwnam(OPT_APACHE_USER))==NULL)
 {
  suphp_log_error("Could not get passwd information for Apache user (%s)", OPT_APACHE_USER);
  error_sysmsg_exit(ERRCODE_UNKNOWN, "getpwnam() failed", __FILE__, __LINE__);
 }
 if (!suphp_passwdcpy(&apacheuser, ptruser))
 {
  error_sysmsg_exit(ERRCODE_UNKNOWN, "Could not copy struct passwd", __FILE__, __LINE__);
 }
 
 if ((ptruser = getpwuid(getuid()))==NULL)
 {
  suphp_log_error("Could not get passwd information for calling UID %d", getuid());
  error_sysmsg_exit(ERRCODE_UNKNOWN, "getpwuid() failed", __FILE__, __LINE__);
 }
 if (!suphp_passwdcpy(&calluser, ptruser))
 {
  error_sysmsg_exit(ERRCODE_UNKNOWN, "Could not copy struct passwd", __FILE__, __LINE__);
 }

 
 if (calluser.pw_uid!=apacheuser.pw_uid)
 {
  suphp_log_error("suPHP was called by %s (%d), not by %s (%d)", calluser.pw_name, calluser.pw_uid, apacheuser.pw_name, apacheuser.pw_uid);
  error_msg_exit(ERRCODE_WRONG_PARENT, "UID of calling process mismatches", __FILE__, __LINE__);
 }
 
#ifdef OPT_CHECKPATH
 // Is the script in the DOCUMENT_ROOT?
 if(!check_path(path_translated))
 {
  suphp_log_error("Script %s is not in the DOCUMENT_ROOT (%s)", path_translated, getenv("DOCUMENT_ROOT"));
  error_msg_exit(ERRCODE_WRONG_PATH, "Script is not in document root", __FILE__, __LINE__);
 }
#endif  

 // Does the script exist?
 if(!file_exists(path_translated))
 {
  suphp_log_error("File %s not found", path_translated);
  error_msg_exit(ERRCODE_FILE_NOT_FOUND, "Script not found", __FILE__, __LINE__);
 }
 // Check permissions for the script
 if(!check_permissions(path_translated))
  error_msg_exit(ERRCODE_WRONG_PERMISSIONS, "Inappropriate permissions set on script", __FILE__, __LINE__);
 
#if (defined(OPT_USERGROUP_FORCE) || defined(OPT_USERGROUP_PARANOID))
 if (getenv("PHP_SU_USER") != NULL)
  envusername = strdup(getenv("PHP_SU_USER"));
 else
  error_msg_exit(ERRCODE_WRONG_ENVIRONMENT, "PHP_SU_USER is not set", __FILE__, __LINE__);
 
 if ((ptruser = getpwnam(envusername)) == NULL)
 {
  suphp_log_error("getpwnam() for user %s failed", envusername);
  error_msg_exit(ERRCODE_UNKNOWN, "getpwnam() failed", __FILE__, __LINE__);
 }
 else
 {
#ifdef OPT_USERGROUP_PARANOID
  if (!suphp_passwdcpy(&envuser, ptruser))
  {
   error_sysmsg_exit(ERRCODE_UNKNOWN, "Could not copy struct passwd", __FILE__, __LINE__);
  }

#endif
#ifdef OPT_USERGROUP_FORCE
  if (!suphp_passwdcpy(&targetuser, ptruser))
  {
   error_sysmsg_exit(ERRCODE_UNKNOWN, "Could not copy struct passwd", __FILE__, __LINE__);
  }

#endif
  free(envusername);
  envusername = NULL;
 }
#endif  
 
#if (defined(OPT_USERGROUP_OWNER) || defined(OPT_USERGROUP_PARANOID))
 // Get gid and uid of the file and check it
 if ((ptruser = getpwuid(file_get_uid(path_translated)))==NULL)
 {
#ifdef OPT_NO_PASSWD
  emptyuser.pw_uid = file_get_uid(path_translated);
  emptyuser.pw_gid = file_get_gid(path_translated);
  emptyuser.pw_name = "NOT AVAILABLE";
  if (!suphp_passwdcpy(&targetuser, &emptyuser))
  {
   error_sysmsg_exit(ERRCODE_UNKNOWN, "Could not copy struct passwd", __FILE__, __LINE__);
  }

  use_supp_groups = 0;
#else
  suphp_log_error ("Could not get passwd information for UID %d", file_get_uid(path_translated));
  error_sysmsg_exit(ERRCODE_UNKNOWN, "getpwuid() failed", __FILE__, __LINE__);
#endif
 }
 else
 {
  if (!suphp_passwdcpy(&targetuser, ptruser))
  {
   error_sysmsg_exit(ERRCODE_UNKNOWN, "Could not copy struct passwd", __FILE__, __LINE__);
  }

 }
#endif

 if (targetuser.pw_uid < OPT_MIN_UID)
 {
  suphp_log_error("UID of %s or its target (%d / %s) < %d", path_translated, targetuser.pw_uid, targetuser.pw_name, OPT_MIN_UID);
  error_msg_exit(ERRCODE_LOW_UID, "User is not allowed to run scripts", __FILE__, __LINE__);
 }

#ifdef OPT_USERGROUP_PARANOID
 if (targetuser.pw_uid != envuser.pw_uid)
 {
  suphp_log_error("UID of owner of %s or its target (%d / %s) mismatches target UID specified in configuration (%d / %s)", path_translated, targetuser.pw_uid, targetuser.pw_name, envuser.pw_uid, envuser.pw_name);
  error_msg_exit(ERRCODE_WRONG_UID, "Script has wrong UID", __FILE__, __LINE__);
 }
#endif
 
#if (defined(OPT_USERGROUP_FORCE) || defined(OPT_USERGROUP_PARANOID))
 if (getenv("PHP_SU_GROUP") != NULL)
  envgroupname = strdup(getenv("PHP_SU_GROUP"));
 else
  error_msg_exit(ERRCODE_WRONG_ENVIRONMENT, "PHP_SU_GROUP is not set", __FILE__, __LINE__);
 
 if ((ptrgroup = getgrnam(envgroupname)) == NULL)
 {
  suphp_log_error("getgrnam() for group %s failed", envgroupname);
  error_msg_exit(ERRCODE_UNKNOWN, "getgrnam() failed", __FILE__, __LINE__);
 }
 else
 {
#ifdef OPT_USERGROUP_PARANOID
  memcpy(&envgroup, ptrgroup, sizeof(struct group));
  if (!suphp_groupcpy(&envgroup, ptrgroup))
  {
   error_sysmsg_exit(ERRCODE_UNKNOWN, "Could not copy struct group", __FILE__, __LINE__);
  }
#endif

#ifdef OPT_USERGROUP_FORCE
  memcpy(&targetgroup, ptrgroup, sizeof(struct group));
  if (!suphp_groupcpy(&targetgroup, ptrgroup))
  {
   error_sysmsg_exit(ERRCODE_UNKNOWN, "Could not copy struct group", __FILE__, __LINE__);
  }

#endif
  free(envgroupname);
 }
#endif  

#if (defined(OPT_USERGROUP_OWNER) || defined(OPT_USERGROUP_PARANOID))
 if ((ptrgroup = getgrgid(file_get_gid(path_translated)))==NULL)
 {
#ifdef OPT_NO_GROUP
  emptygroup.gr_gid = file_get_gid(path_translated);
  emptygroup.gr_name = "NOT AVAILABLE";
  memcpy(&targetgroup,  &emptygroup, sizeof(struct group));
  if (!suphp_groupcpy(&targetgroup, &emptygroup))
  {
   error_sysmsg_exit(ERRCODE_UNKNOWN, "Could not copy struct group", __FILE__, __LINE__);
  }

#else
  suphp_log_error ("Could not get group information for GID %d", file_get_gid(path_translated));
  error_msg_exit(ERRCODE_UNKNOWN, "getgrgid() failed", __FILE__, __LINE__);
#endif 
 }
 else
 {
  if (!suphp_groupcpy(&targetgroup, ptrgroup))
  {
   error_sysmsg_exit(ERRCODE_UNKNOWN, "Could not copy struct group", __FILE__, __LINE__);
  }

 }
#endif
 
 if (targetgroup.gr_gid < OPT_MIN_GID)
 {
  suphp_log_error ("GID of %s or its target (%d / %s) < %d", path_translated, targetgroup.gr_gid, targetgroup.gr_name, OPT_MIN_GID);
  error_msg_exit(ERRCODE_LOW_GID, "Group is not allowed to run scripts", __FILE__, __LINE__);
 }

#ifdef OPT_USERGROUP_PARANOID
 if (targetgroup.gr_gid != envgroup.gr_gid)
 {
  suphp_log_error("GID of group-owner of %s or its target (%d / %s) mismatches target GID specified in configuration (%d / %s)", path_translated, targetgroup.gr_gid, targetgroup.gr_name, envgroup.gr_gid, envgroup.gr_name);
  error_msg_exit(ERRCODE_WRONG_GID, "Script has wrong GID", __FILE__, __LINE__);
 }
#endif

#if (defined(OPT_USERGROUP_OWNER) || defined(OPT_USERGROUP_PARANOID))
 // Check if file is a symbollink
 if (file_is_symbollink(path_translated))
 {
  // Get gid and uid of the symbollink and check if it matches to the target
  if (targetuser.pw_uid != file_get_uid_l(path_translated))
  {
   suphp_log_error ("UID of symbollink %s does not match its target", path_translated);
   error_msg_exit(ERRCODE_SYMBOLLINK_NO_MATCH, "Symbol link UID mismatches target's UID", __FILE__, __LINE__);
  }
  if (targetgroup.gr_gid != file_get_gid_l(path_translated))
  {
   suphp_log_error ("GID of symbollink %s does not match its target", path_translated);
   error_msg_exit(ERRCODE_SYMBOLLINK_NO_MATCH, "Symbol link GID mismatches target'S GID", __FILE__, __LINE__);
  }
 }
#endif

 // We have to create the log entry before losing root privileges
 suphp_log_info("Executing %s as user %s (%d), group %s (%d)", path_translated, targetuser.pw_name, targetuser.pw_uid, targetgroup.gr_name, targetgroup.gr_gid);
 
 // Set gid and uid
 if (setgid(targetgroup.gr_gid))
 {
  suphp_log_error("Could not change GID to %d (%s)", targetgroup.gr_gid, targetgroup.gr_name);
  error_sysmsg_exit(ERRCODE_UNKNOWN, "setgid() failed", __FILE__, __LINE__); 
 }
 
 // Initialize supplementary groups for user
#ifdef OPT_NO_PASSWD
 if (use_supp_groups && initgroups(targetuser.pw_name, targetuser.pw_gid))
 {
  suphp_log_error("Could not initialize supplementary groups for user %s (%d)", targetuser.pw_name, targetuser.pw_uid);
  error_sysmsg_exit(ERRCODE_UNKNOWN, "initgroups() failed", __FILE__, __LINE__);
 }
 if (!use_supp_groups)
 {
  if (setgroups(0, NULL))
  {
   suphp_log_error("Could not set supplementary groups to null");
   error_sysmsg_exit(ERRCODE_UNKNOWN, "setgroups() failed", __FILE__, __LINE__);
  }
 }
#else
 if (initgroups(targetuser.pw_name, targetuser.pw_gid))
 {
  suphp_log_error("Could not initialize supplementary groups for user %s (%d)", targetuser.pw_name, targetuser.pw_uid);
  error_sysmsg_exit(ERRCODE_UNKNOWN, "initgroups() failed", __FILE__, __LINE__);
 }
#endif 

 
 if (setuid(targetuser.pw_uid))
 {
  suphp_log_error("Could not change UID to %d (%s)", targetuser.pw_uid, targetuser.pw_name);
  error_sysmsg_exit(ERRCODE_UNKNOWN, "setuid() failed", __FILE__, __LINE__);
 }
 
 // Execute the script with PHP
 exec_script(path_translated);
 
 // Still here? This cannot be right...
 suphp_log_error("Error on exec() PHP for %s", path_translated);
 error_exit(ERRCODE_UNKNOWN);
 return ERRCODE_UNKNOWN;
}

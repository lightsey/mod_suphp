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

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "suphp.h"

int check_path(char *script_path)
{
 char *document_root = getenv("DOCUMENT_ROOT");
 if(!document_root)
 {
  log_error("Could not get DOCUMENT_ROOT from the environment processing %s", script_path);
  error_exit(ERRCODE_WRONG_ENVIROMENT);
 }
 if (strncmp(document_root, script_path, strlen(document_root))!=0)
  return 0;
 else
  return 1;
}

int check_permissions(char *script_path)
{
 struct stat file_info;
 
 if (stat(script_path, &file_info))
 {
  log_error("Could not stat() script %s", script_path);
  error_exit(ERRCODE_UNKNOWN);
 }
 
 if (!(file_info.st_mode & S_IRUSR))
 {
  log_error("Owner doesn't have read permission on %s", script_path);
  return 0;
 }
 
 if (file_info.st_mode & (S_IWGRP | S_IWOTH))
 {
  log_error("Script (%s) is writeable by others", script_path);
  return 0;
 }
  
 return 1;
}

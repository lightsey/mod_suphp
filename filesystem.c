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
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "suphp.h"

int file_exists(char *filename)
{
 int filehandler;
 if ((filehandler=open(filename, O_RDONLY))==-1)
  return 0;
 close(filehandler);
 return 1;
}

uid_t file_get_uid(char *filename)
{
 struct stat *fileinfo;
 uid_t uid;

 fileinfo = (struct stat*) malloc(sizeof(struct stat));
 if (stat(filename, fileinfo))
 {
  log_error("Could not stat() %s", filename);
  error_exit(ERRCODE_UNKNOWN);
 }
 uid = fileinfo->st_uid;
 free(fileinfo);
 fileinfo = NULL;
 return uid;
}

gid_t file_get_gid(char *filename)
{
 struct stat *fileinfo;
 gid_t gid;

 fileinfo = (struct stat*) malloc(sizeof(struct stat));
 if (stat(filename, fileinfo))
 {
  log_error("Could not stat() %s", filename);
  error_exit(ERRCODE_UNKNOWN);
 }
 gid = fileinfo->st_gid;
 free(fileinfo);
 fileinfo = NULL;
 return gid;
}

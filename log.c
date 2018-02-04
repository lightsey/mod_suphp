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


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include "suphp.h"

void log_error(char *err_msg, ...)
{
 char msg[4096];
 va_list vargs;
 
 
 // Build error message
 va_start(vargs, err_msg);
 vsnprintf(msg, 4095, err_msg, vargs);
 va_end(vargs); 
 
 // Write message to logfile
 log(msg, "error");
}

void log_info(char *info_msg, ...)
{
 char msg[4096];
 va_list vargs;
 
 
 // Build error message
 va_start(vargs, info_msg);
 vsnprintf(msg, 4095, info_msg, vargs);
 va_end(vargs); 
 
 // Write message to logfile
 log(msg, "info");
}

void log(char *msg, char *category)
{
 int fd;
 time_t ts;
 struct tm *now;
 char str_time[1024];
 char row[8192];
 
 
 // Open logfile
 if ((fd = open(OPT_LOGFILE, O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR)) == -1)
  error_exit(ERRCODE_NO_LOG);
 
 // Get time
 ts = time(NULL);
 now = localtime(&ts);
 strftime(str_time, 1023, "[%a %b %d %H:%M:%S %Y]", now);
 
 // Build entry for logfile
 snprintf(row, 8191, "%s [%s] %s\n", str_time, category, msg);
 
 // Write to logfile
 write(fd, row, strlen(row));
 
 // Close logfile
 close(fd);
}

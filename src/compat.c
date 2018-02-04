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

#include <stdlib.h>
#include <string.h>

#include "suphp.h"

int suphp_setenv(const char *name, const char *value, int overwrite)
{
 char *putstr;
 char *temp;
 int retval;
 if (!overwrite && getenv(name) != NULL)
  return 0;
 temp = NULL;
 putstr = malloc(strlen(name) + strlen(value) + 2);
 if (putstr == NULL)
  error_sysmsg_exit(ERRCODE_MEMORY, "Could not allocate memory", __FILE__, __LINE__);
 strcpy(putstr, name);
 temp = putstr + strlen(name);
 *temp = '=';
 temp++;
 strcpy(temp, value);
 retval = putenv(putstr);
 return retval;
}

int suphp_unsetenv(const char *name)
{
 char *putstr = malloc(strlen(name) + 2);
 char *temp;
 int retval;
 if (putstr == NULL)
  error_sysmsg_exit(ERRCODE_MEMORY, "Could not allocate memory", __FILE__, __LINE__);
 strcpy(putstr, name);
 temp = putstr + strlen(name);
 *temp = '=';
 temp++;
 *temp = 0;
 retval = putenv(putstr);
 return retval;
}

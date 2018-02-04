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

#ifndef SUPHP_FILESYSTEM_H
#define SUPHP_FILESYSTEM_H

#include <sys/types.h>

// Functions in filesystem.c
int file_exists(char *filename);
int file_is_symbollink(char *filename);
uid_t file_get_uid(char *filename);
gid_t file_get_gid(char *filename);
uid_t file_get_uid_l(char *filename);
gid_t file_get_gid_l(char *filename);

#endif


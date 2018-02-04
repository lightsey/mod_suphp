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


// Errorcode, only used internally
#define ERRCODE_WRONG_ENVIROMENT 1
#define ERRCODE_WRONG_PATH 2
#define ERRCODE_LOW_UID 3
#define ERRCODE_LOW_GID 6
#define ERRCODE_FILE_NOT_FOUND 4
#define ERRCODE_WRONG_PARENT 5
#define ERRCODE_WRONG_PERMISSIONS 7
#define ERRCODE_NO_LOG 8
#define ERRCODE_UNKNOWN 9999

// Functions in error.c
void error_exit(int errcode);

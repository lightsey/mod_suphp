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


#include "config.h"
#include "filesystem.h"
#include "check.h"
#include "error.h"
#include "log.h"

#ifndef OPT_DISABLE_CHECKPATH
#define OPT_CHECKPATH
#endif

#ifndef OPT_MIN_UID
#define OPT_MIN_UID 100
#endif

#ifndef OPT_MIN_GID
#define OPT_MIN_GID 100
#endif

#ifndef OPT_APACHE_USER
#define OPT_APACHE_USER "wwwrun"
#endif

#ifndef OPT_PATH_TO_PHP
#define OPT_PATH_TO_PHP "/usr/bin/php"
#endif

#ifndef OPT_LOGFILE
#define OPT_LOGFILE "/var/log/httpd/suphp_log"
#endif


# Process this file with autoconf to produce a configure script.
AC_INIT([suphp], [0.5.2], [sebastian.marsching@suphp.org])
AC_CONFIG_SRCDIR([src/suphp.c])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADER([src/config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

AC_MSG_CHECKING([for dynamic Apache module support (via APXS)])
AC_ARG_WITH(apxs, 
            AC_HELP_STRING([--with-apxs=FILE], 
                           [Build shared Apache module.  FILE is the optional pathname to the Apache apxs tool; defaults to "apxs".]),
            [
            if test "$withval" = "yes"; then
              APXS=apxs
            else
              APXS="$withval"
            fi
            ])

if test -z "$APXS"; then
  APXS=`which apxs`
fi

if test "$BINNAME" = "" -a "$APXS" = "" -a "$FAIL_STATIC" = ""; then
  for i in /usr/sbin /usr/local/apache/bin ; do
    if test -f "$i/apxs"; then
      APXS="$i/apxs"
    fi
  done
fi

if test -n "$APXS"; then
    AC_SUBST(APXS)
    
    APACHE_VERSION=`\`$APXS -q SBINDIR\`/\`$APXS -q TARGET\` -v \
                    | grep "Server version" \
                    | cut -f2 -d":" \
                    | cut -f2 -d"/" \
                    | cut -f1 -d" "`
    major_version=`echo $APACHE_VERSION|cut -f1,2 -d.`
    if test "$major_version" = "2.0"; then
      APACHE_VERSION_2=true
      APACHE_VERSION_1_3=false
    else
      APACHE_VERSION_2=false
      APACHE_VERSION_1_3=true
    fi
    AC_SUBST(APACHE_VERSION_1_3)
    AC_SUBST(APACHE_VERSION_2)

    AC_MSG_RESULT(found at $APXS (version $APACHE_VERSION))
else
    APXS="/notfound/"
    AC_SUBST(APXS)
    
    AC_MSG_RESULT(no)
fi

AC_MSG_CHECKING([for set-UID/set-GID mode])
AC_ARG_WITH(setid-mode, 
            AC_HELP_STRING([--with-setid-mode=MODE], 
                           [Mode to use for setting UID/GID.  MODE can be on of "owner", "config" or "paranoid"; defaults to "owner".]),
            [
            if test "$withval" = "yes"; then
              SETID_MODE="owner"
            else
              SETID_MODE="$withval"
            fi
            ])

if test -z "$SETID_MODE" ; then
  SETID_MODE="owner"
fi

if test -n "$SETID_MODE"; then
    case "$SETID_MODE" in 
      owner)
        OPT_APACHEMOD_USERGROUP_DEF=""
        AC_SUBST(OPT_APACHEMOD_USERGROUP_DEF)
        AC_DEFINE(OPT_USERGROUP_OWNER, 1,
                  [Define if you want to set UID/GID to the owner of the script])
        ;;
      force)
        OPT_APACHEMOD_USERGROUP_DEF=-DSUPHP_USE_USERGROUP
        AC_SUBST(OPT_APACHEMOD_USERGROUP_DEF)
        AC_DEFINE(OPT_USERGROUP_FORCE, 1,
                  [Define if you want to set UID/GID to the user/group specified in the Apache configuration])
        ;;
      paranoid)
        OPT_APACHEMOD_USERGROUP_DEF=-DSUPHP_USE_USERGROUP
        AC_SUBST(OPT_APACHEMOD_USERGROUP_DEF)
        AC_DEFINE(OPT_USERGROUP_PARANOID, 1,
                  [Define if you want to set UID/GID to the user/group specified in the Apache configuration AND check if these settings match the UID/GID of the script])
        ;;
      *)
        AC_MSG_ERROR([--with-setid-mode has to be set to one of "owner", "force" or "paranoid"])
        ;;
    esac
    AC_MSG_RESULT([ok - using $SETID_MODE])
fi

checkpath=yes
AC_ARG_ENABLE([checkpath], 
              AC_HELP_STRING([--enable-checkpath],
                             [Check if script resides in DOCUMENT_ROOT (default is ENABLED)]),
              [
              if test "$enableval" = "no"; then
	        checkpath=no
                AC_DEFINE(OPT_DISABLE_CHECKPATH, 1, [Define if you want to disable the check, wether script resides in DOCUMENT_ROOT])
	      fi
	      ])

checkuid=yes
AC_ARG_ENABLE([checkuid],
              AC_HELP_STRING([--enable-checkuid],
                             [Do not accept UIDs that are not listed in /etc/passwd (default is ENABLED)]),
              [
              if test "$enableval" = "no"; then
                checkuid=no
                AC_DEFINE(OPT_NO_PASSWD, 1, 
                          [Define if you want to proceed even if UID of the script is not listed in /etc/passwd])
              fi
              ])

checkgid=yes
AC_ARG_ENABLE([checkgid],
              AC_HELP_STRING([--enable-checkgid],
                             [Do not accept GIDs that are not listed in /etc/group (default is ENABLED)]),
              [
              if test "$enableval" = "no"; then
                checkgid=no
                AC_DEFINE(OPT_NO_GROUP, 1, 
                          [Define if you want to proceed even if GID of the script is not listed in /etc/group])
              fi
              ])

AC_ARG_WITH([min-uid], 
            AC_HELP_STRING([--with-min-uid=UID],
                           [Minimum UID, which is allowed to run scripts
                            (default=100)]),
            [
             if test "$withval" -a ! "$withval" = "yes" ; then
	       AC_DEFINE_UNQUOTED(OPT_MIN_UID, $withval, [Defines the min UID
	                 allowed to run scripts])
             fi
	    ],
	    [
	      AC_DEFINE(OPT_MIN_UID, 100, [Defines the min UID
	                               allowed to run scripts])
            ])

AC_ARG_WITH([min-gid],
            AC_HELP_STRING([--with-min-gid=GID],
                           [Minimum GID, which is allowed to run scripts
                            (default=100)]),
            [
             if test "$withval" -a ! "$withval" = "yes" ; then
               AC_DEFINE_UNQUOTED(OPT_MIN_GID, $withval, [Defines the min GID
                         allowed to run scripts])
             fi
            ],
            [
              AC_DEFINE(OPT_MIN_GID, 100, [Defines the min GID
                                       allowed to run scripts])
            ])

AC_ARG_WITH([apache-user],
            AC_HELP_STRING([--with-apache-user=USERNAME],
                           [Name of the user Apache is running as
(default is "wwwrun"]),
            [
             if test "$withval" -a ! "$withval" = "yes" ; then
               AC_DEFINE_UNQUOTED(OPT_APACHE_USER, "$withval", [Defines the username of the Apache user])
             fi
            ],
            [
             AC_DEFINE_UNQUOTED(OPT_APACHE_USER, "wwwrun", [Defines the username of the Apache user])
            ])

AC_ARG_WITH([php],
            AC_HELP_STRING([--with-php=FILE],
                           [Path to PHP interpreter (default is "/usr/bin/php"]),
            [ 
             if test "$withval" -a ! "$withval" = "yes" ; then 
               AC_DEFINE_UNQUOTED(OPT_PATH_TO_PHP, "$withval", [Defines path to PHP interpreter])
             fi
            ],
            [
             AC_DEFINE_UNQUOTED(OPT_PATH_TO_PHP, "/usr/bin/php", [Defines path to PHP interpreter])
            ])

AC_ARG_WITH([logfile],
            AC_HELP_STRING([--with-logfile=FILE],
                           [Path to suPHP logfile (default is "/var/log/httpd/suphp_log"]),
            [
             if test "$withval" -a ! "$withval" = "yes" ; then
               AC_DEFINE_UNQUOTED(OPT_LOGFILE, "$withval", [Defines
path to logfile])
             fi
            ],
            [
             AC_DEFINE_UNQUOTED(OPT_LOGFILE, "/var/log/httpd/suphp_log", [Defines path to logfile])
            ])


# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_UID_T
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_LSTAT
AC_FUNC_MALLOC
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([setenv strcasecmp strdup strrchr])

AC_CONFIG_FILES([Makefile src/Makefile src/apache/Makefile src/apache2/Makefile])
AC_OUTPUT

if test -n "$OPT_APACHEMOD_USERGROUP_DEF" -a "$APACHE_VERSION_1_3" = "true"; then
  AC_MSG_WARN([
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!*** The suPHP module for Apache 1.3 only works with     ***!!
!!*** set-ID mode "owner", other modes are only supported ***!!
!!*** by the Apache 2.x module                            ***!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  ])
fi

if test "$APXS" = "/notfound/"; then
  AC_MSG_WARN([
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!*** APXS was not found, so mod_suphp will not be built! ***!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  ])
fi


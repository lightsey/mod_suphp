# Process this file with autoconf to produce a configure script.

# Initialize
AC_INIT([suPHP], [0.8.0-pre], [john@nixnuts.net], [suphp])

# Auxiliary tools
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])

# Check for right directory
AC_CONFIG_SRCDIR([src/Application.cpp])
# Config headers for automake
AM_CONFIG_HEADER([src/config.h])

AM_INIT_AUTOMAKE

# Build time sanity check
AM_SANITY_CHECK

# Look for install program
AC_PROG_INSTALL

# Look for compiler
AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_CXX
AM_PROG_LIBTOOL

AX_CXX_COMPILE_STDCXX_11(,optional)
if test "$HAVE_CXX11" != "1"; then
  # G++ 4.4 fails the full c++11 test but includes the features
  # that are actually being used (auto and unique_ptr).
  # The older autoconf STDCXX_0X will pass it.
  AX_CXX_COMPILE_STDCXX_0X
  if test "x$ax_cv_cxx_compile_cxx0x_native" != xyes; then
    if test "x$ax_cv_cxx_compile_cxx0x_gxx" = xyes; then
      CXXFLAGS="$CXXFLAGS -std=gnu++0x"
    elif test "x$ax_cv_cxx_compile_cxx0x_cxx" = xyes; then
      CXXFLAGS="$CXXFLAGS -std=c++0x"
    else
      AC_MSG_ERROR([A compiler with C++11 or C++0x support is required to build mod_suphp])
    fi
  fi
fi

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h unistd.h])

# Find apr-config

APR_FIND_APR([], [], [1], [1])

# Determine Apache version and find apxs

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
    if test "$major_version" = "2.4"; then
      APXS_EXTRA_CFLAGS=`$APXS -q CFLAGS`
    else
      APXS_EXTRA_CFLAGS=`$APXS -q EXTRA_CFLAGS`
    fi
    APXS_INCLUDEDIR=`$APXS -q INCLUDEDIR`
    APXS_LIBEXECDIR=`$APXS -q LIBEXECDIR`
    AC_SUBST([APXS_INCLUDEDIR])
    AC_SUBST([APXS_LIBEXECDIR])
    AC_SUBST([APXS_EXTRA_CFLAGS])

    AC_MSG_RESULT(found at $APXS (version $APACHE_VERSION))
else
    APXS="/notfound/"
    AC_SUBST(APXS)
    
    AC_MSG_RESULT(no)
fi

dnl For Apache 2.x APR is needed
if test x"${apr_found}" = xyes ; then \
    APR_INCLUDEDIR=`${apr_config} --includedir`
    AC_SUBST([APR_INCLUDEDIR])
    APR_CPPFLAGS=`${apr_config} --cppflags`
    AC_SUBST([APR_CPPFLAGS])
else
    AC_MSG_ERROR([APR is needed to build mod_suphp for Apache 2.x but was not found])
fi

# Get uid/gid mode

AC_MSG_CHECKING([for set-UID/set-GID mode])
AC_ARG_WITH(setid-mode, 
            AC_HELP_STRING([--with-setid-mode=MODE], 
                           [Mode to use for setting UID/GID.  MODE can be on of "owner", "config" or "paranoid"; defaults to "paranoid".]),
            [
            if test "$withval" = "yes"; then
              SETID_MODE="paranoid"
            else
              SETID_MODE="$withval"
            fi
            ])

if test -z "$SETID_MODE" ; then
  SETID_MODE="paranoid"
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


# Checkpath (docroot) option

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


# Minimum UID

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

# Minimum GID

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


# Webserver user
AC_ARG_WITH([apache-user],
            AC_HELP_STRING([--with-apache-user=USERNAME],
                           [Name of the user Apache is running as
(default is "www-data" on Debian, "apache" on RedHat, and "wwwrun" on other systems)]),
            [
             if test "$withval" -a ! "$withval" = "yes" ; then
               webserver_user = "$withval"
             fi
            ])

if test -z "$webserver_user"; then
    if test -e "/etc/redhat-release" ; then
        webserver_user="apache"
    elif test -e "/etc/debian_version" ; then
        webserver_user="www-data"
    else
        webserver_user="wwwrun"
    fi
fi

AC_DEFINE_UNQUOTED(OPT_APACHE_USER, "$webserver_user", [Defines the username of the Apache user])

# Path to logfile

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


# Conditionally include UserGroup support
AM_CONDITIONAL([COND_APUSERGROUP], [test \( x"$SETID_MODE" = xparanoid \) -o \( x"$SETID_MODE" = xforce \) ])

AC_CONFIG_FILES([Makefile src/Makefile src/apache2/Makefile])
AC_OUTPUT

if test "$APXS" = "/notfound/"; then
  AC_MSG_WARN([
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
!!*** APXS was not found, so mod_suphp will not be built! ***!!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  ])
fi


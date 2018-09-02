AM_CXXFLAGS=-DOPT_CONFIGFILE=\"${sysconfdir}/suphp.conf\"

SUBDIRS = apache2
DIST_SUBDIRS = apache2

sbin_PROGRAMS = suphp

suphp_SOURCES = Application.cpp
suphp_LDADD = libsuphp.la

noinst_LTLIBRARIES = libsuphp.la
libsuphp_la_SOURCES = API.cpp API.hpp API_Helper.cpp API_Helper.hpp API_Linux.cpp API_Linux.hpp API_Linux_Logger.cpp API_Linux_Logger.hpp Application.hpp CommandLine.cpp CommandLine.hpp Configuration.cpp Configuration.hpp Environment.cpp Environment.hpp Exception.cpp Exception.hpp File.cpp File.hpp GroupInfo.cpp GroupInfo.hpp IOException.cpp IOException.hpp IniFile.cpp IniFile.hpp IniSection.cpp IniSection.hpp KeyNotFoundException.cpp KeyNotFoundException.hpp Logger.cpp Logger.hpp LookupException.cpp LookupException.hpp OutOfRangeException.cpp OutOfRangeException.hpp PathMatcher.hpp PathMatcher.cpp ParsingException.cpp ParsingException.hpp PointerException.cpp PointerException.hpp SecurityException.cpp SecurityException.hpp SmartPtr.hpp SoftException.cpp SoftException.hpp SystemException.cpp SystemException.hpp UserInfo.cpp UserInfo.hpp Util.cpp Util.hpp
libsuphp_la_LDFLAGS = -static

install-exec-hook:
	chmod u+s $(DESTDIR)$(sbindir)/suphp$(EXEEXT)

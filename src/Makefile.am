AM_CXXFLAGS=-DOPT_CONFIGFILE=\"${sysconfdir}/suphp.conf\"

if COND_AP13
  MAYBE_AP = apache
endif
if COND_AP20
  MAYBE_AP = apache2
endif
SUBDIRS = $(MAYBE_AP)
DIST_SUBDIRS = apache apache2

sbin_PROGRAMS = suphp

suphp_SOURCES = API.cpp API.hpp API_Helper.cpp API_Helper.hpp API_Linux.cpp API_Linux.hpp API_Linux_Logger.cpp API_Linux_Logger.hpp Application.cpp Application.hpp CommandLine.cpp CommandLine.hpp Configuration.cpp Configuration.hpp Environment.cpp Environment.hpp Exception.cpp Exception.hpp File.cpp File.hpp GroupInfo.cpp GroupInfo.hpp IOException.cpp IOException.hpp IniFile.cpp IniFile.hpp IniSection.cpp IniSection.hpp KeyNotFoundException.cpp KeyNotFoundException.hpp Logger.cpp Logger.hpp LookupException.cpp LookupException.hpp OutOfRangeException.cpp OutOfRangeException.hpp PathMatcher.hpp PathMatcher.cpp ParsingException.cpp ParsingException.hpp PointerException.cpp PointerException.hpp SecurityException.cpp SecurityException.hpp SmartPtr.hpp SoftException.cpp SoftException.hpp SystemException.cpp SystemException.hpp UserInfo.cpp UserInfo.hpp Util.cpp Util.hpp

install-exec-hook:
	chmod u+s $(DESTDIR)$(sbindir)/suphp$(EXEEXT)

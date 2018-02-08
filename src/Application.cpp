/*
  suPHP - (c)2002-2013 Sebastian Marsching <sebastian@marsching.com>
          (c)2018 John Lightsey <john@nixnuts.net>

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
  along with suPHP; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <iostream>

#include "config.h"

#include "API.hpp"
#include "API_Helper.hpp"
#include "CommandLine.hpp"
#include "Configuration.hpp"
#include "Environment.hpp"
#include "Exception.hpp"
#include "File.hpp"
#include "GroupInfo.hpp"
#include "Logger.hpp"
#include "PathMatcher.hpp"
#include "UserInfo.hpp"
#include "Util.hpp"

#include "Application.hpp"

using namespace suPHP;

suPHP::Application::Application() {
  // do nothing
}

int suPHP::Application::run(CommandLine& cmdline, const Environment& env) {
  Configuration config;
  API& api = API_Helper::getSystemAPI();
  Logger& logger = api.getSystemLogger();

#ifdef OPT_CONFIGFILE
  File cfgFile = File(OPT_CONFIGFILE);
#else
  File cfgFile = File("/etc/suphp.conf");
#endif

  std::string interpreter;
  std::string phprc_path;
  TargetMode targetMode;
  Environment newEnv;

  // Begin try block - soft exception cannot really be handled before
  // initialization
  try {
    std::string scriptFilename;
    UserInfo targetUser;
    GroupInfo targetGroup;

    // If caller is super-user, print info message and exit
    if (api.getRealProcessUser().isSuperUser()) {
      this->printAboutMessage();
      return 0;
    }
    config.readFromFile(cfgFile);

    // Check permissions (real uid, effective uid)
    this->checkProcessPermissions(config);

    // Initialize logger
    // not done before, because we need super-user privileges for
    // logging anyway
    logger.init(config);

    try {
      scriptFilename = env.getVar("SCRIPT_FILENAME");
    } catch (KeyNotFoundException& e) {
      logger.logError("Environment variable SCRIPT_FILENAME not set");
      this->printAboutMessage();
      return 1;
    }

    // Do checks that do not need target user info
    this->checkScriptFileStage1(scriptFilename, config, env);

    // Find out target user
    this->checkProcessPermissions(scriptFilename, config, env, targetUser,
                                  targetGroup);

    // Now do checks that might require user info
    this->checkScriptFileStage2(scriptFilename, config, env, targetUser,
                                targetGroup);

    // Root privileges are needed for chroot()
    // so do this before changing process permissions
    if (config.getChrootPath().length() > 0) {
      PathMatcher pathMatcher = PathMatcher(targetUser, targetGroup);
      std::string chrootPath =
          pathMatcher.resolveVariables(config.getChrootPath());
      api.chroot(chrootPath);
    }

    this->changeProcessPermissions(config, targetUser, targetGroup);

    interpreter = this->getInterpreter(env, config);

    targetMode = this->getTargetMode(interpreter);

    // Prepare environment for new process
    newEnv = this->prepareEnvironment(env, config, targetMode);

    phprc_path = this->getPHPRCPath(env, config);
    if (!phprc_path.empty()) {
      newEnv.putVar("PHPRC", phprc_path);
    }

    // Set PATH_TRANSLATED to SCRIPT_FILENAME, otherwise
    // the PHP interpreter will not be able to find the script
    if (targetMode == TARGETMODE_PHP && newEnv.hasVar("PATH_TRANSLATED")) {
      newEnv.setVar("PATH_TRANSLATED", scriptFilename);
    }

    // Log attempt to execute script
    logger.logInfo("Executing \"" + scriptFilename + "\" as UID " +
                   Util::intToStr(api.getEffectiveProcessUser().getUid()) +
                   ", GID " +
                   Util::intToStr(api.getEffectiveProcessGroup().getGid()));

    this->executeScript(scriptFilename, interpreter, targetMode, newEnv,
                        config);

    // Function should never return
    // So, if we get here, return with error code
    return 1;
  } catch (SoftException& e) {
    if (!config.getErrorsToBrowser()) {
      std::cerr << e;
      return 2;
    }
    std::cout << "Content-Type: text/html\n"
              << "Status: 500\n"
              << "\n"
              << "<html>\n"
              << " <head>\n"
              << "  <title>500 Internal Server Error</title>\n"
              << " </head>\n"
              << " <body>\n"
              << "  <h1>Internal Server Error</h1>\n"
              << "  <p>" << e.getMessage() << "</p>\n"
              << "  <hr/>"
              << "  <address>suPHP " << PACKAGE_VERSION << "</address>\n"
              << " </body>\n"
              << "</html>\n";
  }

  // Only reached on error
  return 2;
}

void suPHP::Application::printAboutMessage() {
  std::cerr << "suPHP version " << PACKAGE_VERSION << "\n";
  std::cerr << "(c) 2002-2007 Sebastian Marsching\n";
  std::cerr << std::endl;
  std::cerr << "suPHP has to be called by mod_suphp to work." << std::endl;
}

void suPHP::Application::checkProcessPermissions(Configuration& config) throw(
    SecurityException, LookupException) {
  API& api = API_Helper::getSystemAPI();
  if (api.getRealProcessUser() != api.getUserInfo(config.getWebserverUser())) {
    throw SecurityException("Calling user is not webserver user!", __FILE__,
                            __LINE__);
  }

  if (!api.getEffectiveProcessUser().isSuperUser()) {
    throw SecurityException(
        "Do not have root privileges. Executable not set-uid root?", __FILE__,
        __LINE__);
  }
}

void suPHP::Application::checkScriptFileStage1(
    const std::string& scriptFilename, const Configuration& config,
    const Environment& environment) const
    throw(SystemException, SoftException) {
  Logger& logger = API_Helper::getSystemAPI().getSystemLogger();
  File scriptFile = File(scriptFilename);
  File realScriptFile = File(scriptFile.getRealPath());

  // Check wheter file exists
  if (!scriptFile.exists()) {
    std::string error = "File " + scriptFile.getPath() + " does not exist";
    logger.logWarning(error);
    throw SoftException(error, __FILE__, __LINE__);
  }
  if (!realScriptFile.exists()) {
    std::string error = "File " + realScriptFile.getPath() +
                        " referenced by symlink " + scriptFile.getPath() +
                        " does not exist";
    logger.logWarning(error);
    throw SoftException(error, __FILE__, __LINE__);
  }

  // If enabled, check whether script is in the vhost's docroot
  if (!environment.hasVar("DOCUMENT_ROOT"))
    throw SoftException("Environment variable DOCUMENT_ROOT not set", __FILE__,
                        __LINE__);
  if (config.getCheckVHostDocroot() &&
      realScriptFile.getPath().find(environment.getVar("DOCUMENT_ROOT")) != 0) {
    std::string error = "File \"" + realScriptFile.getPath() +
                        "\" is not in document root of Vhost \"" +
                        environment.getVar("DOCUMENT_ROOT") + "\"";
    logger.logWarning(error);
    throw SoftException(error, __FILE__, __LINE__);
  }
  if (config.getCheckVHostDocroot() &&
      scriptFile.getPath().find(environment.getVar("DOCUMENT_ROOT")) != 0) {
    std::string error = "File \"" + scriptFile.getPath() +
                        "\" is not in document root of Vhost \"" +
                        environment.getVar("DOCUMENT_ROOT") + "\"";
    logger.logWarning(error);
    throw SoftException(error, __FILE__, __LINE__);
  }

  // Check script permissions
  // Directories will be checked later
  if (!realScriptFile.hasUserReadBit()) {
    std::string error =
        "File \"" + realScriptFile.getPath() + "\" not readable";
    logger.logWarning(error);
    throw SoftException(error, __FILE__, __LINE__);
  }

  if (!config.getAllowFileGroupWriteable() &&
      realScriptFile.hasGroupWriteBit()) {
    std::string error =
        "File \"" + realScriptFile.getPath() + "\" is writeable by group";
    logger.logWarning(error);
    throw SoftException(error, __FILE__, __LINE__);
  }

  if (!config.getAllowFileOthersWriteable() &&
      realScriptFile.hasOthersWriteBit()) {
    std::string error =
        "File \"" + realScriptFile.getPath() + "\" is writeable by others";
    logger.logWarning(error);
    throw SoftException(error, __FILE__, __LINE__);
  }

  // Check UID/GID of symlink is matching target
  if (scriptFile.getUser() != realScriptFile.getUser() ||
      scriptFile.getGroup() != realScriptFile.getGroup()) {
    std::string error = "UID or GID of symlink \"" + scriptFile.getPath() +
                        "\" is not matching its target";
    logger.logWarning(error);
    throw SoftException(error, __FILE__, __LINE__);
  }
}

void suPHP::Application::checkScriptFileStage2(
    const std::string& scriptFilename, const Configuration& config,
    const Environment& environment, const UserInfo& targetUser,
    const GroupInfo& targetGroup) const throw(SystemException, SoftException) {
  Logger& logger = API_Helper::getSystemAPI().getSystemLogger();
  File scriptFile = File(scriptFilename);
  PathMatcher pathMatcher = PathMatcher(targetUser, targetGroup);

  // Get full path to script file
  File realScriptFile = File(scriptFile.getRealPath());

  // Check wheter script is in one of the defined docroots
  bool file_in_docroot = false;
  const std::vector<std::string> docroots = config.getDocroots();
  for (std::vector<std::string>::const_iterator i = docroots.begin();
       i != docroots.end(); i++) {
    std::string docroot = *i;
    if (pathMatcher.matches(docroot, realScriptFile.getPath())) {
      file_in_docroot = true;
      break;
    }
  }
  if (!file_in_docroot) {
    std::string error = "Script \"" + scriptFile.getPath() +
                        "\" resolving to \"" + realScriptFile.getPath() +
                        "\" not within configured docroot";
    logger.logWarning(error);
    throw SoftException(error, __FILE__, __LINE__);
  }
  file_in_docroot = false;
  for (std::vector<std::string>::const_iterator i = docroots.begin();
       i != docroots.end(); i++) {
    std::string docroot = *i;
    if (pathMatcher.matches(docroot, scriptFile.getPath())) {
      file_in_docroot = true;
      break;
    }
  }
  if (!file_in_docroot) {
    std::string error =
        "Script \"" + scriptFile.getPath() + "\" not within configured docroot";
    logger.logWarning(error);
    throw SoftException(error, __FILE__, __LINE__);
  }

  // Check directory ownership and permissions
  checkParentDirectories(realScriptFile, targetUser, config);
  checkParentDirectories(scriptFile, targetUser, config);
}

void suPHP::Application::checkProcessPermissions(
    const std::string& scriptFilename, const Configuration& config,
    const Environment& environment, UserInfo& targetUser,
    GroupInfo& targetGroup) const
    throw(SystemException, SoftException, SecurityException) {
  File scriptFile = File(scriptFilename);
  File realScriptFile = File(scriptFile.getRealPath());
  API& api = API_Helper::getSystemAPI();
  Logger& logger = api.getSystemLogger();

// Make sure that exactly one mode is set

#if !defined(OPT_USERGROUP_OWNER) && !defined(OPT_USERGROUP_FORCE) && \
    !defined(OPT_USERGROUP_PARANOID)
#error "No uid/gid change model specified"
#endif
#if (defined(OPT_USERGROUP_OWNER) && defined(OPT_USERGROUP_FORCE)) ||    \
    (defined(OPT_USERGROUP_FORCE) && defined(OPT_USERGROUP_PARANOID)) || \
    (defined(OPT_USERGROUP_OWNER) && defined(OPT_USERGROUP_PARANOID))
#error "More than one uid/gid change model specified"
#endif

  // Common code (for all security modes)

  // Check UID/GID of script
  if (scriptFile.getUser().getUid() < config.getMinUid()) {
    std::string error =
        "UID of script \"" + scriptFilename + "\" is smaller than min_uid";
    logger.logWarning(error);
    throw SoftException(error, __FILE__, __LINE__);
  }
  if (scriptFile.getGroup().getGid() < config.getMinGid()) {
    std::string error =
        "GID of script \"" + scriptFilename + "\" is smaller than min_gid";
    logger.logWarning(error);
    throw SoftException(error, __FILE__, __LINE__);
  }

// Paranoid and force mode

#if (defined(OPT_USERGROUP_PARANOID) || defined(OPT_USERGROUP_FORCE))
  std::string targetUsername, targetGroupname;
  try {
    targetUsername = environment.getVar("SUPHP_USER");
    targetGroupname = environment.getVar("SUPHP_GROUP");
  } catch (KeyNotFoundException& e) {
    throw SecurityException(
        "Environment variable SUPHP_USER or SUPHP_GROUP not set", __FILE__,
        __LINE__);
  }

  if (config.getUserdirOverridesUsergroup() &&
      environment.hasVar("SUPHP_USERDIR_USER") &&
      environment.hasVar("SUPHP_USERDIR_GROUP")) {
    try {
      targetUsername = environment.getVar("SUPHP_USERDIR_USER");
      targetGroupname = environment.getVar("SUPHP_USERDIR_GROUP");
    } catch (KeyNotFoundException& e) {
      throw SecurityException(
          "Environment variable SUPHP_USERDIR_USER or SUPHP_USERDIR_GROUP not "
          "set",
          __FILE__, __LINE__);
    }
  }

  if (targetUsername[0] == '#' &&
      targetUsername.find_first_not_of("0123456789", 1) == std::string::npos) {
    targetUser = api.getUserInfo(Util::strToInt(targetUsername.substr(1)));
  } else {
    targetUser = api.getUserInfo(targetUsername);
  }

  if (targetGroupname[0] == '#' &&
      targetGroupname.find_first_not_of("0123456789", 1) == std::string::npos) {
    targetGroup = api.getGroupInfo(Util::strToInt(targetGroupname.substr(1)));
  } else {
    targetGroup = api.getGroupInfo(targetGroupname);
  }
#endif  // OPT_USERGROUP_PARANOID || OPT_USERGROUP_FORCE

// Owner mode only

#ifdef OPT_USERGROUP_OWNER
  targetUser = scriptFile.getUser();
  targetGroup = scriptFile.getGroup();
#endif  // OPT_USERGROUP_OWNER

// Paranoid mode only

#ifdef OPT_USERGROUP_PARANOID
  if (config.getParanoidUIDCheck() && targetUser != scriptFile.getUser()) {
    std::string error = "Mismatch between target UID (" +
                        Util::intToStr(targetUser.getUid()) + ") and UID (" +
                        Util::intToStr(scriptFile.getUser().getUid()) +
                        ") of file \"" + scriptFile.getPath() + "\"";
    logger.logWarning(error);
    throw SoftException(error, __FILE__, __LINE__);
  }

  if (config.getParanoidGIDCheck() && targetGroup != scriptFile.getGroup()) {
    std::string error = "Mismatch between target GID (" +
                        Util::intToStr(targetGroup.getGid()) + ") and GID (" +
                        Util::intToStr(scriptFile.getGroup().getGid()) +
                        ") of file \"" + scriptFile.getPath() + "\"";
    logger.logWarning(error);
    throw SoftException(error, __FILE__, __LINE__);
  }
#endif  // OPT_USERGROUP_PARANOID
}

void suPHP::Application::changeProcessPermissions(
    const Configuration& config, const UserInfo& targetUser,
    const GroupInfo& targetGroup) const
    throw(SystemException, SoftException, SecurityException) {
  API& api = API_Helper::getSystemAPI();

  // Set new group first, because we still need super-user privileges
  // for this
  api.setProcessGroup(targetGroup);

  // Then set new user
  api.setProcessUser(targetUser);

  api.setUmask(config.getUmask());
}

Environment suPHP::Application::prepareEnvironment(
    const Environment& sourceEnv, const Configuration& config,
    TargetMode mode) throw(KeyNotFoundException) {
  // Create environment for new process from old environment
  Environment env = sourceEnv;

  // Delete unwanted environment variables
  env.deleteVar("LD_PRELOAD");
  env.deleteVar("LD_LIBRARY_PATH");
  env.deleteVar("PHPRC");
  env.deleteVar("SUPHP_USER");
  env.deleteVar("SUPHP_GROUP");
  env.deleteVar("SUPHP_USERDIR_USER");
  env.deleteVar("SUPHP_USERDIR_GROUP");
  env.deleteVar("SUPHP_HANDLER");
  env.deleteVar("SUPHP_AUTH_USER");
  env.deleteVar("SUPHP_AUTH_PW");
  env.deleteVar("SUPHP_PHP_CONFIG");

  // Reset PATH
  env.putVar("PATH", config.getEnvPath());

  // If we are in PHP mode, set PHP specific variables
  if (mode == TARGETMODE_PHP) {
    if (sourceEnv.hasVar("SUPHP_PHP_CONFIG"))
      env.putVar("PHPRC", sourceEnv.getVar("SUPHP_PHP_CONFIG"));
    if (sourceEnv.hasVar("SUPHP_AUTH_USER") &&
        sourceEnv.hasVar("SUPHP_AUTH_PW")) {
      env.putVar("PHP_AUTH_USER", sourceEnv.getVar("SUPHP_AUTH_USER"));
      env.putVar("PHP_AUTH_PW", sourceEnv.getVar("SUPHP_AUTH_PW"));
    }

    // PHP may need this, when compiled with security features
    if (!env.hasVar("REDIRECT_STATUS")) {
      env.putVar("REDIRECT_STATUS", "200");
    }
  }

  return env;
}

std::string suPHP::Application::getPHPRCPath(
    const Environment& env,
    const Configuration& config) throw(SecurityException) {
  if (!env.hasVar("SUPHP_HANDLER"))
    throw SecurityException("Environment variable SUPHP_HANDLER not set",
                            __FILE__, __LINE__);
  std::string handler = env.getVar("SUPHP_HANDLER");

  std::string phprc_path = config.getPHPRCPath(handler);
  return phprc_path;
}

std::string suPHP::Application::getInterpreter(
    const Environment& env,
    const Configuration& config) throw(SecurityException) {
  if (!env.hasVar("SUPHP_HANDLER"))
    throw SecurityException("Environment variable SUPHP_HANDLER not set",
                            __FILE__, __LINE__);
  std::string handler = env.getVar("SUPHP_HANDLER");

  std::string interpreter = "";
  try {
    interpreter = config.getInterpreter(handler);
  } catch (KeyNotFoundException& e) {
    throw SecurityException("Handler not found in configuration", e, __FILE__,
                            __LINE__);
  }

  return interpreter;
}

TargetMode suPHP::Application::getTargetMode(
    const std::string& interpreter) throw(SecurityException) {
  if (interpreter.substr(0, 4) == "php:")
    return TARGETMODE_PHP;
  else if (interpreter == "execute:!self")
    return TARGETMODE_SELFEXECUTE;
  else
    throw SecurityException("Unknown Interpreter: " + interpreter, __FILE__,
                            __LINE__);
}

void suPHP::Application::executeScript(const std::string& scriptFilename,
                                       const std::string& interpreter,
                                       TargetMode mode, const Environment& env,
                                       const Configuration& config) const
    throw(SoftException) {
  try {
    // Change working directory to script path
    API_Helper::getSystemAPI().setCwd(
        File(scriptFilename).getParentDirectory().getPath());
    if (mode == TARGETMODE_PHP) {
      std::string interpreterPath = interpreter.substr(4);
      CommandLine cline;
      cline.putArgument(interpreterPath);
      if (config.getFullPHPProcessDisplay()) {
        cline.putArgument(scriptFilename);
      }
      API_Helper::getSystemAPI().execute(interpreterPath, cline, env);
    } else if (mode == TARGETMODE_SELFEXECUTE) {
      CommandLine cline;
      cline.putArgument(scriptFilename);
      API_Helper::getSystemAPI().execute(scriptFilename, cline, env);
    }
  } catch (SystemException& e) {
    throw SoftException("Could not execute script \"" + scriptFilename + "\"",
                        e, __FILE__, __LINE__);
  }
}

void suPHP::Application::checkParentDirectories(
    const File& file, const UserInfo& owner, const Configuration& config) const
    throw(SoftException) {
  File directory = file;
  Logger& logger = API_Helper::getSystemAPI().getSystemLogger();
  do {
    directory = directory.getParentDirectory();

    UserInfo directoryOwner = directory.getUser();
    if (directoryOwner != owner && !directoryOwner.isSuperUser()) {
      std::string error = "Directory " + directory.getPath() +
                          " is not owned by " + owner.getUsername();
      logger.logWarning(error);
      throw SoftException(error, __FILE__, __LINE__);
    }

    if (!directory.isSymlink() && !config.getAllowDirectoryGroupWriteable() &&
        directory.hasGroupWriteBit()) {
      std::string error =
          "Directory \"" + directory.getPath() + "\" is writeable by group";
      logger.logWarning(error);
      throw SoftException(error, __FILE__, __LINE__);
    }

    if (!directory.isSymlink() && !config.getAllowDirectoryOthersWriteable() &&
        directory.hasOthersWriteBit()) {
      std::string error =
          "Directory \"" + directory.getPath() + "\" is writeable by others";
      logger.logWarning(error);
      throw SoftException(error, __FILE__, __LINE__);
    }
  } while (directory.getPath() != "/");
}

int main(int argc, char** argv) {
  try {
    API& api = API_Helper::getSystemAPI();
    CommandLine cmdline;
    Environment env;
    Application app;
    for (int i = 0; i < argc; i++) {
      cmdline.putArgument(argv[i]);
    }
    env = api.getProcessEnvironment();
    return app.run(cmdline, env);
  } catch (Exception& e) {
    std::cerr << e;
    return 1;
  }
}

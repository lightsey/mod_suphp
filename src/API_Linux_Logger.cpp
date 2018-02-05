/*
    suPHP - (c)2002-2005 Sebastian Marsching <sebastian@marsching.com>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <iostream>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

#include "API_Linux_Logger.hpp"

using namespace suPHP;

suPHP::API_Linux_Logger::API_Linux_Logger() {
    this->ready = false;
    this->logFd = -1;
}

void suPHP::API_Linux_Logger::log(const std::string& classification, 
                                  const std::string& message) {
    ::time_t ts;
    struct ::tm *now;
    char timestr[64] = {0};
    std::string logline;
    
    ts = ::time(NULL);
    now = ::localtime(&ts);

    // Do not check for error when running strftime -
    // we couldn't handle it anyway :-)
    ::strftime(timestr, 64, "[%a %b %d %H:%M:%S %Y]", now);

    // Construct logline
    logline = std::string(timestr) + " [" + classification + "] " 
        + message + "\n";

    // Check wheter we have an uninitialized logfile 
    // or write to the logfile failed
    if (!this->isInitialized() 
        || (write(this->logFd, logline.c_str(), logline.size()) == -1)) {
        // Print message to stderr
        std::cerr << "Could not write to logfile:" << std::endl
//		  << ::strerror(::errno) << std::endl
                  << "Printing message to stderr:" << std::endl
                  << logline << std::endl;
    }
}

void suPHP::API_Linux_Logger::init(const Configuration& config)
    throw (IOException) {
    // Open logfile in append mode, create if not existing.
    this->logFd = ::open(config.getLogfile().c_str(), 
                         O_WRONLY|O_CREAT|O_APPEND|O_NOCTTY, 
                         S_IRUSR|S_IWUSR);
    
    // Check wheter something failed
    if (this->logFd == -1) {
        throw IOException("Could not open logfile " + config.getLogfile(),
                          __FILE__, __LINE__);
    }
    
    // Set close-on-exec flag, because we do not want
    // the user to write to our logfile
    if (::fcntl(this->logFd, F_SETFD, FD_CLOEXEC)) {
        // Ooops, something went wrong
        throw IOException("Could not set close-on-exec flag on logfile",
                          __FILE__, __LINE__);
    }
    
    // Get log level from configuration
    this->setLogLevel(config.getLogLevel());

    // We got here, so nothing failed
    this->ready = true;
}

bool suPHP::API_Linux_Logger::isInitialized() {
    return this->ready;
}

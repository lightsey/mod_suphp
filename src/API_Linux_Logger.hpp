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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef SUPHP_API_LINUX_LOGGER_H

namespace suPHP {
    class API_Linux_Logger;
};

#define SUPHP_API_LINUX_LOGGER_H

#include "Logger.hpp"

namespace suPHP {
    /**
     * Class containing logging facility.
     * Implementation for Linux API.
     */
    class API_Linux_Logger : public Logger {
    private:
	int logFd;
	bool ready;

	/**
	 * Internal log function - implementation
	 */
	virtual void log(const std::string& classification, 
			 const std::string& message);

    public:
	/**
	 * Constructor
	 */
	API_Linux_Logger();
	
	/**
	 * Initialize (open logfile) - implementation
	 */
	virtual void init(const Configuration& config) throw (IOException);
	/**
	 * Is Logger initialized?
	 */
	virtual bool isInitialized();
    };
};

#endif // SUPHP_API_LINUX_LOGGER_H

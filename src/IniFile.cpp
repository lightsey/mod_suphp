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

#include <string>
#include <fstream>
#include <streambuf>
#include <sstream>

#include "KeyNotFoundException.hpp"

#include "IniFile.hpp"

using namespace suPHP;

IniSection& suPHP::IniFile::getSection(std::string name)
    throw (KeyNotFoundException) {
    if (this->sections.find(name) != this->sections.end()) {
	return this->sections.find(name)->second;
    } else {
	throw KeyNotFoundException("Section " + name + " not found", 
				   __FILE__, __LINE__);
    }
}

bool suPHP::IniFile::hasSection(std::string name) {
    if (this->sections.find(name) != this->sections.end()) {
	return true;
    } else {
	return false;
    }
    
}

IniSection& suPHP::IniFile::operator[](std::string name) 
    throw (KeyNotFoundException) {
    return this->getSection(name);
}


void suPHP::IniFile::parse(File file) throw (IOException, ParsingException) {
    SmartPtr<std::ifstream> is = file.getInputStream();
    IniSection *current_section = NULL;
    while (!is->eof() && !is->bad() && !is->fail()) {
	std::string line;
	std::string tstr;
	char dummy;
	int startpos = 0;
	int endpos = 0;
	
	// Read line from file
	getline(*is, line);
	
	tstr = line;
	
	// Find first char not being space or tab
	startpos = tstr.find_first_not_of(" \t");
	// Find last char not being space or tab
	endpos = tstr.find_last_not_of(" \t");

	// Skip empty line, only containing whitespace
	if (startpos == std::string::npos)
	    continue;
	
	// Get trimmed string
	tstr = tstr.substr(startpos, endpos - startpos + 1);

	// Is line a comment (starting with ";")?
	if (tstr[0] == ';') {
	    // Comments are not interessting => skip
	    continue;
       
        // Is line a section mark ("[section]")?
	} else if (tstr[0] == '[' && tstr[tstr.size()-1] == ']') {
	    // Extract name of section
	    std::string name = tstr.substr(1, tstr.size() - 2);
	    // If section is not yet existing, create it
	    if (!this->hasSection(name)) {
		std::pair<std::string, IniSection> p;
		IniSection sect;
		p.first = name;
		p.second = sect;
		this->sections.insert(p);
	    }
	    // Set current section
	    current_section = &(this->getSection(name));
	    
	// Is the line a key-value pair?
	} else if (tstr.find_first_of('=') != std::string::npos) {
	    std::string name;
	    std::string value;
	    int eqpos = 0;
	    
	    // Check wheter we already have a section
	    if (current_section == NULL) {
		throw ParsingException("Option line \"" + tstr +
				       "\" before first section", 
				       __FILE__, __LINE__);
	    }
	    
	    // Extract name
	    eqpos = tstr.find_first_of('=');
	    name = tstr.substr(0, eqpos);
	    value = tstr.substr(eqpos + 1, tstr.size() - eqpos + 1);
	    name = name.substr(0, name.find_last_not_of(" \t") + 1);
	    value = value.substr(value.find_first_not_of(" \t"));
	    if (value[0] == '"')
		value = value.substr(1);
	    if (value[value.size()-1] == '"')
		value = value.substr(0, value.size()-1);
	    current_section->putValue(name, value);
	// Line is something we do not know
	} else {
	    throw ParsingException("Illegal line \"" + tstr + "\"", 
				   __FILE__, __LINE__);
	}
    }
    is->close();
}

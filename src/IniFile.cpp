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
    along with suPHP; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include <string>
#include <fstream>
#include <sstream>
#include <vector>

#include "KeyNotFoundException.hpp"

#include "IniFile.hpp"

using namespace suPHP;

const IniSection& suPHP::IniFile::getSection(const std::string& name) const
    throw (KeyNotFoundException) {
    if (this->sections.find(name) != this->sections.end()) {
        return this->sections.find(name)->second;
    } else {
        throw KeyNotFoundException("Section " + name + " not found", 
                                   __FILE__, __LINE__);
    }
}

bool suPHP::IniFile::hasSection(const std::string& name) const {
    if (this->sections.find(name) != this->sections.end()) {
        return true;
    } else {
        return false;
    }
}

const IniSection& suPHP::IniFile::operator[](const std::string& name) const
    throw (KeyNotFoundException) {
    return this->getSection(name);
}


void suPHP::IniFile::parse(const File& file) throw (IOException, ParsingException) {
    SmartPtr<std::ifstream> is = file.getInputStream();
    IniSection *current_section = NULL;
    while (!is->eof() && !is->bad() && !is->fail()) {
        std::string line;
        std::string tstr;
        std::string::size_type startpos = 0;
        std::string::size_type endpos = 0;

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
            current_section = &(this->sections.find(name)->second);

        // Is the line a key-value pair?
        } else if (tstr.find_first_of('=') != std::string::npos) {
            std::string name;
            std::vector<std::string> values;
            bool append_mode = false;

            std::string::size_type eqpos = 0;

            // Check wheter we already have a section
            if (current_section == NULL) {
                throw ParsingException("Option line \"" + tstr +
                                       "\" before first section", 
                                       __FILE__, __LINE__);
            }

            // Extract name
            eqpos = tstr.find_first_of('=');
            if (eqpos == std::string::npos || eqpos < 1 || eqpos == tstr.length()-1) {
                throw ParsingException("Malformed line: " + tstr, __FILE__, __LINE__);
            }
            if (tstr[eqpos-1] == '+') {
                append_mode = true;
                name = tstr.substr(0, eqpos-1);
            } else {
                name = tstr.substr(0, eqpos);
            }

            std::string::size_type temppos;
            temppos = name.find_first_not_of(" \t");
            if (temppos == std::string::npos) {
                throw ParsingException("Malformed line: " + tstr, __FILE__, __LINE__);
            }
            name = name.substr(0, name.find_last_not_of(" \t") + 1);

            bool in_quotes = false;
            bool last_was_backslash = false;
            std::string::size_type token_start = eqpos + 1;

            for (std::string::size_type i=eqpos+1; i<tstr.length(); i++) {
                bool current_is_backslash = false;
                if (tstr[i] == '"') {
                    if (!last_was_backslash) {
                        in_quotes = !in_quotes;
                    }
                } else if (tstr[i] == '\\') {
                    if (!last_was_backslash) {
                        current_is_backslash = true;
                    }
                } else if (tstr[i] == ':') {
                    if (!in_quotes && !last_was_backslash) {
                        // Save token and begin new one
                        std::string token = tstr.substr(token_start, i - token_start);
                        try {
                            token = parseValue(token);
                        } catch (ParsingException e) {
                            throw ParsingException("Malformed line: " + tstr, __FILE__, __LINE__);
                        }
                        if (token.length() == 0) {
                            throw ParsingException("Malformed line: " + tstr, __FILE__, __LINE__);
                        }
                        values.push_back(token);
                        token_start = i + 1;
                    }
                }
                if (i == tstr.length() - 1) {
                    if (in_quotes) {
                        throw ParsingException("Unended quotes in line: " + tstr, __FILE__, __LINE__);
                    } else if (last_was_backslash) {
                        throw ParsingException("Multiline values are not supported in line: " + tstr, __FILE__, __LINE__);
                    }
                    std::string token = tstr.substr(token_start, i + 1 - token_start);
                    try {
                        token = parseValue(token);
                    } catch (ParsingException e) {
                        throw ParsingException("Malformed line: " + tstr, __FILE__, __LINE__);
                    }
                    if (token.length() == 0) {
                        throw ParsingException("Malformed line: " + tstr, __FILE__, __LINE__);
                    }
                    values.push_back(token);
                }
                last_was_backslash = current_is_backslash;
            }

            if (!append_mode) {
                current_section->removeValues(name);
            }
            for (std::vector<std::string>::iterator i = values.begin(); i != values.end(); i++) {
                current_section->putValue(name, *i);
            }

        // Line is something we do not know
        } else {
            throw ParsingException("Malformed line \"" + tstr + "\"", 
                                   __FILE__, __LINE__);
        }
    }
    is->close();
}

std::string suPHP::IniFile::parseValue(const std::string& value) const throw (ParsingException) {
    bool in_quotes = false;
    bool last_was_backslash = false;
    std::string tempvalue;
    std::string output;

    std::string::size_type startpos = value.find_first_not_of(" \t");
    std::string::size_type endpos = value.find_last_not_of(" \t");
    if (startpos == std::string::npos) {
        return "";
    }
    if (endpos == std::string::npos) {
        tempvalue = value;
    } else {
        tempvalue = value.substr(startpos, endpos - startpos + 1);
    }

    for (std::string::size_type i=0; i<value.length(); i++) {
        bool current_is_backslash = false;

        if (tempvalue[i] == '"') {
            if (last_was_backslash) {
                output.append("\"");
            } else {
                if (!in_quotes && i != 0) {
                    throw ParsingException("Content preceding quoted content", __FILE__, __LINE__);
                }
                if (in_quotes && i != tempvalue.length() - 1) {
                    throw ParsingException("Content following quoted content", __FILE__, __LINE__);
                }
                in_quotes = !in_quotes;
            }
        } else if (tempvalue[i] == '\\') {
            if (last_was_backslash) {
                output.append("\\");
            } else {
                current_is_backslash = true;
            }
        } else if (tempvalue[i] == ':') {
            output.append(":");
        } else {
            if (last_was_backslash) {
                throw ParsingException("Illegal character after backslash", __FILE__, __LINE__);
            }
            output.append(tempvalue.substr(i, 1));
        }

        last_was_backslash = current_is_backslash;
    }

    return output;
}

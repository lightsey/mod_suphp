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

#include "API_Helper.hpp"

#include "File.hpp"

using namespace suPHP;
using namespace std;

bool suPHP::File::hasPermissionBit(FileMode perm) const {
  return API_Helper::getSystemAPI().File_hasPermissionBit(*this, perm);
}

suPHP::File::File(string path) { this->path = path; }

string suPHP::File::getPath() const { return this->path; }

unique_ptr<ifstream> suPHP::File::getInputStream() const {
  unique_ptr<ifstream> infile(new ifstream(this->path.c_str()));
  if (infile->fail()) {
    throw IOException("Could not open file " + this->path + " for reading",
                      __FILE__, __LINE__);
  }
  return infile;
}

bool suPHP::File::exists() const {
  return API_Helper::getSystemAPI().File_exists(*this);
}

string suPHP::File::getRealPath() const {
  return API_Helper::getSystemAPI().File_getRealPath(*this);
}

File suPHP::File::getParentDirectory() const {
  string path = this->getPath();
  path = path.substr(0, path.rfind('/'));
  if (path.length() == 0) {
    path = "/";
  }
  return File(path);
}

bool suPHP::File::hasUserReadBit() const {
  return this->hasPermissionBit(FILEMODE_USER_READ);
}

bool suPHP::File::hasUserWriteBit() const {
  return this->hasPermissionBit(FILEMODE_USER_WRITE);
}

bool suPHP::File::hasUserExecuteBit() const {
  return this->hasPermissionBit(FILEMODE_USER_EXEC);
}

bool suPHP::File::hasGroupReadBit() const {
  return this->hasPermissionBit(FILEMODE_GROUP_READ);
}

bool suPHP::File::hasGroupWriteBit() const {
  return this->hasPermissionBit(FILEMODE_GROUP_WRITE);
}

bool suPHP::File::hasGroupExecuteBit() const {
  return this->hasPermissionBit(FILEMODE_GROUP_EXEC);
}

bool suPHP::File::hasOthersReadBit() const {
  return this->hasPermissionBit(FILEMODE_OTHERS_READ);
}

bool suPHP::File::hasOthersWriteBit() const {
  return this->hasPermissionBit(FILEMODE_OTHERS_WRITE);
}

bool suPHP::File::hasOthersExecuteBit() const {
  return this->hasPermissionBit(FILEMODE_OTHERS_EXEC);
}

UserInfo suPHP::File::getUser() const {
  return API_Helper::getSystemAPI().File_getUser(*this);
}

GroupInfo suPHP::File::getGroup() const {
  return API_Helper::getSystemAPI().File_getGroup(*this);
}

bool suPHP::File::isSymlink() const {
  return API_Helper::getSystemAPI().File_isSymlink(*this);
}

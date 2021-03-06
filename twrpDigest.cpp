/*
	Copyright 2012 bigbiff/Dees_Troy TeamWin
	This file is part of TWRP/TeamWin Recovery Project.

	TWRP is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	TWRP is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with TWRP.  If not, see <http://www.gnu.org/licenses/>.
*/

extern "C"
{
	#include "digest/md5.h"
	#include "gui/gui.h"
	#include "libcrecovery/common.h"
}

#include <vector>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <sys/mman.h>
#include "twcommon.h"
#include "data.hpp"
#include "variables.h"
#include "twrp-functions.hpp"
#include "twrpDigest.hpp"

using namespace std;

void twrpDigest::setfn(string fn) {
	md5fn = fn;
}

int twrpDigest::computeMD5(void) {
	string line;
	struct MD5Context md5c;
	FILE *file;
	int len;
	unsigned char buf[1024];
	MD5Init(&md5c);
	file = fopen(md5fn.c_str(), "rb");
	if (file == NULL)
		return -1;
	while ((len = fread(buf, 1, sizeof(buf), file)) > 0) {
		MD5Update(&md5c, buf, len);
	}
	fclose(file);
	MD5Final(md5sum, &md5c);
	return 0;
}

int twrpDigest::write_md5digest(void) {
	int i;
	string md5string, md5file;
	char hex[3];
	md5file = md5fn + ".md5";

	for (i = 0; i < 16; ++i) {
		snprintf(hex, 3, "%02x", md5sum[i]);
		md5string += hex;
	}
	md5string += "  ";
	md5string += basename((char*) md5fn.c_str());
	md5string +=  + "\n";
	TWFunc::write_file(md5file, md5string);
	LOGINFO("MD5 for %s: %s\n", md5fn.c_str(), md5string.c_str());
	return 0;
}

int twrpDigest::read_md5digest(void) {
	int i = 0;
	bool foundMd5File = false;
	string md5file = "";
	vector<string> md5ext;
	md5ext.push_back(".md5");
	md5ext.push_back(".md5sum");

	while (i < md5ext.size()) {
		md5file = md5fn + md5ext[i];
		if (TWFunc::Path_Exists(md5file)) {
			foundMd5File = true;
			break;
		}
		i++;
	}

	if (!foundMd5File) {
		gui_print("Skipping MD5 check: no MD5 file found\n");
		return -1;
	} else if (TWFunc::read_file(md5file, line) != 0) {
		gui_print("Skipping MD5 check: MD5 file unreadable\n");
		return 1;
	}

	return 0;
}

/* verify_md5digest return codes:
	-2: md5 did not match
	-1: no md5 file found
	 0: md5 matches
	 1: md5 file unreadable
*/

int twrpDigest::verify_md5digest(void) {
	string buf;
	char hex[3];
	int i, ret;
	string md5string;

	ret = read_md5digest();
	if (ret != 0)
		return ret;
	stringstream ss(line);
	vector<string> tokens;
	while (ss >> buf)
		tokens.push_back(buf);
	computeMD5();
	for (i = 0; i < 16; ++i) {
		snprintf(hex, 3, "%02x", md5sum[i]);
		md5string += hex;
	}
	if (tokens.at(0) != md5string) {
		LOGERR("MD5 does not match\n");
		return -2;
	}

	gui_print("MD5 matched\n");
	return 0;
}

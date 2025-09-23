/* 
 * Copyright (C) 2025 Zoltán Rácz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.  
 */

#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <openssl/sha.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "sha1.h"

int sha1_to_hex(unsigned char *sha1, char* out_hex)
{
	if (!out_hex)
		return -1;

	for (int i = 0; i < SHA_DIGEST_LENGTH; i++) 
        sprintf(out_hex + i*2, "%02x", sha1[i]);

	return 0;
}

int write_sha1_file(unsigned char *sha1, char *buffer, int len)
{
	int ret = 0;
	int written = 0;
	char path[PATH_MAX];
	char sha1_hex[40+1];

	sha1_to_hex(sha1, sha1_hex);

	sprintf(path, "./.bkp-data/%s", sha1_hex);

	int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
	if (fd < 0) 
		return errno == EEXIST ? 0 : fd;

	written = write(fd, buffer, len);

	if (written != len) {
		fprintf(stderr, "Error writing to file %s!\n", path);
		ret = -1;
		goto ret;
	}

ret:
	if (fd)
		close(fd);

	return ret;
}



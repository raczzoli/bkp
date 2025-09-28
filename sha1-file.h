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

#ifndef SHA1_FILE_H
#define SHA1_FILE_H

#include <openssl/sha.h>

struct write_job_data {
	unsigned char sha1[SHA_DIGEST_LENGTH];
	char *buff;
	int len;
};

int sha1_to_hex(unsigned char *sha1, char* out_hex);
int hex_to_sha1(char *hex, unsigned char *out_sha1);

int write_sha1_file(unsigned char *sha1, char *buffer, int len);
int write_sha1_file_async(unsigned char *sha1, char *buffer, int len);
int read_sha1_file(unsigned char *sha1, char **out_buff, int *out_size);

int sha1_is_valid(unsigned char *sha1);

#endif

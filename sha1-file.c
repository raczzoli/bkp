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
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <openssl/sha.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>

#include "sha1-file.h"
#include "worker.h"

static int hexchar_to_int(char c);
static int write_async_cb(void *data);

int sha1_to_hex(unsigned char *sha1, char* out_hex)
{
	if (!out_hex)
		return -1;

	for (int i = 0; i < SHA_DIGEST_LENGTH; i++) 
        sprintf(out_hex + i*2, "%02x", sha1[i]);

	return 0;
}

static int hexchar_to_int(char c) 
{
    if ('0' <= c && c <= '9') return c - '0';
    if ('a' <= c && c <= 'f') return c - 'a' + 10;
    if ('A' <= c && c <= 'F') return c - 'A' + 10;
    return -1;
}

int hex_to_sha1(char *hex, unsigned char *out_sha1) 
{
    int len = 40;

    for (int i = 0; i < len / 2; i++) {
        int hi = hexchar_to_int(hex[2 * i]);
        int lo = hexchar_to_int(hex[2 * i + 1]);

        if (hi < 0 || lo < 0) 
			return -1;

        out_sha1[i] = (hi << 4) | lo;
    }

    return 0;
}

int sha1_is_valid(unsigned char *sha1)
{
	for (int i=0;i<SHA_DIGEST_LENGTH;i++) 
		if (sha1[i] != 0)
			return 1;

	return 0;
}

int write_sha1_file_async(unsigned char *sha1, char *buffer, int len)
{
	int ret = 0;
	struct write_job_data *data = malloc(sizeof(struct write_job_data));

	if (!data) {
		fprintf(stderr, "Error allocating memory for write job data structure!\n");
		return -ENOMEM;
	}

	memcpy(data->sha1, sha1, SHA_DIGEST_LENGTH);
	data->buff = buffer;
	data->len = len;

	ret = reg_worker_job(data, write_async_cb);
	if (ret) {
		free(data);
		return -1;
	}

	return 0;		
}

int write_sha1_file(unsigned char *sha1, char *buffer, int len)
{
	int ret = 0;
	int written = 0;
	char path[PATH_MAX];
	char sha1_hex[40+1];
	char *compr_buff = NULL;
	uLongf compr_len = 0;

	sha1_to_hex(sha1, sha1_hex);

	sprintf(path, ".bkp-data/%s", sha1_hex);

	int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
	if (fd < 0) 
		return errno == EEXIST ? 0 : fd;

	if (len > 0) {
		compr_len = compressBound(len);
		compr_buff = malloc(compr_len);

		if (!compr_buff) {
			ret = -1;
			goto ret;
		}

		if (compress((Bytef *)compr_buff, &compr_len, (const Bytef *)buffer, len) != Z_OK) {
			fprintf(stderr, "Compression of blob file failed!\n");
			ret = -1;
			goto ret;
		}

		written = write(fd, compr_buff, compr_len);

		if (written != (int)compr_len) {
			fprintf(stderr, "Error writing to file %s!\n", path);
			ret = -1;
			goto ret;
		}
	}

ret:
	if (fd)
		close(fd);

	if (compr_buff)
		free(compr_buff);

	return ret;
}

static int write_async_cb(void *data)
{
	struct write_job_data *jb_data = (struct write_job_data *) data;
	int ret = write_sha1_file(jb_data->sha1, jb_data->buff, jb_data->len);
		
	free(jb_data->buff);
	free(jb_data);

	return ret;
}

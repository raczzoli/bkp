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

#include <asm-generic/errno-base.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <openssl/sha.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <zlib.h>

#include "sha1-file.h"

static int hexchar_to_int(char c);
static int inflate_sha1_file(char *in_buff, size_t in_size, char **out_buff, int *out_size);

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

int write_sha1_file(unsigned char *sha1, char *buffer, int len)
{
	int ret = 0;
	int fd = -1;
	int written = 0;
	char path[PATH_MAX];
	char sha1_hex[40+1];
	char *compr_buff = NULL;
	uLongf compr_len = 0;

	compr_len = compressBound(len);
	compr_buff = malloc(compr_len);

	if (!compr_buff) {
		ret = -1;
		goto ret;
	}

	if (compress((Bytef *)compr_buff, &compr_len, (const Bytef *)buffer, len) != Z_OK) {
		fprintf(stderr, "Compression of SHA1 file content failed!\n");
		ret = -1;
		goto ret;
	}

	SHA1((const unsigned char *)compr_buff, compr_len, sha1);	

	sha1_to_hex(sha1, sha1_hex);
	sprintf(path, ".bkp-data/%s", sha1_hex);

	fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0666);
	if (fd < 0) 
		return errno == EEXIST ? 0 : fd;

	written = write(fd, compr_buff, compr_len);

	if (written != (int)compr_len) {
		fprintf(stderr, "Error writing SHA1 file!\n");
		ret = -1;
		goto ret;
	}

ret:
	if (fd >= 0)
		close(fd);

	if (compr_buff)
		free(compr_buff);

	return ret;
}

int read_sha1_file(unsigned char *sha1, char *type, char **out_buff, int *out_size)
{
	int ret = 0;
	int bytes = 0;
	struct stat stat;
	char sha1_hex[40+1];
	//char sha1_check_hex[40+1];
	char path[PATH_MAX];
	char *buff = NULL;
	int buff_len = 0;
	char *uncompr_buff = NULL;
	int uncompr_len = 0;
	char hdr_len = strlen(type)+1; // to include \0

	sha1_to_hex(sha1, sha1_hex);
	sprintf(path, ".bkp-data/%s", sha1_hex);

	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Unable to open SHA1 file: %s!\n", sha1_hex);
		return -1;
	}

	if (fstat(fd, &stat)) {
		fprintf(stderr, "Cannot stat SHA1 file: %s!\n", sha1_hex);
		ret = -1;
		goto end;
	}

	buff_len = stat.st_size;
	buff = malloc(buff_len);
	if (!buff) {
		ret = -ENOMEM;
		fprintf(stderr, "Error allocating memory for SHA1 file content: %s\n", sha1_hex);
		goto end;
	}

	bytes = read(fd, buff, buff_len);

	/*
	 * TODO: this condition will need to be replaced with a 
	 * while(bytes = read()...) just like we did in other places
	 */
	if (bytes != buff_len) { 
		ret = -1;
		fprintf(stderr, "Error reading from SHA1 file: %s!\n", sha1_hex);
		goto end;
	}
	ret = inflate_sha1_file(buff, bytes, &uncompr_buff, &uncompr_len);

	if (ret != 0) {
		fprintf(stderr, "Error uncompressing sha1 file %s!\n", sha1_hex);
		goto end;
	}

	/*
	 * Checking if the content still has the same SHA1 hash
	 */

	/* TODO - check suspended for the moment
	unsigned char sha1_check[SHA_DIGEST_LENGTH];
	SHA1((const unsigned char *)uncompr_buff, uncompr_len, sha1_check);

	if (memcmp(sha1_check, sha1, SHA_DIGEST_LENGTH) != 0) {
		ret = -1;

		sha1_to_hex(sha1_check, sha1_check_hex);
		fprintf(stderr, "SHA1 file corrupted! Expected \"%s\" but computed \"%s\"!\n", sha1_hex, sha1_check_hex);
		goto end;
	}
	*/

	/*
	 * Check if sha1 content header matches the requested type
	 */
	if (memcmp(uncompr_buff, type, hdr_len-1) != 0) {
		ret = -1;
		fprintf(stderr, "Requested type \"%s\" not matched in SHA1 file %s!\n", type, sha1_hex);
		goto end;
	}

	*out_size = uncompr_len - hdr_len;
	*out_buff = malloc(*out_size);

	if (!*out_buff) {
		ret = -ENOMEM;
		fprintf(stderr, "Error allocating memory for output buffer for SHA1 file %s!\n", sha1_hex);
		goto end;
	}

	memcpy(*out_buff, uncompr_buff + hdr_len, *out_size);


end:
	if (buff)
		free(buff);

	if (uncompr_buff)
		free(uncompr_buff);

	close(fd);
	return ret;
}

static int inflate_sha1_file(char *in_buff, size_t in_size, char **out_buff, int *out_size)
{
	int ret = 0;
	unsigned char *buff = NULL;
	int chunk_size = 1024 * 1024; // 1MB chunks
	int offset = 0;

	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree  = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = in_size;
	strm.next_in  = (Bytef *)in_buff;

	ret = inflateInit(&strm);
	if (ret != Z_OK) {
		fprintf(stderr, "Error inflating SHA1 file!\n");
		return -1;
	}
		
	buff = malloc(chunk_size);
	if (!buff) {
		ret = -ENOMEM;
		goto end;
	}

	do {
		if (!buff) 
			buff = malloc(chunk_size);
		else if (offset % chunk_size == 0)
			buff = realloc(buff, offset + chunk_size);

		if (!buff) {
			ret = -ENOMEM;
			fprintf(stderr, "Error allocating memory for zstream chunk!\n");
			goto end;
		}

		strm.avail_out = chunk_size;
		strm.next_out = buff + offset;
		
		if ((ret = inflate(&strm, Z_NO_FLUSH)) < 0) {
			fprintf(stderr, "SHA1 file inflate returned code %d!\n", ret);
			ret = -1;
			goto end;
		}

		offset += chunk_size - strm.avail_out;
	} while (ret != Z_STREAM_END);

	*out_buff = (char *) buff;
	*out_size = offset;
	ret = 0;

end:
	inflateEnd(&strm);
	return ret;
}


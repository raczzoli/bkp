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
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <openssl/sha.h>

#include "file.h"
#include "sha1-file.h"

static int write_blob(char *buffer, int size, unsigned char *sha1);


int write_file(char *path, off_t size, unsigned char *sha1)
{
	int ret = 0;
	int bytes_read = 0;
	int fd = open(path, O_RDONLY);
	char *buff = NULL;
	char *chunks_buff = NULL;
	unsigned char chunk_sha1[SHA_DIGEST_LENGTH];
	int chunks_offset = 0;
	int num_chunks = (size / FILE_CHUNK_SIZE) + (size % FILE_CHUNK_SIZE == 0 ? 0 : 1);
	int chunks_buff_size = num_chunks * SHA_DIGEST_LENGTH; 

	if (fd < 0) {
		fprintf(stderr, "Error opening file %s for backup (errno: %d)\n", path, errno);
		return -1;
	}

	buff = malloc(FILE_CHUNK_SIZE);
	if (!buff) {
		fprintf(stderr, "Error allocating memory for read buffer while backing up file!\n");
		ret = -ENOMEM;
		goto end;
	}

	chunks_buff = malloc(100 + chunks_buff_size);
	if (!chunks_buff) {
		fprintf(stderr, "Error allocating memory for sha1 chunks buffer!\n");
		ret = -ENOMEM;
		goto end;
	}

	memset(chunks_buff, 0, 100 + chunks_buff_size);
	chunks_offset = sprintf(chunks_buff, "chunks") + 1; // \0 too

	while((bytes_read = read(fd, buff, FILE_CHUNK_SIZE)) > 0)
	{
		ret = write_blob(buff, bytes_read, chunk_sha1);
		if (ret) 
			goto end;

		memcpy(chunks_buff+chunks_offset, chunk_sha1, SHA_DIGEST_LENGTH);
		chunks_offset += SHA_DIGEST_LENGTH;
	}

	SHA1((const unsigned char *)chunks_buff, chunks_offset, sha1);
	ret = write_sha1_file(sha1, chunks_buff, chunks_offset);

end:
	if (buff)
		free(buff);

	if (chunks_buff)
		free(chunks_buff);

	close(fd);
	return ret;
}

static int write_blob(char *buffer, int size, unsigned char *sha1)
{
	int ret = 0;
	int offset = 0;
	char *buff = malloc(100 + size);

	if (!buff)
		return -ENOMEM;

	offset = sprintf(buff, "blob");
	offset += 1; // we want to keep the \0 too
	memcpy(buff+offset, buffer, size);
	offset += size;

	SHA1((const unsigned char *)buff, offset, sha1);

	ret = write_sha1_file(sha1, buff, offset);
	if (ret)
		goto end;

end:
	free(buff);

	return 0;
}

int read_blob(unsigned char *sha1, char **out_buff, size_t *out_size)
{
	int fd = 0;
	int ret = 0;
	int offset = 0;
	int bytes = 0;
	struct stat stat;
	char path[PATH_MAX];
	char sha1_hex[40+1];
	int hdr_len = 5; // "blob"+\0
	char hdr[hdr_len];

	sha1_to_hex(sha1, sha1_hex);
	sprintf(path, ".bkp-data/%s", sha1_hex); 

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Cannot open blob file: %s - %s!\n", sha1_hex, strerror(errno));
		return -1;
	}

	if (fstat(fd, &stat)) {
		fprintf(stderr, "Cannot stat blob file: %s!\n", sha1_hex);
		ret = -1;
		goto end;
	}

	if (((bytes = read(fd, hdr, hdr_len)) != hdr_len) || 
		memcmp(hdr, "blob\0", hdr_len) != 0) {
		ret = -1;
		fprintf(stderr, "Invalid or corrupted blob file!\n");
		goto end;
	}

	*out_size = stat.st_size - hdr_len;
	*out_buff = malloc(*out_size);

	while ( (bytes = read(fd, (*out_buff)+offset, *out_size)) > 0 ) 
		offset += bytes;	
	
	if (bytes < 0) {
		free(*out_buff);
		ret = -1;
		goto end;
	}

end:
	close(fd);
	return ret;
}

int read_chunks_file(unsigned char *sha1, unsigned char **out_buff, int *num_chunks)
{
	int ret = 0;
	int fd = 0;
	struct stat stat;
	char path[PATH_MAX];
	char sha1_hex[40+1];
	int hdr_len = 7; // "chunks"+\0
	char hdr[hdr_len];
	int chunks_size = 0;
	int bytes;

	sha1_to_hex(sha1, sha1_hex);
	sprintf(path, ".bkp-data/%s", sha1_hex);

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Cannot open chunks file: %s!\n", sha1_hex);
		return -1;
	}

	if (fstat(fd, &stat)) {
		fprintf(stderr, "Cannot stat chunks file: %s!\n", sha1_hex);
		ret = -1;
		goto end;
	}

	if (((bytes = read(fd, hdr, hdr_len)) != hdr_len) || 
		memcmp(hdr, "chunks\0", hdr_len) != 0) {
		ret = -1;
		fprintf(stderr, "Invalid or corrupted chunks file!\n");
		goto end;
	}

	chunks_size = stat.st_size - hdr_len; 

	if (chunks_size % SHA_DIGEST_LENGTH != 0) {
		fprintf(stderr, "Invalid or corrupted chunks file! The size of the chunks should be a multiple of %d bytes.\n", SHA_DIGEST_LENGTH);
		ret = -1;
		goto end;
	}

	*num_chunks = chunks_size / SHA_DIGEST_LENGTH;
	*out_buff = malloc(chunks_size);

	if (!*out_buff) {
		ret = -ENOMEM;
		fprintf(stderr, "Error allocating memory for chunks buffer!\n");
		goto end;
	}

	int offset = 0;
	while ( (bytes = read(fd, (*out_buff)+offset, chunks_size)) > 0 ) 
		offset += bytes;	
	
	if (bytes < 0) {
		free(*out_buff);
		ret = -1;
		goto end;
	}


end:
	close(fd);
	return ret;
}

int print_chunks_file(unsigned char *sha1)
{
	unsigned char *buff = NULL, *tmp_buff = NULL;
	int num_chunks = 0;
	char sha1_hex[40+1];

	if (read_chunks_file(sha1, &buff, &num_chunks)) 
		return -1;
	
	tmp_buff = buff;
	while(num_chunks > 0) {
		sha1_to_hex(tmp_buff, sha1_hex);
		printf("%s\n", sha1_hex);

		tmp_buff += SHA_DIGEST_LENGTH;
		num_chunks--;
	}
	
	free(buff);
	return 0;
}


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
#include <zconf.h>
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

		if (ret) {
			free(chunks_buff);
			chunks_buff = NULL;
			goto end;
		}

		memcpy(chunks_buff+chunks_offset, chunk_sha1, SHA_DIGEST_LENGTH);
		chunks_offset += SHA_DIGEST_LENGTH;
	}

	SHA1((const unsigned char *)chunks_buff, chunks_offset, sha1);
	ret = write_sha1_file_async(sha1, chunks_buff, chunks_offset);

end:
	if (buff)
		free(buff);

	close(fd);
	return ret;
}

static int write_blob(char *buffer, int size, unsigned char *sha1)
{
	int offset = 0;
	int buff_len = size + 5; // 5 = "blob\0"
	char *buff = malloc(buff_len);

	if (!buff)
		return -ENOMEM;

	offset = sprintf(buff, "blob");
	offset += 1; // we want to keep the \0 too
	memcpy(buff+offset, buffer, size);
	offset += size;

	SHA1((const unsigned char *)buff, buff_len, sha1);

	return write_sha1_file_async(sha1, buff, buff_len);
}

int read_blob(unsigned char *sha1, char **out_buff, int *out_size)
{
	return read_sha1_file(sha1, "blob", out_buff, out_size);
}

int read_chunks_file(unsigned char *sha1, unsigned char **out_buff, int *num_chunks)
{
	int ret = 0;
	unsigned char *buff = NULL;
	int buff_len = 0;

	ret = read_sha1_file(sha1, "chunks", (char **)&buff, &buff_len);
	if (ret)
		return ret;

	if (buff_len % SHA_DIGEST_LENGTH != 0) {
		fprintf(stderr, "Invalid or corrupted chunks file! The size of the chunks should be a multiple of %d bytes.\n", SHA_DIGEST_LENGTH);
		ret = -1;
		free(buff);
		goto end;
	}

	*out_buff = buff;
	*num_chunks = buff_len / SHA_DIGEST_LENGTH;

end:
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


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <openssl/sha.h>

#include "file.h"
#include "sha1.h"

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
	if (ret)
		goto end;

	char plm_buff[40+1];
	sha1_to_hex(sha1, plm_buff);
	printf("Writtent chunk file: %s\n", plm_buff);
	// TODO - write the sha1 chunks to the file content sha1 file
	
end:
	if (buff)
		free(buff);

	if (chunks_buff)
		free(chunks_buff);

	close(fd);
	return 0;
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

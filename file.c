#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

#include "file.h"

int write_file(char *path, off_t size, unsigned char *sha1)
{
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Error opening file %s for backup\n", path);
		return -1;
	}

	printf("file %s of size %ld\n", path, size);

	return 0;
}


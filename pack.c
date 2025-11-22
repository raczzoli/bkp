#include <asm-generic/errno-base.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "pack.h"
#include "sha1-file.h"

#define PACK_DIR ".bkp-data/packs/"

static int fd = -1;
size_t fd_offset = 0;

int pack_object(unsigned char *sha1, const char *obj, size_t size)
{
	int ret = 0;
	char path[PATH_MAX];
	size_t written = 0;
	char sha1_hex[40];

	sha1_to_hex(sha1, sha1_hex);
	sprintf(path, PACK_DIR"%s", "202511221809.pack");

	if (fd == -1) {
		// create PACK_DIR if doesn`t exist
		DIR *dir = opendir(PACK_DIR);
		if (dir) 
			closedir(dir);
		else 
			mkdir(PACK_DIR, 0755);

		// open packfile
		fd = open(path, O_CREAT | O_WRONLY, 0664);
		if (fd < 0) {
			fprintf(stderr, "Error opening pack file %s!\n", path);
			ret = -1;
			goto end;
		}
	}

	written = write(fd, obj, size);
	if (written != size) {
		fprintf(stderr, "Error writing to packfile %s!\n", path);
		ret = -1;
		goto end;
	}

end:
	return ret;
}

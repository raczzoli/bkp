#include <asm-generic/errno-base.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include "pack.h"
#include "sha1-file.h"


static int fd = -1;
size_t fd_offset = 0;
char pack_fname[PACK_FNAME_LEN+1];

static void add_pack_entry(struct pack_idx_entry *entry);
static void get_pack_fname();
static int create_packfile();

int pack_object(unsigned char *sha1, const char *obj, size_t size)
{
	int ret = 0;
	size_t written = 0;
	struct pack_idx_entry *entry = malloc(sizeof(struct pack_idx_entry));

	if (!entry) {
		fprintf(stderr, "Error allocating memory for pack index entry!\n");
		return -ENOMEM;
	}

	if (fd == -1) 
		create_packfile();

	written = write(fd, obj, size);
	if (written != size) {
		fprintf(stderr, "Error writing to packfile!\n");
		ret = -1;
		goto end;
	}

	memcpy(entry->sha1, sha1, SHA_DIGEST_LENGTH);
	memcpy(entry->packfile, pack_fname, PACK_FNAME_LEN+1);
	entry->offset = fd_offset;
	entry->len = size;
	add_pack_entry(entry);

	fd_offset += size;

	if (fd_offset > PACK_FILE_SIZE) 
		close_last_packfile();
	
end:
	return ret;
}

static int create_packfile()
{
	char path[PATH_MAX];
	
	get_pack_fname();
	sprintf(path, PACK_DIR"%s", pack_fname);

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
		return -1;
	}
	
	return 0;
}

static void add_pack_entry(struct pack_idx_entry *entry)
{

}

static void get_pack_fname()
{
	time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    strftime(pack_fname, sizeof(pack_fname), "%Y%m%d%H%M%S.pack", &tm);
}

void update_pack_idx()
{
	
}

void close_last_packfile()
{
	if (fd >= 0)
		close(fd);

	fd_offset = 0;
	fd = -1;
}

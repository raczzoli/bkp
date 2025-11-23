#include <asm-generic/errno-base.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <time.h>
#include <errno.h>

#include "pack.h"
#include "sha1-file.h"


static int fd = -1;
size_t fd_offset = 0;
char pack_fname[PACK_FNAME_LEN+1];

struct pack_index *pack_index = NULL;

static int resize_pack_index(struct pack_index *index, size_t size);
static size_t get_sha1_idx(unsigned char *sha1, size_t size);
static int add_pack_entry(struct pack_index *index, struct pack_idx_entry *entry);
static void get_pack_fname();
static int create_packfile();


int pack_object(unsigned char *sha1, const char *obj, size_t size)
{
	int ret = 0;
	size_t written = 0;
	struct pack_idx_entry *entry = NULL;

	if (!pack_index)
		load_pack_index();

	entry = malloc(sizeof(struct pack_idx_entry));
	entry->next = NULL;

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
	add_pack_entry(pack_index, entry);

	fd_offset += size;

	if (fd_offset > PACK_FILE_SIZE) 
		close_last_packfile();
	
end:
	return ret;
}

int unpack_object(unsigned char *sha1, char **out_buff, int *out_buff_len)
{
	int ret = 0;
	int fd = 0;
	off_t pos = -1;
	int bytes = 0;
	char *buff;
	char path[PATH_MAX];
	struct pack_idx_entry *entry = NULL;

	if (!pack_index)
		load_pack_index();

	entry = get_pack_entry(pack_index, sha1);

	if (!entry) {
		printf("Pack entry not found!\n");
		return -ENOENT;
	}

	buff = malloc(entry->len);
	if (!buff) {
		ret = -ENOMEM;
		fprintf(stderr, "Error allocating memory for pack object!\n");
		goto err;
	}

	sprintf(path, PACK_DIR"%s", entry->packfile);
	
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		ret = -ENOENT;
		fprintf(stderr, "Unable to open packfile %s (%s)!\n", path, strerror(errno));
		goto err;
	}

	pos = lseek(fd, entry->offset, SEEK_SET);
	if (pos == (off_t)-1) {
		fprintf(stderr, "Unable to seek to offset %ld in %s!\n", entry->offset, entry->packfile);
		ret = -1;
		goto err;
	}

	bytes = read(fd, buff, entry->len);
	if (bytes != entry->len) {
		fprintf(stderr, "Read %d bytes from %s (%d expected)!\n", bytes, entry->packfile, entry->len);
		ret = -1;
		goto err;
	}

	*out_buff = buff;
	*out_buff_len = entry->len;

	goto end;

err:
	if (buff)
		free(buff);

end:

	if (fd >= 0)
		close(fd);

	return ret;
}

struct pack_idx_entry *get_pack_entry(struct pack_index *index, unsigned char *sha1)
{
	if (!index)
		return NULL;

	size_t idx = get_sha1_idx(sha1, index->size);
	struct pack_idx_entry *exist_ent = index->entries[idx];
	
	while (exist_ent) {
		if (memcmp(exist_ent->sha1, sha1, SHA_DIGEST_LENGTH) == 0)
			return exist_ent;

		exist_ent = exist_ent->next;
	}	

	return NULL;
}

int load_pack_index()
{
	int ret = 0;
	int fd = -1;
	int offset = 0;
	struct pack_idx_entry *entry;
	struct stat cstat;
	void *cmap = NULL;

	printf("Loading packfile index into memory... ");
	fflush(stdout);

	pack_index = malloc(sizeof(struct pack_index));
	if (!pack_index) {
		fprintf(stderr, "Error allocating memory for pack index!\n");
		ret = -ENOMEM;
		goto err;
	}

	pack_index->size = 1024;
	pack_index->entries_len = 0;
	pack_index->entries = calloc(pack_index->size, sizeof(struct pack_idx_entry *));

	if (!pack_index->entries) {
		fprintf(stderr, "Error allocating memory for pack index entries!\n");
		free(pack_index);
		pack_index = NULL;

		ret = -ENOMEM;
		goto err;
	}

	fd = open(".bkp-data/pack_index", O_RDONLY);	
	if (fd < 0) 
		goto end; // not an error, it just doesn`t exist yet
	
	if (fstat(fd, &cstat)) {	
		fprintf(stderr, "Error calling fstat on filecache!\n");
		goto err;
	}
	
	cmap = mmap(NULL, cstat.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (cmap == MAP_FAILED) {
		fprintf(stderr, "mmap failed while mapping packfile index into memory!\n");
		goto err;
	}

	while(offset < cstat.st_size) {
		entry = cmap + offset;
		entry->next = NULL;
		offset += sizeof(struct pack_idx_entry);

		add_pack_entry(pack_index, entry);
	}

	goto end;

err:
	if (pack_index) {
		if (pack_index->entries)
			free(pack_index->entries);

		free(pack_index);
		pack_index = NULL;
	}

end:

	printf("done\n");
	return ret;
}

static int add_pack_entry(struct pack_index *index, struct pack_idx_entry *entry)
{
	if (pack_index->entries_len % pack_index->size == 0) {
		size_t new_size = pack_index->size * (pack_index->size == 0 ? 1024 : 2);
		resize_pack_index(pack_index, new_size);
	}

	size_t idx = get_sha1_idx(entry->sha1, index->size);
	struct pack_idx_entry *exist_ent = index->entries[idx];
	
	if (exist_ent) {
		while (exist_ent) {
			if (memcmp(exist_ent->sha1, entry->sha1, SHA_DIGEST_LENGTH) == 0)
				return 0;

			if (!exist_ent->next)
				break;

			exist_ent = exist_ent->next;
		}

		exist_ent->next = entry;
	}
	else {
		index->entries[idx] = entry;
		index->entries_len++;	
	}

	return 0;
}

static int resize_pack_index(struct pack_index *index, size_t size)
{
	int alloc_size = size * sizeof(struct pack_idx_entry *); 
	struct pack_idx_entry **entries = malloc(alloc_size);

	if (!entries) {
		fprintf(stderr, "Error allocating memory for new pack entries array!\n");
		return -ENOMEM;
	}

	memset(entries, 0, alloc_size);

	for (size_t i=0;i<index->entries_len;i++) {
		struct pack_idx_entry *e = index->entries[i];

		size_t new_idx = get_sha1_idx(e->sha1, size);

		entries[new_idx] = e;
	}

	free(index->entries);
	index->entries = entries;
	index->size = size;

	return 0;
}

static size_t get_sha1_idx(unsigned char *sha1, size_t size)
{
    size_t h = 146527;

    for (int i = 0; i < 20; i++) {
        h ^= sha1[i];
        h *= 1099511628211ULL; // 64-bit FNV-1a
    }

    return h % size;
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

static void get_pack_fname()
{
	time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    strftime(pack_fname, sizeof(pack_fname), "%Y%m%d%H%M%S.pack", &tm);
}

int update_pack_idx()
{
	int fd = open(".bkp-data/pack_index.new", O_WRONLY | O_CREAT | O_EXCL, 0666);

	if (fd < 0) {
		if (errno == EEXIST) 
			fprintf(stderr, "pack_index.new already exists! Maybe another pack index update in progress?\n");

		return -1;
	}

	if (pack_index->entries_len > 0) {
		for (size_t i=0;i<pack_index->size;i++) {
			struct pack_idx_entry *e = pack_index->entries[i];
			while(e) {
				printf("writing... %s\n", e->packfile);
				write(fd, e, sizeof(struct pack_idx_entry));
				e = e->next;
			}
		}
	}

	close(fd);
	rename(".bkp-data/pack_index.new", ".bkp-data/pack_index");


	return 0;
}

void close_last_packfile()
{
	if (fd >= 0)
		close(fd);

	fd_offset = 0;
	fd = -1;
}

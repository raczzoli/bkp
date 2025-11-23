#ifndef PACK_H
#define PACK_H 

#include <stddef.h>

#define PACK_FNAME_LEN 19 // 20250101000000.pack
#define PACK_DIR ".bkp-data/packs/"
#define PACK_FILE_SIZE 1024 * 1024 * 1024

int pack_object(unsigned char *sha1, const char *obj, size_t size);
void close_last_packfile();
int update_pack_idx();


struct pack_idx_entry {
	char packfile[PACK_FNAME_LEN+1];
	unsigned char sha1[20];
	size_t offset;
	int len;
};

struct pack_index {
	struct pack_idx_entry **entries;
	size_t entries_len;
	size_t size;
};

#endif

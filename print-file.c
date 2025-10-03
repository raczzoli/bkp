#include <openssl/sha.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "print-file.h"
#include "tree.h"
#include "file.h"
#include "snapshot.h"
#include "sha1-file.h"

int print_sha1_file(char *sha1_hex)
{
	int ret = 0;
	unsigned char sha1[SHA_DIGEST_LENGTH];
	char ftype[10] = {0};
	char *out_buff;
	int out_buff_len = 0;

	if (hex_to_sha1(sha1_hex, sha1)) {
		fprintf(stderr, "Invalid SHA1 value!\n");
		return -1;
	}

	ret = read_sha1_file(sha1, ftype, &out_buff, &out_buff_len);
	if (ret) 
		return -1;

	
	//if (strcmp(ftype, "snapshot") == 0) 
	//	return print_snapshot_file(sha1);

	if (strcmp(ftype, "tree") == 0) 
		return print_tree_buffer(out_buff, out_buff_len);

	//if (strcmp(ftype, "chunks") == 0) 
	//	return print_chunks_file(sha1);
	
	// blob
	// chunks
	// etc.

	return 0;
}


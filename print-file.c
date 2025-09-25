#include <openssl/sha.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "print-file.h"
#include "tree.h"
#include "sha1-file.h"
#include "snapshot.h"

int print_sha1_file(char *sha1_hex)
{
	int bytes = 0;
	int fd = 0;
	char path[PATH_MAX];
	unsigned char sha1[SHA_DIGEST_LENGTH];
	char ftype[20];

	if (hex_to_sha1(sha1_hex, sha1)) {
		fprintf(stderr, "Invalid SHA1 value!\n");
		return -1;
	}

	sprintf(path, ".bkp-data/%s", sha1_hex);
	
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "SHA1 file %s doesn`t exist!\n", sha1_hex);
		return -1;
	}

	bytes = read(fd, ftype, 20);
	if (bytes < 0) {
		fprintf(stderr, "Error reading from SHA1 file!\n");
		return -1;
	}
	close(fd);

	printf("----------- %s -----------\n", ftype);

	if (strcmp(ftype, "snapshot") == 0) 
		return print_snapshot_file(sha1);
	/*
	if (strcmp(ftype, "tree") == 0) 
		print_tree_file(fd, &stat);
	*/
	// blob
	// chunks
	// etc.

	return 0;
}


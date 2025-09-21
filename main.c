#include <stdio.h>
#include <stdlib.h>
#include <openssl/sha.h>

#include "tree.h"
#include "sha1.h"

int main()
{
	int ret = 0;
	unsigned char sha1[SHA_DIGEST_LENGTH];
	char sha1_hex[40+1];

	ret = gen_tree("/home/rng89/workspace/linux/", sha1);

	if (ret) {
		fprintf(stderr, "Error generating tree!\n");
		return -1;
	}

	sha1_to_hex(sha1, sha1_hex);

	printf("Tree sha1: %s\n", sha1_hex);

	return 0;
}


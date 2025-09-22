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
#include <openssl/sha.h>

#include "tree.h"
#include "sha1.h"

int main()
{
	int ret = 0;
	unsigned char sha1[SHA_DIGEST_LENGTH];
	char sha1_hex[40+1];

	ret = gen_tree("/home/rng89/", sha1);

	if (ret) {
		fprintf(stderr, "Error generating tree (code: %d)!\n", ret);
		return -1;
	}

	sha1_to_hex(sha1, sha1_hex);

	printf("Tree sha1: %s\n", sha1_hex);

	return 0;
}


#ifndef SHA1_H
#define SHA1_H

int sha1_to_hex(unsigned char *sha1, char* out_hex);
int write_sha1_file(unsigned char *sha1, char *buffer, int len);

#endif

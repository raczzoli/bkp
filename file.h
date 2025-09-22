#ifndef FILE_H
#define FILE_H

#define FILE_CHUNK_SIZE (10 * (1024 * 1024))

int write_file(char *path, off_t size, unsigned char *sha1);

#endif

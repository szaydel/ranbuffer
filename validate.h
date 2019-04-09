#ifndef VALIDATE_H
#define VALIDATE_H
#include <unistd.h>

#define MAXIMUM_FILE_SIZE 1L << 40
#define MINIMUM_FILE_SIZE 16L << 10

size_t
adjust_max_filesize(size_t max);
size_t
adjust_min_filesize(size_t lower, size_t upper);
#endif // !VALIDATE_H
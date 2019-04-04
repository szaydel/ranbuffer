#ifndef GENFILE_H
#define GENFILE_H
#include <unistd.h>
ssize_t
generate_file(char* path, void* buf, size_t bufsz, size_t nbytes);

#endif // !GENFILE_H
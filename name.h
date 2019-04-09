#ifndef NAME_H
#define NAME_H

#include <unistd.h>

#define UUID_BYTES 37

const char*
random_filename(char* name, size_t len);
#endif // !NAME_H
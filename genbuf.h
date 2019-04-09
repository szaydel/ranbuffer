#ifndef GENBUF_H
#define GENBUF_H

#include <sodium.h>

extern const unsigned char seed_array[randombytes_SEEDBYTES];

void
random_fill_buffer(void* buf, const size_t size);
#endif // !GENBUF_H
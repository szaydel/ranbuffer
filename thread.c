#define _GNU_SOURCE

#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sodium.h>
#include <uuid/uuid.h>

#include "genfile.h"
#include "name.h"
#include "thread.h"

// extern ssize_t
// string_to_bytes(const char* arg);
// static const char*
// random_filename(char* name, size_t len);
// ssize_t
// generate_file(char* path, void* buf, size_t bufsz, size_t nbytes);

void*
io_thread(void* arg)
{
  thread_data_t* td = (thread_data_t*)arg;
  char* path = malloc(pathconf(td->directory, _PC_PATH_MAX) * sizeof(char));
  if (path == NULL) {
    fprintf(stderr,
            "thread[%u] Failed to allocate path, "
            "directory name may not exist\n",
            td->suffix);
    return NULL;
  }
  char name[37];
  uint32_t r;
  td->file_count = 0;
  td->written = 0;
  while (td->written <
         ((double)(td->total_data) * td->margin_of_error) + td->total_data) {
    r = randombytes_uniform(td->max_file_size);
    if (r < td->min_file_size) {
      r += td->min_file_size;
    }
    random_filename(name, 37);

    sprintf(path, "%s/%s.%u", td->directory, name, td->suffix);
#ifdef DEBUG
    fprintf(stderr, "DEBUG: %s\n", path);
#endif
    ssize_t written = generate_file(path, td->buf, td->bufsz, r);

    if (written == -1) {
      char* err;
      asprintf(&err, "Failed to write data into %s", path);
      perror(err);
      free(err);
      free(path);
      return NULL;
    }
    td->written += written;
    ++td->file_count;
  }

  free(path);
  return arg;
}

#ifndef THREAD_H
#define THREAD_H

#include <unistd.h>

void* io_thread(void* arg);

typedef struct thread_data
{
  char* directory;
  double margin_of_error;
  size_t min_file_size;
  size_t max_file_size;
  size_t total_data;
  size_t file_count;
  size_t written;
  size_t bufsz;
  void* buf;
  unsigned suffix;
  unsigned verbose;
} thread_data_t;

#endif // !THREAD_H
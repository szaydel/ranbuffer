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

#include "genbuf.h"
#include "genfile.h"
#include "name.h"
#include "thread.h"
#include "validate.h"

#ifndef NUM_THREADS
#define NUM_THREADS 4
#endif
#define WORKING_DIRECTORY "."
#define MAXIMUM_FILE_SIZE 1L << 40
#define MINIMUM_FILE_SIZE 16L << 10
#define RANDOM_DATA_BUFSIZE 1L << 20
#define RANDOM_DATA_BUFSIZE_MAX 100L << 20
#define RANDOM_DATA_BUFSIZE_MIN 1L << 20

extern ssize_t
string_to_bytes(const char* arg);

int
main(int argc, char** argv)
{
  size_t thread_count = NUM_THREADS; // Adjustable parallel threads
  size_t total_data = 100L << 20; // 100MB
  size_t min_file = 1L << 20;     // 1MB
  size_t max_file = min_file * 10;
  size_t rand_buffer_size =
    RANDOM_DATA_BUFSIZE_MIN; // Size of random data buffer

  double margin_of_error = 0.05;
  char* path = NULL;
  int retcode = 0;
  int c;
  opterr = 0;
  while ((c = getopt(argc, argv, ":hl:p:t:u:")) != EOF) {
    switch (c) {
      case 'h':
        fprintf(stderr, "Usage: %s [OPTION]... <path>\n", argv[0]);
        fprintf(stderr,
                "\nOptions:\n"
                "  -p <COUNT>\t\tNumber of parallel threads\n"
                "  -l <SIZE>\t\tLower limit on file size\n"
                "  -u <SIZE>\t\tUpper limit on file size\n"
                "  -t <SIZE>\t\tAggregate (total) file size\n"
                "\n\nThe SIZE argument is an integer with an optional unit."
                "\nSIZE Units: B,K,KB,M,MB,G,GB,T,TB\n");
        return 2;
        break;
      case 'p':
        thread_count = atol(optarg);
        if (thread_count < 1)
          thread_count = NUM_THREADS;
        else if (thread_count > NUM_THREADS * 10)
          thread_count = NUM_THREADS;
        break;
      case 'l':
        // Minimum file size should be no less than 2 * sqrt of maximum
        // and not larger than 1/2 of maximum, that is,
        // min_file >= 2*sqrt(max_file) and max_file >= min_file*2.
        min_file = string_to_bytes(optarg);
        if (min_file == -1) {
          fprintf(stderr, "Error: failed to parse minimum file size\n");
          return 1;
        }
        break;
      case 'u':
        // Maximum file size should be at least 2x that of minimum, i.e.
        // max_file >= min_file*2.
        max_file = string_to_bytes(optarg);
        if (max_file == -1) {
          fprintf(stderr, "Error: failed to parse maximum file size\n");
          return 1;
        }
        break;
      case 't':
        total_data = string_to_bytes(optarg);
        break;
      case '?':
        fprintf(stderr, "Unrecognized option: '-%c'\n", optopt);
    }
  }

  max_file = adjust_max_filesize(max_file);
  min_file = adjust_min_filesize(min_file, max_file);

#ifdef DEBUG
  printf("max_file => %zu | min_file => %zu\n", max_file, min_file);
  exit(0);
#endif

  // Remaining arguments are assumed to be path(s).
  // Since only a single path is supported, ignore anything
  // after the first remaining argument.
  if (optind < argc) {
    // While there are trailing slashes in the path,
    // replace them with null byte.
    while (argv[optind][strlen(argv[optind]) - 1] == '/')
      argv[optind][strlen(argv[optind]) - 1] = '\0';
    path = argv[optind];
  }

  if (max_file > total_data) {
    fprintf(stderr,
            "Error: total data amount must be "
            "greater than maximum file size\n");
    return 1;
  }

  if (sodium_init() < 0) {
    fprintf(stderr, "Error: failed to initialize random number generator\n");
    return 1;
  }

  // Compute size of random data buffer, limiting lower threshold by
  // RANDOM_DATA_BUFSIZE_MIN, and upper threshold by RANDOM_DATA_BUFSIZE_MAX.
  // In other words random data buffer length will be within
  // range(RANDOM_DATA_BUFSIZE_MIN, RANDOM_DATA_BUFSIZE_MAX);
  if (min_file < RANDOM_DATA_BUFSIZE_MIN) {
    rand_buffer_size = RANDOM_DATA_BUFSIZE_MIN;
  } else if (min_file > RANDOM_DATA_BUFSIZE_MAX) {
    rand_buffer_size = RANDOM_DATA_BUFSIZE_MAX;
  } else {
    rand_buffer_size = min_file;
  }
  void* buf = malloc(rand_buffer_size);
  if (buf == NULL) {
    perror("Failed to allocate memory!");
    return 1;
  } else {
    random_fill_buffer(buf, rand_buffer_size);
  }

  pthread_t** threads = NULL;
  thread_data_t** params = NULL;

  threads = malloc(sizeof(pthread_t*) * thread_count);
  if (threads == NULL) {
    perror("Unable to allocate threads array");
    return 1;
  }
  params = malloc(sizeof(thread_data_t*) * thread_count);
  if (params == NULL) {
    perror("Unable to allocate params array");
    return 1;
  }

  for (size_t i = 0; i < thread_count; i++) {
    params[i] = (thread_data_t*)malloc(sizeof(thread_data_t));
    if (params[i] == NULL) {
      perror("Unable to allocate new thread context structure");
      return 1;
    }

    memset(params[i], 0, sizeof(thread_data_t));

    if (path) {
      params[i]->directory = path;
    } else {
      params[i]->directory = WORKING_DIRECTORY;
    }

    params[i]->margin_of_error = margin_of_error;
    params[i]->min_file_size = min_file;
    params[i]->max_file_size = max_file;
    params[i]->total_data = total_data / thread_count;
    params[i]->bufsz = rand_buffer_size;
    params[i]->buf = buf;
    params[i]->suffix = i;
  }

  for (size_t i = 0; i < thread_count; i++) {
    threads[i] = (pthread_t*)malloc(sizeof(pthread_t));
    if (threads[i] == NULL) {
      perror("Unable to allocate new thread");
      return 1;
    }

    int status = pthread_create(threads[i], NULL, io_thread, params[i]);
    if (status != 0) {
      fprintf(stderr, "Create thread failed, errno: %d\n", status);
      return 1;
    }
  }

  for (size_t i = 0; i < thread_count; i++) {
    void* arg = NULL;
    pthread_join(*threads[i], &arg);
    // If arg here is NULL, something went wrong in the thread.
    // In this instance set return code to 1 to indicate program
    // is partially successful or entirely unsuccessful.
    if (arg == NULL) {
      retcode = 1;
      continue;
    }
    // Produce final statistics and free resources,
    // though freeing here does not really matter.
    if ((params[i]->written / params[i]->file_count) > (1L << 30)) {
      printf("thread[%zu] generated %zu files, averaging %zuGB/file\n",
             i,
             params[i]->file_count,
             (params[i]->written / params[i]->file_count) << 30);
    } else if ((params[i]->written / params[i]->file_count) >
               (size_t)(1 << 20)) {
      printf("thread[%zu] generated %zu files, averaging %zuMB/file\n",
             i,
             params[i]->file_count,
             (params[i]->written / params[i]->file_count) >> 20);
    } else {
      printf("thread[%zu] generated %zu files, averaging %zuKB/file\n",
             i,
             params[i]->file_count,
             (params[i]->written / params[i]->file_count) >> 10);
    }
    free(params[i]);
    free(threads[i]);
  }

  // One day freeing these may actually matter.
  free(params);
  free(threads);
  free(buf);

  return retcode;
}

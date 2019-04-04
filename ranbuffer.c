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

#include "genfile.h"
#include "name.h"
#include "thread.h"

#ifndef NUM_THREADS
#define NUM_THREADS 4
#endif
#define WORKING_DIRECTORY "."
#define RANDOM_DATA_BUFSIZE 1L << 20

extern ssize_t
string_to_bytes(const char* arg);

int
main(int argc, char** argv)
{
  // These will have to come from command line arguments at some point
  size_t total_data = 100L << 20; // 100MB
  size_t min_file = 1L << 20;     // 1MB
  size_t max_file = min_file * 10;
  double margin_of_error = 0.05;
  char* path = NULL;
  int retcode = 0;
  int c;
  opterr = 0;
  while ((c = getopt(argc, argv, ":hm:t:")) != EOF) {
    switch (c) {
      case 'h':
        fprintf(stderr,
                "Usage: %s [ -m max file size ]"
                "[ -t total size ] <path>\n",
                argv[0]);
        fprintf(stderr, "       Size Units: B, K, KB, M, MB, G, GB, T, TB\n");
        return 2;
        break;
      case 'm':
        // Maximum file size should be at least 2x that of minimum, i.e.
        // max_file >= min_file*2.
        max_file = string_to_bytes(optarg);
        if (max_file == -1) {
          fprintf(stderr, "Error: failed to parse maximum file size\n");
          return 1;
        }
        if (max_file < min_file * 2)
          max_file = min_file * 2;
        break;
      case 't':
        total_data = string_to_bytes(optarg);
        break;
      case '?':
        fprintf(stderr, "Unrecognized option: '-%c'\n", optopt);
    }
  }

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
    return 1;
  }

  void* buf = malloc(RANDOM_DATA_BUFSIZE);
  if (buf == NULL) {
    perror("Failed to allocate memory!");
    return 1;
  }

  pthread_t** threads = NULL;
  thread_data_t** params = NULL;

  threads = malloc(sizeof(pthread_t*) * NUM_THREADS);
  if (threads == NULL) {
    perror("Unable to allocate threads array");
    return 1;
  }
  params = malloc(sizeof(thread_data_t*) * NUM_THREADS);
  if (params == NULL) {
    perror("Unable to allocate params array");
    return 1;
  }

  for (size_t i = 0; i < NUM_THREADS; i++) {
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
    params[i]->total_data = total_data / NUM_THREADS;
    params[i]->bufsz = RANDOM_DATA_BUFSIZE;
    params[i]->buf = buf;
    params[i]->suffix = i;
  }

  for (size_t i = 0; i < NUM_THREADS; i++) {
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

  for (size_t i = 0; i < NUM_THREADS; i++) {
    void *arg = NULL;
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
    printf("thread[%zu] generated %zu files, averaging %zuMB/file\n",
        i,
        params[i]->file_count,
        (params[i]->written / params[i]->file_count) >> 20);
    free(params[i]);
    free(threads[i]);
  }

  // One day freeing these may actually matter.
  free(params);
  free(threads);
  free(buf);

  return retcode;
}

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

#ifdef __gnu_linux__
#include <uuid/uuid.h>
#elif __APPLE__
#include <uuid/uuid.h>
#endif

#ifndef NUM_THREADS
#define NUM_THREADS 4
#endif
#define WORKING_DIRECTORY "."
#define UUID_BYTES 37
#define RANDOM_DATA_BUFSIZE 1L << 20

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

extern ssize_t
string_to_bytes(const char* arg);
static const char*
random_filename(char* name, size_t len);
static ssize_t
generate_file(char* path, void* buf, size_t bufsz, size_t nbytes);

static const char*
random_filename(char* name, size_t len)
{
  uuid_t uu;
#ifdef __APPLE__
  uuid_string_t uu_str;
  uuid_string_t p_name;
#elif __gnu_linux__
  char uu_str[UUID_BYTES];
  char p_name[UUID_BYTES];
#endif
  uuid_generate_random(uu);
#ifdef __gnu_linux__
  uuid_unparse_lower(uu, (char*)uu_str);
  strncpy(p_name, (const char*)uu_str, len);
#else
  uuid_unparse_lower(uu, uu_str);
  strncpy(p_name, uu_str, len);
#endif
  char* p = name;
  // We walk over the p_name array and skip hyphens '-', copying everything
  // else.
  for (size_t i = 0; i < strlen(p_name); i++) {
    if (p_name[i] == '-')
      continue;
    *p = p_name[i];
    p++;
  }
  *p = '\0';
  return name;
}

void*
io_thread(void* arg)
{
  thread_data_t* td = (thread_data_t*)arg;
  char* path = malloc(pathconf(td->directory, _PC_PATH_MAX) * sizeof(char));
  char name[37];
  // size_t generated = 0;
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

static ssize_t
generate_file(char* path, void* buf, size_t bufsz, size_t nbytes)
{
  int fd;
  mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  int oflag = O_WRONLY | O_CREAT | O_APPEND;

  if (path == NULL) {
    return -1;
  }

  // Check if parent directory is a valid path, and fail if it is not.
  char* pathcopy = strdup(path);
  DIR* dir;
  if ((dir = opendir(dirname(pathcopy))) == NULL) {
    char* err;
    asprintf(&err, "Failed to open parent directory %s", pathcopy);
    perror(err);
    free(err);
    free(pathcopy);
    return -1;
  } else {
    closedir(dir);
    free(pathcopy);
  }

  if ((fd = open(path, oflag, mode)) == -1) {
    perror("Failed to open file");
    return -1;
  };
  ssize_t written = 0;
  size_t remainder = nbytes;

  while (remainder > 0) {
    // Write either bufsize bytes or remainder bytes to the file, if remainder
    // is less than the size of buffer.
    ssize_t result = write(fd, buf, bufsz > remainder ? remainder : bufsz);
    if (result == -1) {
      char* err;
      asprintf(&err, "Failed to write buffer to %s", path);
      perror(err);
      free(err);
      return -1;
    }
    written += result;
    remainder = nbytes - written;
  }
  return written;
}

int
main(int argc, char** argv)
{
  // These will have to come from command line arguments at some point
  size_t total_data = 100L << 20; // 100MB
  size_t min_file = 1L << 20;     // 1MB
  size_t max_file = min_file * 10;
  double margin_of_error = 0.05;
  char* path = NULL;

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
    pthread_join(*threads[i], NULL);
  }

  // One day freeing these may actually matter.
  free(buf);

  // Produce final statistics and free resources,
  // though freeing here does not really matter.
  for (size_t i = 0; i < NUM_THREADS; i++) {
    printf("thread[%zu] generated %zu files, averaging %zuMB/file\n",
           i,
           params[i]->file_count,
           (params[i]->written / params[i]->file_count) >> 20);
    free(params[i]);
    free(threads[i]);
  }
  free(params);
  free(threads);
  return 0;
}

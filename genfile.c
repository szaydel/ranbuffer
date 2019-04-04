#define _GNU_SOURCE

#include <dirent.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "genfile.h"

ssize_t
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
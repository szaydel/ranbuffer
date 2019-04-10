#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "validate.h"

size_t
adjust_max_filesize(size_t max)
{
  if (max < MINIMUM_FILE_SIZE)
    return MINIMUM_FILE_SIZE;
  if (max > MAXIMUM_FILE_SIZE)
    return MAXIMUM_FILE_SIZE;
  return max;
}

size_t
adjust_min_filesize(size_t lower, size_t upper)
{
  if ((double)lower < (2. * sqrt(upper))) {
    return 2. * sqrt(upper);
  }
  if (lower > (upper / 2))
    return upper / 2;
  return lower;
}
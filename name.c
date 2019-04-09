#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uuid/uuid.h>

#include "name.h"

const char*
random_filename(char* name, size_t len)
{
  uuid_t uu;
#if defined __APPLE__
  uuid_string_t uu_str;
  uuid_string_t p_name;
#elif defined(__sun) || defined(__gnu_linux__)
  char uu_str[UUID_BYTES];
  char p_name[UUID_BYTES];
#endif
  uuid_generate_random(uu);
#if defined(__sun) || defined(__gnu_linux__)
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
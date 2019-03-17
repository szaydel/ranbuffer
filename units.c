#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>

typedef enum {
  INVALID = -1, B, KB, MB, GB, TB,
} unit_t;

size_t units[5] = {
  [B] = 1L, [KB] = 1L<<10, [MB] = 1L<<20,
  [GB] = 1L<<30, [TB] = 1L<<40,
};

ssize_t string_to_bytes(const char *arg) {
  char unitc = '\0';
  size_t intval = 0;
  unit_t parsed_unit = INVALID;
  if (!isdigit(arg[0])) {
    return -1;
  }

  char *numeric_prefix = calloc(strlen(arg), sizeof(char));
  if (!numeric_prefix) {
    perror("Failed to allocate memory for string");
    return -1;
  }

  for (size_t i = 0; i < strlen(arg) && unitc == '\0' ; i++) {
    if (isdigit(arg[i])) {
      numeric_prefix[i] = arg[i];
      continue;
    }
    // We only care about the first character here, so tb, TB, t, or T are all same.
    unitc = arg[i];

    // Extract a number from numeric string.
    intval = (size_t)atoll(numeric_prefix);
    if (!intval) return -1;
    switch (arg[i]) {
      case 'b': case 'B':
        parsed_unit = B;
        break;
      case 'k': case 'K':
        parsed_unit = KB;
        break;
      case 'm': case 'M':
        parsed_unit = MB;
        break;
      case 'g': case 'G':
        parsed_unit = GB;
        break;
      case 't': case 'T':
        parsed_unit = TB;
        break;
      default:
        break;
    };

    break;
  }
  if (parsed_unit == INVALID) return INVALID;
  return intval * units[parsed_unit];
}

// int main(void) {
//   printf("%zd\n", string_to_bytes("1024b"));
//   printf("%zd\n", string_to_bytes("102m"));
//   printf("%zd\n", string_to_bytes("102M"));
//   printf("%zd\n", string_to_bytes("102MB"));
//   printf("%zd\n", string_to_bytes("0xaMB"));
//   printf("%zd\n", string_to_bytes("114g"));
//   printf("%zd\n", string_to_bytes("507k"));
//   printf("%zd\n", string_to_bytes("568e"));
//   printf("%zd\n", string_to_bytes("-568b"));
//   printf("%zd\n", string_to_bytes("a568KB"));
//   return 0;
// }

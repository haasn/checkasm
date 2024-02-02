#include "nihcpy.h"

void *nihcpy(void *dest, const void *src, size_t n) {
  char *cdest = (char *)dest;
  const char *csrc = (const char *)src;
  while (n-- > 0) *cdest++ = *csrc++;
  return dest;
}

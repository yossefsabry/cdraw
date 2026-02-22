#include "ai_base64.h"
#include <stdlib.h>

char *AiBase64Encode(const unsigned char *data,
                     size_t len, size_t *out_len) {
  static const char *tab =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  size_t enc = 4 * ((len + 2) / 3);
  char *out = (char *)malloc(enc + 1);
  if (!out)
    return NULL;
  size_t j = 0;
  for (size_t i = 0; i < len; i += 3) {
    unsigned int v = data[i] << 16;
    if (i + 1 < len)
      v |= data[i + 1] << 8;
    if (i + 2 < len)
      v |= data[i + 2];
    out[j++] = tab[(v >> 18) & 0x3F];
    out[j++] = tab[(v >> 12) & 0x3F];
    out[j++] = (i + 1 < len) ?
               tab[(v >> 6) & 0x3F] : '=';
    out[j++] = (i + 2 < len) ?
               tab[v & 0x3F] : '=';
  }
  out[j] = '\0';
  if (out_len)
    *out_len = j;
  return out;
}

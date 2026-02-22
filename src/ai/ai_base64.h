#ifndef AI_BASE64_H
#define AI_BASE64_H

#include <stddef.h>

char *AiBase64Encode(const unsigned char *data,
                     size_t len,
                     size_t *out_len);

#endif

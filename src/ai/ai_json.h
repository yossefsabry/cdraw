#ifndef AI_JSON_H
#define AI_JSON_H

#include <stdbool.h>
#include <stddef.h>

char *AiJsonEscape(const char *s);
bool AiJsonFindString(const char *json,
                      const char *key,
                      char *out,
                      size_t out_sz);

#endif

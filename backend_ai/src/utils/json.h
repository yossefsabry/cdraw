#ifndef BACKEND_JSON_H
#define BACKEND_JSON_H

#include <stdbool.h>
#include <stddef.h>

char *JsonEscape(const char *s);
bool JsonFindString(const char *json, const char *key, char *out, size_t out_sz);
char *JsonExtractRaw(const char *json, const char *key);

#endif

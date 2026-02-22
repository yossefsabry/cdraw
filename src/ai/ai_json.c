#include "ai_json.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *AiJsonEscape(const char *s) {
  size_t len = 0;
  for (const char *p = s; p && *p; p++) {
    switch (*p) {
    case '\\':
    case '"':
    case '\n':
    case '\r':
    case '\t':
      len += 2;
      break;
    default:
      len += 1;
      break;
    }
  }
  char *out = (char *)malloc(len + 1);
  if (!out)
    return NULL;
  size_t j = 0;
  for (const char *p = s; p && *p; p++) {
    if (*p == '\\' || *p == '"') {
      out[j++] = '\\';
      out[j++] = *p;
    } else if (*p == '\n') {
      out[j++] = '\\';
      out[j++] = 'n';
    } else if (*p == '\r') {
      out[j++] = '\\';
      out[j++] = 'r';
    } else if (*p == '\t') {
      out[j++] = '\\';
      out[j++] = 't';
    } else {
      out[j++] = *p;
    }
  }
  out[j] = '\0';
  return out;
}

static const char *SkipWs(const char *p) {
  while (p && *p && isspace((unsigned char)*p))
    p++;
  return p;
}

bool AiJsonFindString(const char *json,
                      const char *key,
                      char *out,
                      size_t out_sz) {
  if (!json || !key || !out || out_sz == 0)
    return false;
  char pat[64];
  snprintf(pat, sizeof(pat), "\"%s\"", key);
  const char *p = strstr(json, pat);
  if (!p)
    return false;
  p += strlen(pat);
  p = SkipWs(p);
  p = strchr(p, ':');
  if (!p)
    return false;
  p++;
  p = SkipWs(p);
  if (*p != '"')
    return false;
  p++;
  size_t j = 0;
  while (*p && j + 1 < out_sz) {
    if (*p == '"')
      break;
    if (*p == '\\') {
      p++;
      if (*p == 'n')
        out[j++] = '\n';
      else if (*p == 'r')
        out[j++] = '\r';
      else if (*p == 't')
        out[j++] = '\t';
      else
        out[j++] = *p;
      if (*p)
        p++;
      continue;
    }
    out[j++] = *p++;
  }
  out[j] = '\0';
  return j > 0;
}

#include "http_client.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *data;
  size_t len;
  size_t cap;
} Buf;

static size_t WriteCb(char *ptr, size_t size, size_t nmemb, void *ud) {
  Buf *b = (Buf *)ud;
  size_t n = size * nmemb;
  if (!b)
    return n;
  if (b->len + n + 1 > b->cap) {
    size_t new_cap = b->cap == 0 ? 4096 : b->cap * 2;
    while (new_cap < b->len + n + 1)
      new_cap *= 2;
    char *next = (char *)realloc(b->data, new_cap);
    if (!next)
      return 0;
    b->data = next;
    b->cap = new_cap;
  }
  memcpy(b->data + b->len, ptr, n);
  b->len += n;
  b->data[b->len] = '\0';
  return n;
}

static void SanitizeSnippet(const char *src, char *out, size_t out_sz) {
  if (!out || out_sz == 0)
    return;
  out[0] = '\0';
  if (!src)
    return;
  size_t j = 0;
  for (size_t i = 0; src[i] && j + 1 < out_sz; i++) {
    unsigned char c = (unsigned char)src[i];
    if (c < 32 || c == 127)
      c = ' ';
    out[j++] = (char)c;
  }
  out[j] = '\0';
}

static bool CurlInitOnce(char *err, size_t err_sz) {
  static int ready = 0;
  if (ready)
    return true;
  if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
    if (err && err_sz > 0)
      snprintf(err, err_sz, "curl init failed");
    return false;
  }
  ready = 1;
  return true;
}

bool HttpPostJson(const char *url,
                  const char *body,
                  const char *auth,
                  char **out,
                  char *err,
                  size_t err_sz) {
  if (!url || !body || !out)
    return false;
  *out = NULL;
  if (!CurlInitOnce(err, err_sz))
    return false;

  CURL *curl = curl_easy_init();
  if (!curl)
    return false;

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json");
  if (auth && auth[0] != '\0') {
    char line[256];
    snprintf(line, sizeof(line), "Authorization: Bearer %s", auth);
    headers = curl_slist_append(headers, line);
  }

  Buf buf = {0};
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

  CURLcode res = curl_easy_perform(curl);
  long code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    if (err && err_sz > 0)
      snprintf(err, err_sz, "curl error: %s", curl_easy_strerror(res));
    free(buf.data);
    return false;
  }
  if (code < 200 || code >= 300) {
    if (err && err_sz > 0)
      if (buf.data && buf.data[0] != '\0') {
        char snippet[160];
        SanitizeSnippet(buf.data, snippet, sizeof(snippet));
        snprintf(err, err_sz, "http %ld: %s", code, snippet);
      } else {
        snprintf(err, err_sz, "http %ld", code);
      }
    free(buf.data);
    return false;
  }
  if (!buf.data) {
    buf.data = (char *)malloc(1);
    if (!buf.data)
      return false;
    buf.data[0] = '\0';
  }
  *out = buf.data;
  return true;
}

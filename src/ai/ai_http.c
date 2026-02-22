#include "ai_http.h"
#include <curl/curl.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  char *buf;
  size_t cap;
  size_t len;
  bool overflow;
} AiBuf;

static size_t WriteCb(char *ptr, size_t size,
                      size_t nmemb, void *ud) {
  AiBuf *b = (AiBuf *)ud;
  size_t n = size * nmemb;
  if (!b || !b->buf || b->cap == 0)
    return n;
  size_t room = (b->cap - 1) - b->len;
  size_t take = (n > room) ? room : n;
  if (take > 0) {
    memcpy(b->buf + b->len, ptr, take);
    b->len += take;
    b->buf[b->len] = '\0';
  }
  if (take < n)
    b->overflow = true;
  return n;
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

bool AiHttpPostJson(const char *url,
                    const char *body,
                    const char *auth,
                    char *out,
                    size_t out_sz,
                    char *err,
                    size_t err_sz) {
  if (!url || !body || !out || out_sz == 0)
    return false;
  out[0] = '\0';
  if (!CurlInitOnce(err, err_sz))
    return false;

  CURL *curl = curl_easy_init();
  if (!curl)
    return false;

  AiBuf buf = {out, out_sz, 0, false};
  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers,
                              "Content-Type: application/json");
  if (auth && auth[0] != '\0') {
    char line[256];
    snprintf(line, sizeof(line),
             "Authorization: Bearer %s", auth);
    headers = curl_slist_append(headers, line);
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                   WriteCb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                   &buf);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

  CURLcode res = curl_easy_perform(curl);
  long code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE,
                    &code);

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (res != CURLE_OK) {
    if (err && err_sz > 0)
      snprintf(err, err_sz, "curl error: %s",
               curl_easy_strerror(res));
    return false;
  }
  if (code < 200 || code >= 300) {
    if (err && err_sz > 0)
      snprintf(err, err_sz, "http %ld", code);
    return false;
  }
  if (buf.overflow) {
    if (err && err_sz > 0)
      snprintf(err, err_sz, "response too large");
    return false;
  }
  return true;
}

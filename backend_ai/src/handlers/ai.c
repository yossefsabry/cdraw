#include "ai.h"
#include "../utils/http_client.h"
#include "../utils/json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void RespondError(HttpResponse *res, int status, const char *msg) {
  if (!res)
    return;
  res->status = status;
  res->content_type = "application/json";
  char *esc = JsonEscape(msg ? msg : "error");
  if (!esc)
    esc = strdup("error");
  size_t sz = strlen(esc) + 64;
  res->body = (char *)malloc(sz);
  if (res->body)
    snprintf(res->body, sz,
             "{\"status\":\"error\",\"message\":\"%s\"}", esc);
  free(esc);
}

static char *JsonExtractStringAlloc(const char *json, const char *key) {
  char *raw = JsonExtractRaw(json, key);
  if (!raw)
    return NULL;
  size_t len = strlen(raw);
  if (len >= 2 &&
      ((raw[0] == '"' && raw[len - 1] == '"') ||
       (raw[0] == '\'' && raw[len - 1] == '\''))) {
    raw[len - 1] = '\0';
    char *out = strdup(raw + 1);
    free(raw);
    return out;
  }
  return raw;
}

static bool HasChatCompletions(const char *base) {
  return base && strstr(base, "/chat/completions");
}

static bool HasV1Segment(const char *base) {
  if (!base)
    return false;
  const char *p = base;
  while ((p = strstr(p, "/v1")) != NULL) {
    char next = p[3];
    if (next == '\0' || next == '/' || next == '?' || next == '#')
      return true;
    p += 3;
  }
  return false;
}

static void StripTrailingSlash(const char *src, char *out, size_t out_sz) {
  if (!out || out_sz == 0)
    return;
  out[0] = '\0';
  if (!src || src[0] == '\0')
    return;
  size_t len = strlen(src);
  while (len > 0 && src[len - 1] == '/')
    len--;
  if (len >= out_sz)
    len = out_sz - 1;
  memcpy(out, src, len);
  out[len] = '\0';
}

static void BuildLocalUrl(const char *base, char *out, size_t out_sz) {
  if (!out || out_sz == 0)
    return;
  out[0] = '\0';
  if (!base || base[0] == '\0')
    base = "http://localhost:11434/v1";

  char clean[512];
  StripTrailingSlash(base, clean, sizeof(clean));
  if (clean[0] == '\0')
    strncpy(clean, "http://localhost:11434/v1", sizeof(clean) - 1);
  clean[sizeof(clean) - 1] = '\0';

  if (HasChatCompletions(clean)) {
    size_t n = strnlen(clean, out_sz - 1);
    memcpy(out, clean, n);
    out[n] = '\0';
    return;
  }

  const char *suffix = HasV1Segment(clean) ? "/chat/completions" : "/v1/chat/completions";
  snprintf(out, out_sz, "%s%s", clean, suffix);
}

static const char *StripImagePrefix(const char *image) {
  if (!image)
    return NULL;
  const char *p = strstr(image, "base64,");
  if (p)
    return p + 7;
  return image;
}

static bool SendLocal(const char *model,
                      const char *prompt,
                      const char *base_url,
                      const char *api_key,
                      char **out_text,
                      char *err,
                      size_t err_sz) {
  char url[512];
  BuildLocalUrl(base_url, url, sizeof(url));
  char *prompt_esc = JsonEscape(prompt);
  char *model_esc = JsonEscape(model);
  if (!prompt_esc || !model_esc) {
    free(prompt_esc);
    free(model_esc);
    if (err && err_sz > 0)
      snprintf(err, err_sz, "prompt escape failed");
    return false;
  }
  size_t body_sz = strlen(prompt_esc) + strlen(model_esc) + 512;
  char *body = (char *)malloc(body_sz);
  if (!body) {
    free(prompt_esc);
    free(model_esc);
    return false;
  }
  snprintf(body, body_sz,
           "{\"model\":\"%s\",\"messages\":["
           "{\"role\":\"system\",\"content\":"
           "\"You analyze sketches and return bullets.\"},"
           "{\"role\":\"user\",\"content\":\"%s\"}],"
           "\"temperature\":0.2}",
           model_esc, prompt_esc);
  char *resp = NULL;
  bool ok = HttpPostJson(url, body, api_key, &resp, err, err_sz);
  free(body);
  free(prompt_esc);
  free(model_esc);
  if (!ok)
    return false;
  char text[4096] = {0};
  bool found = JsonFindString(resp, "content", text, sizeof(text));
  if (!found) {
    free(resp);
    if (err && err_sz > 0)
      snprintf(err, err_sz, "parse failed");
    return false;
  }
  *out_text = strdup(text);
  free(resp);
  return *out_text != NULL;
}

static bool SendGemini(const char *model,
                       const char *prompt,
                       const char *image,
                       const char *api_key,
                       char **out_text,
                       char *err,
                       size_t err_sz) {
  if (!api_key || api_key[0] == '\0') {
    if (err && err_sz > 0)
      snprintf(err, err_sz, "Missing GEMINI_API_KEY");
    return false;
  }
  const char *b64 = StripImagePrefix(image);
  if (!b64 || b64[0] == '\0') {
    if (err && err_sz > 0)
      snprintf(err, err_sz, "Missing image data");
    return false;
  }
  char *prompt_esc = JsonEscape(prompt);
  char *model_esc = JsonEscape(model);
  if (!prompt_esc || !model_esc) {
    free(prompt_esc);
    free(model_esc);
    if (err && err_sz > 0)
      snprintf(err, err_sz, "prompt escape failed");
    return false;
  }
  size_t body_sz = strlen(prompt_esc) + strlen(b64) + 512;
  char *body = (char *)malloc(body_sz);
  if (!body) {
    free(prompt_esc);
    free(model_esc);
    return false;
  }
  snprintf(body, body_sz,
           "{\"contents\":[{\"role\":\"user\","
           "\"parts\":[{\"text\":\"%s\"},"
           "{\"inline_data\":{\"mime_type\":\"image/png\",\"data\":\"%s\"}}]}]}",
           prompt_esc, b64);
  char url[512];
  snprintf(url, sizeof(url),
           "https://generativelanguage.googleapis.com/v1beta/models/%s:generateContent?key=%s",
           model_esc, api_key);
  char *resp = NULL;
  bool ok = HttpPostJson(url, body, NULL, &resp, err, err_sz);
  if (!ok && err && strncmp(err, "http 404", 8) == 0) {
    err[0] = '\0';
    snprintf(url, sizeof(url),
             "https://generativelanguage.googleapis.com/v1/models/%s:generateContent?key=%s",
             model_esc, api_key);
    ok = HttpPostJson(url, body, NULL, &resp, err, err_sz);
  }
  free(body);
  free(prompt_esc);
  free(model_esc);
  if (!ok)
    return false;
  char text[4096] = {0};
  bool found = JsonFindString(resp, "text", text, sizeof(text));
  if (!found) {
    free(resp);
    if (err && err_sz > 0)
      snprintf(err, err_sz, "parse failed");
    return false;
  }
  *out_text = strdup(text);
  free(resp);
  return *out_text != NULL;
}

void HandleAiAnalyze(const HttpRequest *req, HttpResponse *res) {
  if (!req || !res)
    return;
  if (!req->body || req->body_len == 0) {
    RespondError(res, 400, "missing body");
    return;
  }
  char provider[32] = {0};
  char model[64] = {0};
  if (!JsonFindString(req->body, "provider", provider, sizeof(provider))) {
    RespondError(res, 400, "missing provider");
    return;
  }
  if (!JsonFindString(req->body, "model", model, sizeof(model))) {
    RespondError(res, 400, "missing model");
    return;
  }

  char *prompt = JsonExtractStringAlloc(req->body, "prompt");
  if (!prompt) {
    RespondError(res, 400, "missing prompt");
    return;
  }
  char *base_url = JsonExtractStringAlloc(req->body, "base_url");
  char *api_key = JsonExtractStringAlloc(req->body, "api_key");
  char *image = JsonExtractStringAlloc(req->body, "image");

  char err[128] = {0};
  char *text = NULL;
  bool ok = false;
  if (strcmp(provider, "local") == 0 || strcmp(provider, "openai") == 0) {
    ok = SendLocal(model, prompt, base_url, api_key, &text, err, sizeof(err));
    if (!ok)
      RespondError(res, 502, err[0] ? err : "upstream failed");
  } else if (strcmp(provider, "gemini") == 0) {
    const char *key = (api_key && api_key[0] != '\0') ? api_key : getenv("GEMINI_API_KEY");
    ok = SendGemini(model, prompt, image, key, &text, err, sizeof(err));
    if (!ok) {
      int status = strstr(err, "Missing GEMINI_API_KEY") ? 401 : 502;
      RespondError(res, status, err[0] ? err : "upstream failed");
    }
  } else {
    RespondError(res, 400, "unknown provider");
  }

  if (ok && text) {
    char *esc = JsonEscape(text);
    if (!esc) {
      RespondError(res, 500, "encode failed");
    } else {
      size_t sz = strlen(esc) + 64;
      res->status = 200;
      res->content_type = "application/json";
      res->body = (char *)malloc(sz);
      if (res->body)
        snprintf(res->body, sz,
                 "{\"status\":\"success\",\"text\":\"%s\"}", esc);
      free(esc);
    }
  }

  free(prompt);
  free(base_url);
  free(api_key);
  free(image);
  free(text);
}

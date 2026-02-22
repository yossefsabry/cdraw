#include "ai_client.h"
#include "ai_base64.h"
#include "ai_http.h"
#include "ai_image.h"
#include "ai_json.h"
#include "ai_prompt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool AiDebugEnabled(void) {
  const char *val = getenv("CDRAW_AI_DEBUG");
  return val && val[0] != '\0' && strcmp(val, "0") != 0;
}

static void MaskKeyValue(const char *key, char *out, size_t out_sz) {
  if (!out || out_sz == 0)
    return;
  out[0] = '\0';
  if (!key || key[0] == '\0')
    return;
  size_t len = strlen(key);
  const char *tail = key + (len > 4 ? len - 4 : 0);
  snprintf(out, out_sz, "****%s", tail);
}

static void RedactUrlKey(const char *url, char *out, size_t out_sz) {
  if (!out || out_sz == 0)
    return;
  out[0] = '\0';
  if (!url) {
    return;
  }
  const char *key = strstr(url, "key=");
  if (!key) {
    snprintf(out, out_sz, "%s", url);
    return;
  }
  size_t prefix_len = (size_t)(key - url) + 4;
  if (prefix_len >= out_sz) {
    snprintf(out, out_sz, "%s", url);
    return;
  }
  memcpy(out, url, prefix_len);
  out[prefix_len] = '\0';
  strncat(out, "REDACTED", out_sz - prefix_len - 1);
}

static void DebugPrintSettings(const AiSettings *settings) {
  if (!AiDebugEnabled() || !settings)
    return;
  char masked[32];
  MaskKeyValue(settings->api_key, masked, sizeof(masked));
  fprintf(stderr,
          "AI DEBUG: provider=%s model=%s key=%s base=%s\n",
          AiProviderLabel(settings->provider),
          settings->model[0] ? settings->model : "(empty)",
          masked[0] ? masked : "(empty)",
          settings->base_url[0] ? settings->base_url : "(empty)");
}

static void DebugPrintLocalRequest(const char *url, const char *body) {
  if (!AiDebugEnabled())
    return;
  size_t body_len = body ? strlen(body) : 0;
  fprintf(stderr, "AI DEBUG: local url=%s body_bytes=%zu\n",
          url ? url : "(null)", body_len);
  if (body && body[0] != '\0') {
    char snippet[256];
    size_t take = body_len < sizeof(snippet) - 1 ? body_len
                                                 : sizeof(snippet) - 1;
    memcpy(snippet, body, take);
    snippet[take] = '\0';
    fprintf(stderr, "AI DEBUG: local body_snippet=%s\n", snippet);
  }
}

static void DebugPrintGeminiRequest(const char *url,
                                    size_t prompt_len,
                                    size_t png_sz,
                                    size_t body_sz) {
  if (!AiDebugEnabled())
    return;
  char redacted[512];
  RedactUrlKey(url, redacted, sizeof(redacted));
  fprintf(stderr,
          "AI DEBUG: gemini url=%s prompt_bytes=%zu png_bytes=%zu body_bytes=%zu\n",
          redacted[0] ? redacted : "(null)",
          prompt_len, png_sz, body_sz);
}

static void DebugPrintResponse(const char *resp) {
  if (!AiDebugEnabled())
    return;
  size_t len = resp ? strlen(resp) : 0;
  fprintf(stderr, "AI DEBUG: response_bytes=%zu\n", len);
  if (resp && resp[0] != '\0') {
    char snippet[256];
    size_t take = len < sizeof(snippet) - 1 ? len
                                            : sizeof(snippet) - 1;
    memcpy(snippet, resp, take);
    snippet[take] = '\0';
    fprintf(stderr, "AI DEBUG: response_snippet=%s\n", snippet);
  }
}

static bool ExtractTextGemini(const char *json,
                              char *out,
                              size_t out_sz) {
  return AiJsonFindString(json, "text", out, out_sz);
}

static bool ExtractTextOpenAI(const char *json,
                              char *out,
                              size_t out_sz) {
  return AiJsonFindString(json, "content", out, out_sz);
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
    if (next == '\0' || next == '/' || next == '?' ||
        next == '#')
      return true;
    p += 3;
  }
  return false;
}

static void StripTrailingSlash(const char *src,
                               char *out,
                               size_t out_sz) {
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

static void BuildLocalUrl(const char *base,
                          char *out,
                          size_t out_sz) {
  if (!out || out_sz == 0)
    return;
  out[0] = '\0';
  if (!base || base[0] == '\0')
    base = "http://localhost:11434/v1";

  char clean[512];
  StripTrailingSlash(base, clean, sizeof(clean));
  if (clean[0] == '\0')
    strncpy(clean, "http://localhost:11434/v1",
            sizeof(clean) - 1);
  clean[sizeof(clean) - 1] = '\0';

  if (HasChatCompletions(clean)) {
    size_t n = strnlen(clean, out_sz - 1);
    memcpy(out, clean, n);
    out[n] = '\0';
    return;
  }

  const char *suffix = HasV1Segment(clean)
                           ? "/chat/completions"
                           : "/v1/chat/completions";
  snprintf(out, out_sz, "%s%s", clean, suffix);
}

bool AiAnalyzeCanvas(const Canvas *canvas,
                     const AiSettings *settings,
                     char *out,
                     size_t out_sz,
                     char *err,
                     size_t err_sz) {
  if (!canvas || !settings || !out || out_sz == 0)
    return false;

  char prompt[2048];
  AiBuildPrompt(canvas, prompt, sizeof(prompt));
  char *prompt_esc = AiJsonEscape(prompt);
  if (!prompt_esc)
    return false;

  DebugPrintSettings(settings);

  char resp[8192];
  bool ok = false;

  if (settings->provider == AI_PROVIDER_GEMINI) {
    unsigned char *png = NULL;
    size_t png_sz = 0;
    if (!AiCapturePng(canvas, &png, &png_sz,
                      err, err_sz)) {
      free(prompt_esc);
      return false;
    }
    size_t b64_sz = 0;
    char *b64 = AiBase64Encode(png, png_sz,
                               &b64_sz);
    free(png);
    if (!b64) {
      free(prompt_esc);
      return false;
    }
    size_t body_sz = strlen(prompt_esc) +
                     b64_sz + 512;
    char *body = (char *)malloc(body_sz);
    if (!body) {
      free(prompt_esc);
      free(b64);
      return false;
    }
    snprintf(body, body_sz,
             "{\"contents\":[{\"role\":\"user\","
             "\"parts\":[{\"text\":\"%s\"},"
             "{\"inline_data\":{\"mime_type\":"
             "\"image/png\",\"data\":\"%s\"}}]}]}",
             prompt_esc, b64);
    char url[512];
    snprintf(url, sizeof(url),
             "https://generativelanguage.googleapis.com/"
             "v1beta/models/%s:"
             "generateContent?key=%s",
             settings->model, settings->api_key);
    DebugPrintGeminiRequest(url, strlen(prompt_esc), png_sz, body_sz);
    ok = AiHttpPostJson(url, body, NULL,
                        resp, sizeof(resp),
                        err, err_sz);
    if (AiDebugEnabled() && resp[0] != '\0')
      DebugPrintResponse(resp);
    if (!ok && err && strncmp(err, "http 404", 8) == 0) {
      err[0] = '\0';
      char url_v1[512];
      snprintf(url_v1, sizeof(url_v1),
               "https://generativelanguage.googleapis.com/"
               "v1/models/%s:"
               "generateContent?key=%s",
               settings->model, settings->api_key);
      DebugPrintGeminiRequest(url_v1, strlen(prompt_esc), png_sz, body_sz);
      ok = AiHttpPostJson(url_v1, body, NULL,
                          resp, sizeof(resp),
                          err, err_sz);
      if (AiDebugEnabled() && resp[0] != '\0')
        DebugPrintResponse(resp);
    }
    free(body);
    free(b64);
    if (ok)
      ok = ExtractTextGemini(resp, out, out_sz);
  } else {
    size_t body_sz = strlen(prompt_esc) +
                     512;
    char *body = (char *)malloc(body_sz);
    if (!body) {
      free(prompt_esc);
      return false;
    }
    snprintf(body, body_sz,
             "{\"model\":\"%s\",\"messages\":["
             "{\"role\":\"system\",\"content\":"
             "\"You analyze sketches and return bullets.\"},"
             "{\"role\":\"user\",\"content\":\"%s\"}],"
             "\"temperature\":0.2}",
             settings->model, prompt_esc);
    char url[512];
    BuildLocalUrl(settings->base_url,
                  url, sizeof(url));
    DebugPrintLocalRequest(url, body);
    ok = AiHttpPostJson(url, body, settings->api_key,
                        resp, sizeof(resp),
                        err, err_sz);
    free(body);
    if (ok)
      DebugPrintResponse(resp);
    if (ok)
      ok = ExtractTextOpenAI(resp, out, out_sz);
  }

  free(prompt_esc);
  if (!ok && err && err_sz > 0 &&
      err[0] == '\0')
    snprintf(err, err_sz, "AI parse failed");
  return ok;
}

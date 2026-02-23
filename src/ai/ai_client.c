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

static void DebugPrintBackendRequest(const char *provider,
                                     const char *url,
                                     const char *body) {
  if (!AiDebugEnabled())
    return;
  size_t body_len = body ? strlen(body) : 0;
  fprintf(stderr,
          "AI DEBUG: backend provider=%s url=%s body_bytes=%zu\n",
          provider ? provider : "(null)",
          url ? url : "(null)", body_len);
}

static bool ExtractTextBackend(const char *json,
                               char *out,
                               size_t out_sz) {
  return AiJsonFindString(json, "text", out, out_sz);
}

static bool ExtractErrorBackend(const char *json,
                                char *out,
                                size_t out_sz) {
  return AiJsonFindString(json, "message", out, out_sz);
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

static void BuildBackendUrl(const char *path,
                            char *out,
                            size_t out_sz) {
  if (!out || out_sz == 0)
    return;
  out[0] = '\0';
  const char *base = getenv("CDRAW_AI_BACKEND_URL");
  if (!base || base[0] == '\0')
    base = "http://127.0.0.1:8900";

  char clean[512];
  StripTrailingSlash(base, clean, sizeof(clean));
  if (clean[0] == '\0')
    strncpy(clean, "http://127.0.0.1:8900",
            sizeof(clean) - 1);
  clean[sizeof(clean) - 1] = '\0';

  snprintf(out, out_sz, "%s%s", clean, path ? path : "");
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
  char url[512];
  BuildBackendUrl("/ai/analyze", url, sizeof(url));

  if (settings->provider == AI_PROVIDER_GEMINI) {
    unsigned char *png = NULL;
    size_t png_sz = 0;
    if (!AiCapturePng(canvas, &png, &png_sz,
                      err, err_sz)) {
      free(prompt_esc);
      return false;
    }
    size_t b64_sz = 0;
    char *b64 = AiBase64Encode(png, png_sz, &b64_sz);
    free(png);
    if (!b64) {
      free(prompt_esc);
      return false;
    }

    char *model_esc = AiJsonEscape(settings->model);
    char *key_esc = AiJsonEscape(settings->api_key);
    if (!model_esc || !key_esc) {
      free(prompt_esc);
      free(b64);
      free(model_esc);
      free(key_esc);
      return false;
    }

    size_t body_sz = strlen(prompt_esc) +
                     strlen(model_esc) +
                     strlen(key_esc) +
                     b64_sz + 512;
    char *body = (char *)malloc(body_sz);
    if (!body) {
      free(prompt_esc);
      free(b64);
      free(model_esc);
      free(key_esc);
      return false;
    }
    snprintf(body, body_sz,
             "{\"provider\":\"gemini\",\"model\":\"%s\","
             "\"prompt\":\"%s\",\"image\":\"data:image/png;base64,%s\","
             "\"api_key\":\"%s\"}",
             model_esc, prompt_esc, b64, key_esc);
    DebugPrintBackendRequest("gemini", url, body);
    ok = AiHttpPostJson(url, body, NULL,
                        resp, sizeof(resp),
                        err, err_sz);
    free(body);
    free(b64);
    free(model_esc);
    free(key_esc);
    if (resp[0] != '\0')
      DebugPrintResponse(resp);
    if (!ok && err && err_sz > 0) {
      char msg[256];
      if (ExtractErrorBackend(resp, msg, sizeof(msg)))
        snprintf(err, err_sz, "%s", msg);
    }
    if (ok)
      ok = ExtractTextBackend(resp, out, out_sz);
  } else {
    char *model_esc = AiJsonEscape(settings->model);
    char *prompt_copy = prompt_esc;
    char *base_esc = AiJsonEscape(settings->base_url);
    char *key_esc = AiJsonEscape(settings->api_key);
    if (!model_esc || !base_esc || !key_esc) {
      free(model_esc);
      free(base_esc);
      free(key_esc);
      return false;
    }
    size_t body_sz = strlen(prompt_copy) +
                     strlen(model_esc) +
                     strlen(base_esc) +
                     strlen(key_esc) + 512;
    char *body = (char *)malloc(body_sz);
    if (!body) {
      free(model_esc);
      free(base_esc);
      free(key_esc);
      return false;
    }
    snprintf(body, body_sz,
             "{\"provider\":\"local\",\"model\":\"%s\","
             "\"prompt\":\"%s\",\"base_url\":\"%s\","
             "\"api_key\":\"%s\"}",
             model_esc, prompt_copy, base_esc, key_esc);
    DebugPrintBackendRequest("local", url, body);
    ok = AiHttpPostJson(url, body, NULL,
                        resp, sizeof(resp),
                        err, err_sz);
    free(body);
    free(model_esc);
    free(base_esc);
    free(key_esc);
    if (resp[0] != '\0')
      DebugPrintResponse(resp);
    if (!ok && err && err_sz > 0) {
      char msg[256];
      if (ExtractErrorBackend(resp, msg, sizeof(msg)))
        snprintf(err, err_sz, "%s", msg);
    }
    if (ok)
      ok = ExtractTextBackend(resp, out, out_sz);
  }

  free(prompt_esc);
  if (!ok && err && err_sz > 0 &&
      err[0] == '\0')
    snprintf(err, err_sz, "AI parse failed");
  return ok;
}

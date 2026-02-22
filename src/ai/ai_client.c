#include "ai_client.h"
#include "ai_base64.h"
#include "ai_http.h"
#include "ai_image.h"
#include "ai_json.h"
#include "ai_prompt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void BuildLocalUrl(const char *base,
                          char *out,
                          size_t out_sz) {
  if (!out || out_sz == 0)
    return;
  out[0] = '\0';
  if (!base || base[0] == '\0')
    base = "http://localhost:11434/v1";
  if (strstr(base, "/chat/completions")) {
    size_t n = strnlen(base, out_sz - 1);
    memcpy(out, base, n);
    out[n] = '\0';
    return;
  }
  const char *suffix = "/chat/completions";
  size_t base_len = strlen(base);
  size_t suffix_len = strlen(suffix);
  if (base_len + suffix_len + 1 > out_sz) {
    size_t n = out_sz - 1;
    if (n > 0) {
      memcpy(out, base, n);
      out[n] = '\0';
    }
    return;
  }
  memcpy(out, base, base_len);
  memcpy(out + base_len, suffix, suffix_len + 1);
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
    ok = AiHttpPostJson(url, body, NULL,
                        resp, sizeof(resp),
                        err, err_sz);
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
    ok = AiHttpPostJson(url, body, settings->api_key,
                        resp, sizeof(resp),
                        err, err_sz);
    free(body);
    if (ok)
      ok = ExtractTextOpenAI(resp, out, out_sz);
  }

  free(prompt_esc);
  if (!ok && err && err_sz > 0 &&
      err[0] == '\0')
    snprintf(err, err_sz, "AI parse failed");
  return ok;
}

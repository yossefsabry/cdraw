#include "ai_settings.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void StrCopy(char *dst, size_t size, const char *src) {
  if (!dst || size == 0)
    return;
  if (!src) {
    dst[0] = '\0';
    return;
  }
  size_t len = strlen(src);
  if (len >= size)
    len = size - 1;
  memcpy(dst, src, len);
  dst[len] = '\0';
}

static char *Trim(char *s) {
  if (!s)
    return NULL;
  while (*s && isspace((unsigned char)*s))
    s++;
  char *end = s + strlen(s);
  while (end > s && isspace((unsigned char)end[-1]))
    end--;
  *end = '\0';
  return s;
}

static int StrEqI(const char *a, const char *b) {
  if (!a || !b)
    return 0;
  while (*a && *b) {
    int da = tolower((unsigned char)*a++);
    int db = tolower((unsigned char)*b++);
    if (da != db)
      return 0;
  }
  return *a == '\0' && *b == '\0';
}

static void Defaults(AiSettings *out) {
  if (!out)
    return;
  out->provider = AI_PROVIDER_GEMINI;
  StrCopy(out->model, sizeof(out->model),
          "gemini-1.5-flash");
  out->api_key[0] = '\0';
  StrCopy(out->base_url, sizeof(out->base_url),
          "http://localhost:11434/v1");
}

bool AiSettingsPath(char *out, size_t out_sz) {
  if (!out || out_sz == 0)
    return false;
#if defined(_WIN32)
  const char *base = getenv("APPDATA");
  if (base && base[0] != '\0') {
    snprintf(out, out_sz,
             "%s\\cdraw\\ai.cfg", base);
    return true;
  }
#else
  const char *xdg = getenv("XDG_CONFIG_HOME");
  if (xdg && xdg[0] != '\0') {
    snprintf(out, out_sz,
             "%s/cdraw/ai.cfg", xdg);
    return true;
  }
  const char *home = getenv("HOME");
  if (home && home[0] != '\0') {
    snprintf(out, out_sz,
             "%s/.config/cdraw/ai.cfg", home);
    return true;
  }
#endif
  snprintf(out, out_sz, "cdraw_ai.cfg");
  return true;
}

const char *AiProviderLabel(AiProvider provider) {
  switch (provider) {
  case AI_PROVIDER_LOCAL:
    return "Local";
  case AI_PROVIDER_GEMINI:
  default:
    return "Gemini";
  }
}

static void ApplyEnv(const char *name, char *dst, size_t size) {
  const char *val = getenv(name);
  if (val && val[0] != '\0')
    StrCopy(dst, size, val);
}

static void ParseLine(AiSettings *out, char *line) {
  char *eq = strchr(line, '=');
  if (!eq)
    return;
  *eq = '\0';
  char *key = Trim(line);
  char *val = Trim(eq + 1);
  if (StrEqI(key, "provider")) {
    if (StrEqI(val, "local") ||
        StrEqI(val, "openai"))
      out->provider = AI_PROVIDER_LOCAL;
    else if (StrEqI(val, "gemini"))
      out->provider = AI_PROVIDER_GEMINI;
  } else if (StrEqI(key, "model")) {
    StrCopy(out->model, sizeof(out->model), val);
  } else if (StrEqI(key, "apikey")) {
    StrCopy(out->api_key, sizeof(out->api_key), val);
  } else if (StrEqI(key, "baseurl")) {
    StrCopy(out->base_url, sizeof(out->base_url), val);
  }
}

bool AiSettingsLoad(AiSettings *out, char *err, size_t err_sz) {
  if (!out)
    return false;
  Defaults(out);

  char path[512];
  if (!AiSettingsPath(path, sizeof(path))) {
    if (err && err_sz > 0)
      snprintf(err, err_sz, "AI settings path failed.");
    return false;
  }

  FILE *f = fopen(path, "rb");
  if (f) {
    char line[256];
    while (fgets(line, (int)sizeof(line), f)) {
      char *s = Trim(line);
      if (s[0] == '\0' || s[0] == '#' || s[0] == ';')
        continue;
      ParseLine(out, s);
    }
    fclose(f);
  }

  if (getenv("CDRAW_AI_PROVIDER")) {
    const char *p = getenv("CDRAW_AI_PROVIDER");
    if (p && p[0] != '\0') {
      if (StrEqI(p, "local") ||
          StrEqI(p, "openai"))
        out->provider = AI_PROVIDER_LOCAL;
      else if (StrEqI(p, "gemini"))
        out->provider = AI_PROVIDER_GEMINI;
    }
  }
  ApplyEnv("CDRAW_AI_MODEL", out->model,
           sizeof(out->model));
  ApplyEnv("CDRAW_AI_KEY", out->api_key,
           sizeof(out->api_key));
  ApplyEnv("CDRAW_AI_BASE_URL", out->base_url,
           sizeof(out->base_url));

  if (out->provider == AI_PROVIDER_LOCAL) {
    if (out->model[0] == '\0')
      StrCopy(out->model, sizeof(out->model), "llama3");
    if (out->base_url[0] == '\0')
      StrCopy(out->base_url, sizeof(out->base_url),
              "http://localhost:11434/v1");
    return true;
  }

  if (out->api_key[0] == '\0') {
    if (err && err_sz > 0)
      snprintf(err, err_sz, "Missing apiKey in ai.cfg");
    return false;
  }
  if (out->model[0] == '\0') {
    StrCopy(out->model, sizeof(out->model),
            "gemini-1.5-flash");
  }
  return true;
}

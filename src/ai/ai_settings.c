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
          "gemini-1.5-flash-latest");
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

  bool loadedFile = false;
  FILE *f = fopen(path, "rb");
  if (f) {
    loadedFile = true;
    char line[256];
    while (fgets(line, (int)sizeof(line), f)) {
      char *s = Trim(line);
      if (s[0] == '\0' || s[0] == '#' || s[0] == ';')
        continue;
      ParseLine(out, s);
    }
    fclose(f);
  }

  bool envProvider = false;
  if (getenv("CDRAW_AI_PROVIDER")) {
    const char *p = getenv("CDRAW_AI_PROVIDER");
    if (p && p[0] != '\0') {
      envProvider = true;
      if (StrEqI(p, "local") ||
          StrEqI(p, "openai"))
        out->provider = AI_PROVIDER_LOCAL;
      else if (StrEqI(p, "gemini"))
        out->provider = AI_PROVIDER_GEMINI;
    }
  }
  bool envModel = false;
  bool envKey = false;
  bool envBase = false;
  if (getenv("CDRAW_AI_MODEL")) {
    const char *v = getenv("CDRAW_AI_MODEL");
    if (v && v[0] != '\0')
      envModel = true;
  }
  if (getenv("CDRAW_AI_KEY")) {
    const char *v = getenv("CDRAW_AI_KEY");
    if (v && v[0] != '\0')
      envKey = true;
  }
  if (getenv("CDRAW_AI_BASE_URL")) {
    const char *v = getenv("CDRAW_AI_BASE_URL");
    if (v && v[0] != '\0')
      envBase = true;
  }
  ApplyEnv("CDRAW_AI_MODEL", out->model,
           sizeof(out->model));
  ApplyEnv("CDRAW_AI_KEY", out->api_key,
           sizeof(out->api_key));
  ApplyEnv("CDRAW_AI_BASE_URL", out->base_url,
           sizeof(out->base_url));

  if (AiDebugEnabled()) {
    char masked[32];
    MaskKeyValue(out->api_key, masked, sizeof(masked));
    fprintf(stderr,
            "AI DEBUG: settings path=%s file=%s env_provider=%d env_model=%d env_key=%d env_base=%d\n",
            path, loadedFile ? "yes" : "no", envProvider, envModel, envKey, envBase);
    fprintf(stderr,
            "AI DEBUG: settings provider=%s model=%s key=%s base=%s\n",
            AiProviderLabel(out->provider),
            out->model[0] ? out->model : "(empty)",
            masked[0] ? masked : "(empty)",
            out->base_url[0] ? out->base_url : "(empty)");
  }

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
            "gemini-1.5-flash-latest");
  }
  return true;
}

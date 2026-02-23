#include "ai_settings.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MKDIR(path) mkdir(path, 0755)
#endif

static int MkdirIfMissing(const char *path) {
  if (!path || path[0] == '\0')
    return -1;
  if (MKDIR(path) == 0)
    return 0;
  if (errno == EEXIST)
    return 0;
  return -1;
}

static void EnsureDirs(void) {
#if defined(_WIN32)
  const char *base = getenv("APPDATA");
  if (!base || base[0] == '\0')
    return;
  char dir[512];
  snprintf(dir, sizeof(dir), "%s\\cdraw", base);
  (void)MkdirIfMissing(dir);
#else
  const char *xdg = getenv("XDG_CONFIG_HOME");
  if (xdg && xdg[0] != '\0') {
    char dir[512];
    snprintf(dir, sizeof(dir), "%s/cdraw", xdg);
    (void)MkdirIfMissing(xdg);
    (void)MkdirIfMissing(dir);
    return;
  }
  const char *home = getenv("HOME");
  if (!home || home[0] == '\0')
    return;
  char cfg[512];
  char dir[512];
  snprintf(cfg, sizeof(cfg), "%s/.config", home);
  snprintf(dir, sizeof(dir), "%s/.config/cdraw", home);
  (void)MkdirIfMissing(cfg);
  (void)MkdirIfMissing(dir);
#endif
}

static const char *ProviderName(AiProvider p) {
  if (p == AI_PROVIDER_LOCAL)
    return "local";
  if (p == AI_PROVIDER_OPENAI)
    return "openai";
  return "gemini";
}

bool AiSettingsSave(const AiSettings *s,
                    char *err,
                    size_t err_sz) {
  if (!s)
    return false;
  EnsureDirs();
  char path[512];
  if (!AiSettingsPath(path, sizeof(path))) {
    if (err && err_sz > 0)
      snprintf(err, err_sz, "settings path failed");
    return false;
  }
  FILE *f = fopen(path, "wb");
  if (!f) {
    if (err && err_sz > 0)
      snprintf(err, err_sz, "save failed");
    return false;
  }
  fprintf(f, "CDRAW_AI1\n");
  fprintf(f, "provider=%s\n", ProviderName(s->provider));
  fprintf(f, "model=%s\n", s->model);
  fprintf(f, "apiKey=%s\n", s->api_key);
  fprintf(f, "baseUrl=%s\n", s->base_url);
  fclose(f);
  return true;
}

bool AiSettingsClear(char *err, size_t err_sz) {
  char path[512];
  if (!AiSettingsPath(path, sizeof(path))) {
    if (err && err_sz > 0)
      snprintf(err, err_sz, "settings path failed");
    return false;
  }
  if (remove(path) != 0) {
    if (err && err_sz > 0)
      snprintf(err, err_sz, "clear failed");
    return false;
  }
  return true;
}

bool AiSettingsReady(const AiSettings *s,
                     char *err,
                     size_t err_sz) {
  if (!s)
    return false;
  if (s->model[0] == '\0') {
    if (err && err_sz > 0)
      snprintf(err, err_sz, "Missing model");
    return false;
  }
  if (s->provider == AI_PROVIDER_GEMINI) {
    if (s->api_key[0] == '\0') {
      if (err && err_sz > 0)
        snprintf(err, err_sz, "Missing apiKey");
      return false;
    }
  } else if (s->provider == AI_PROVIDER_OPENAI) {
    if (s->api_key[0] == '\0') {
      if (err && err_sz > 0)
        snprintf(err, err_sz, "Missing apiKey");
      return false;
    }
    if (s->base_url[0] == '\0') {
      if (err && err_sz > 0)
        snprintf(err, err_sz, "Missing baseUrl");
      return false;
    }
  } else {
    if (s->base_url[0] == '\0') {
      if (err && err_sz > 0)
        snprintf(err, err_sz, "Missing baseUrl");
      return false;
    }
  }
  return true;
}

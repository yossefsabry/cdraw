#include "prefs.h"
#include <ctype.h>
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
  if (path == NULL || path[0] == '\0')
    return -1;
  if (MKDIR(path) == 0)
    return 0;
#if defined(_WIN32)
  if (errno == EEXIST)
    return 0;
#else
  if (errno == EEXIST)
    return 0;
#endif
  return -1;
}

static bool PrefsBuildPath(char *outPath, size_t outSize) {
  if (outPath == NULL || outSize == 0)
    return false;

#if defined(_WIN32)
  const char *base = getenv("APPDATA");
  if (base && base[0] != '\0') {
    snprintf(outPath, outSize, "%s\\cdraw\\prefs.cfg", base);
    return true;
  }
#else
  const char *xdg = getenv("XDG_CONFIG_HOME");
  if (xdg && xdg[0] != '\0') {
    snprintf(outPath, outSize, "%s/cdraw/prefs.cfg", xdg);
    return true;
  }
  const char *home = getenv("HOME");
  if (home && home[0] != '\0') {
    snprintf(outPath, outSize, "%s/.config/cdraw/prefs.cfg", home);
    return true;
  }
#endif

  snprintf(outPath, outSize, "cdraw_prefs.cfg");
  return true;
}

static void PrefsEnsureDirs(void) {
#if defined(_WIN32)
  const char *base = getenv("APPDATA");
  if (!base || base[0] == '\0')
    return;

  char appDir[512];
  snprintf(appDir, sizeof(appDir), "%s\\cdraw", base);
  (void)MkdirIfMissing(appDir);
#else
  const char *xdg = getenv("XDG_CONFIG_HOME");
  if (xdg && xdg[0] != '\0') {
    char appDir[512];
    snprintf(appDir, sizeof(appDir), "%s/cdraw", xdg);
    (void)MkdirIfMissing(xdg);
    (void)MkdirIfMissing(appDir);
    return;
  }

  const char *home = getenv("HOME");
  if (!home || home[0] == '\0')
    return;
  char cfgDir[512];
  snprintf(cfgDir, sizeof(cfgDir), "%s/.config", home);
  char appDir[512];
  snprintf(appDir, sizeof(appDir), "%s/.config/cdraw", home);
  (void)MkdirIfMissing(cfgDir);
  (void)MkdirIfMissing(appDir);
#endif
}

static char *Trim(char *s) {
  if (s == NULL)
    return NULL;
  while (*s && isspace((unsigned char)*s))
    s++;
  char *end = s + strlen(s);
  while (end > s && isspace((unsigned char)end[-1]))
    end--;
  *end = '\0';
  return s;
}

static int StrCaseCmp(const char *a, const char *b) {
  if (a == NULL && b == NULL)
    return 0;
  if (a == NULL)
    return -1;
  if (b == NULL)
    return 1;
  for (;;) {
    unsigned char ca = (unsigned char)*a++;
    unsigned char cb = (unsigned char)*b++;
    int da = tolower(ca);
    int db = tolower(cb);
    if (da != db)
      return da - db;
    if (ca == '\0')
      return 0;
  }
}

static bool ParseBool(const char *s, bool *out) {
  if (!s || !out)
    return false;
  while (*s && isspace((unsigned char)*s))
    s++;
  if (*s == '\0')
    return false;
  if (*s == '1') {
    *out = true;
    return true;
  }
  if (*s == '0') {
    *out = false;
    return true;
  }
  if (StrCaseCmp(s, "true") == 0 || StrCaseCmp(s, "yes") == 0 ||
      StrCaseCmp(s, "on") == 0) {
    *out = true;
    return true;
  }
  if (StrCaseCmp(s, "false") == 0 || StrCaseCmp(s, "no") == 0 ||
      StrCaseCmp(s, "off") == 0) {
    *out = false;
    return true;
  }
  return false;
}

AppPrefs PrefsDefaults(void) {
  return (AppPrefs){
      .darkMode = true,
      .showGrid = true,
      .hasSeenWelcome = false,
  };
}

bool PrefsLoad(AppPrefs *outPrefs) {
  if (!outPrefs)
    return false;

  *outPrefs = PrefsDefaults();

  char path[512];
  if (!PrefsBuildPath(path, sizeof(path)))
    return false;

  FILE *f = fopen(path, "rb");
  if (!f)
    return false;

  char line[256];
  bool firstLine = true;
  while (fgets(line, (int)sizeof(line), f)) {
    char *s = Trim(line);
    if (s[0] == '\0')
      continue;
    if (s[0] == '#' || s[0] == ';')
      continue;

    if (firstLine && strncmp(s, "CDRAW_PREFS", 11) == 0) {
      firstLine = false;
      continue;
    }
    firstLine = false;

    char *eq = strchr(s, '=');
    if (!eq)
      continue;
    *eq = '\0';
    char *key = Trim(s);
    char *val = Trim(eq + 1);

    bool b;
    if (strcmp(key, "darkMode") == 0) {
      if (ParseBool(val, &b))
        outPrefs->darkMode = b;
    } else if (strcmp(key, "showGrid") == 0) {
      if (ParseBool(val, &b))
        outPrefs->showGrid = b;
    } else if (strcmp(key, "hasSeenWelcome") == 0) {
      if (ParseBool(val, &b))
        outPrefs->hasSeenWelcome = b;
    }
  }

  fclose(f);
  return true;
}

bool PrefsSave(const AppPrefs *prefs) {
  if (!prefs)
    return false;

  PrefsEnsureDirs();

  char path[512];
  if (!PrefsBuildPath(path, sizeof(path)))
    return false;

  FILE *f = fopen(path, "wb");
  if (!f)
    return false;

  fprintf(f, "CDRAW_PREFS1\n");
  fprintf(f, "darkMode=%d\n", prefs->darkMode ? 1 : 0);
  fprintf(f, "showGrid=%d\n", prefs->showGrid ? 1 : 0);
  fprintf(f, "hasSeenWelcome=%d\n", prefs->hasSeenWelcome ? 1 : 0);

  fclose(f);
  return true;
}

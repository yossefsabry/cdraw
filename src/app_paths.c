#include "app_paths.h"
#include "raylib.h"
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static void CopyString(char *dst, size_t size, const char *src) {
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

static void JoinPath(char *out, size_t size, const char *root,
                     const char *suffix) {
  if (!out || size == 0)
    return;
  if (!root || root[0] == '\0') {
    CopyString(out, size, suffix ? suffix : "");
    return;
  }
  if (!suffix || suffix[0] == '\0') {
    CopyString(out, size, root);
    return;
  }

  size_t len = strlen(root);
  char sep = '/';
  if (len > 0 && (root[len - 1] == '/' || root[len - 1] == '\\'))
    sep = '\0';

  if (sep == '\0')
    snprintf(out, size, "%s%s", root, suffix);
  else
    snprintf(out, size, "%s%c%s", root, sep, suffix);
}

static bool HasAssetsAt(const char *root) {
  if (!root || root[0] == '\0')
    return false;
  char path[PATH_MAX];
  JoinPath(path, sizeof(path), root, "assets/ui_icons/menu.png");
  if (FileExists(path))
    return true;
  JoinPath(path, sizeof(path), root, "assets/ui_icons/atlas.png");
  return FileExists(path);
}

static bool IsAbsolutePath(const char *path) {
  if (!path || path[0] == '\0')
    return false;
  if (path[0] == '/' || path[0] == '\\')
    return true;
#if defined(_WIN32)
  if (path[1] == ':')
    return true;
#endif
  return false;
}

bool CdrawResolveRoot(char *out, size_t size) {
  if (!out || size == 0)
    return false;

  static bool resolved = false;
  static char cached[PATH_MAX];

  if (!resolved) {
    cached[0] = '\0';
    resolved = true;

    const char *env = getenv("CDRAW_ROOT");
    if (env && env[0] != '\0' && HasAssetsAt(env)) {
      CopyString(cached, sizeof(cached), env);
    } else if (HasAssetsAt(".")) {
      const char *cwd = GetWorkingDirectory();
      if (cwd && cwd[0] != '\0')
        CopyString(cached, sizeof(cached), cwd);
    } else {
      const char *appDir = GetApplicationDirectory();
      if (appDir && appDir[0] != '\0' && HasAssetsAt(appDir)) {
        CopyString(cached, sizeof(cached), appDir);
      } else {
        const char *home = getenv("HOME");
        if (home && home[0] != '\0') {
          char localShare[PATH_MAX];
          snprintf(localShare, sizeof(localShare), "%s/.local/share/cdraw",
                   home);
          if (HasAssetsAt(localShare))
            CopyString(cached, sizeof(cached), localShare);
        }
        if (cached[0] == '\0' && HasAssetsAt("/usr/local/share/cdraw"))
          CopyString(cached, sizeof(cached), "/usr/local/share/cdraw");
        if (cached[0] == '\0' && HasAssetsAt("/usr/share/cdraw"))
          CopyString(cached, sizeof(cached), "/usr/share/cdraw");
      }
    }
  }

  if (cached[0] == '\0') {
    out[0] = '\0';
    return false;
  }

  CopyString(out, size, cached);
  return true;
}

const char *CdrawAssetPath(char *out, size_t size, const char *relative) {
  if (!out || size == 0)
    return "";
  if (!relative || relative[0] == '\0') {
    out[0] = '\0';
    return out;
  }
  if (IsAbsolutePath(relative)) {
    CopyString(out, size, relative);
    return out;
  }

  char root[PATH_MAX];
  if (CdrawResolveRoot(root, sizeof(root)))
    JoinPath(out, size, root, relative);
  else
    CopyString(out, size, relative);

  return out;
}

bool CdrawBackendPath(char *out, size_t size) {
  if (!out || size == 0)
    return false;

  const char *env = getenv("CDRAW_ROOT");
  if (env && env[0] != '\0') {
    JoinPath(out, size, env, "backend_ai/backend_ai");
    if (FileExists(out))
      return true;
  }

  JoinPath(out, size, ".", "backend_ai/backend_ai");
  if (FileExists(out))
    return true;

  const char *appDir = GetApplicationDirectory();
  if (appDir && appDir[0] != '\0') {
    JoinPath(out, size, appDir, "backend_ai/backend_ai");
    if (FileExists(out))
      return true;
  }

  const char *home = getenv("HOME");
  if (home && home[0] != '\0') {
    char localShare[PATH_MAX];
    snprintf(localShare, sizeof(localShare), "%s/.local/share/cdraw", home);
    JoinPath(out, size, localShare, "backend_ai/backend_ai");
    if (FileExists(out))
      return true;
  }

  JoinPath(out, size, "/usr/local/share/cdraw", "backend_ai/backend_ai");
  if (FileExists(out))
    return true;

  JoinPath(out, size, "/usr/share/cdraw", "backend_ai/backend_ai");
  if (FileExists(out))
    return true;

  out[0] = '\0';
  return false;
}

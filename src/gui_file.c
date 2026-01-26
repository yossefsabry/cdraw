#include "gui_internal.h"
#include "nfd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *kCdrawExt = ".cdraw";
static const char *kCdrawFilterList = "cdraw";

static bool HasGuiSession(void) {
#ifdef __linux__
  const char *display = getenv("DISPLAY");
  const char *wayland = getenv("WAYLAND_DISPLAY");
  if ((!display || display[0] == '\0') && (!wayland || wayland[0] == '\0'))
    return false;
#endif
  return true;
}

static void CopyPath(char *dst, size_t size, const char *src) {
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

static void EnsureCdrawExtension(char *path, size_t size) {
  size_t len = strlen(path);
  size_t extLen = strlen(kCdrawExt);
  if (len >= extLen) {
    const char *tail = path + (len - extLen);
    if (strcmp(tail, kCdrawExt) == 0)
      return;
  }
  if (len + extLen + 1 >= size)
    return;
  memcpy(path + len, kCdrawExt, extLen + 1);
}

static void UpdateLastDir(GuiState *gui, const char *path) {
  const char *dir = GetDirectoryPath(path);
  if (dir && dir[0] != '\0')
    CopyPath(gui->lastDir, sizeof(gui->lastDir), dir);
}

static void UpdateLastFileName(GuiState *gui, const char *path) {
  const char *name = GetFileName(path);
  if (name && name[0] != '\0')
    CopyPath(gui->lastFileName, sizeof(gui->lastFileName), name);
}

static void JoinPath(char *out, size_t size, const char *dir, const char *name) {
  if (!dir || dir[0] == '\0') {
    snprintf(out, size, "%s", name ? name : "");
    return;
  }
  size_t len = strlen(dir);
  char sep = '/';
  if (len > 0 && (dir[len - 1] == '/' || dir[len - 1] == '\\'))
    sep = '\0';
  if (sep == '\0')
    snprintf(out, size, "%s%s", dir, name ? name : "");
  else
    snprintf(out, size, "%s%c%s", dir, sep, name ? name : "");
}

static void BuildDefaultPath(char *out, size_t size, const char *dir,
                             const char *name) {
  if (!dir || dir[0] == '\0')
    dir = GetWorkingDirectory();
  JoinPath(out, size, dir, name);
}

static void BuildDefaultDir(char *out, size_t size, const char *dir) {
  if (!dir || dir[0] == '\0')
    dir = GetWorkingDirectory();
  if (!dir || dir[0] == '\0') {
    out[0] = '\0';
    return;
  }
  size_t len = strlen(dir);
  if (len > 0 && (dir[len - 1] == '/' || dir[len - 1] == '\\'))
    snprintf(out, size, "%s", dir);
  else
    snprintf(out, size, "%s/", dir);
}

static void ShowDialogError(GuiState *gui, const char *action) {
  const char *err = NFD_GetError();
  if (err && err[0] != '\0') {
    char msg[196];
    snprintf(msg, sizeof(msg), "%s: %s", action, err);
    GuiToastSet(gui, msg);
  } else {
    GuiToastSet(gui, "File dialog failed.");
  }
}

void GuiMarkNewDocument(GuiState *gui) {
  Document *doc = GuiGetActiveDocument(gui);
  if (!doc)
    return;
  doc->hasPath = false;
  doc->path[0] = '\0';
}

void GuiRequestOpen(GuiState *gui) {
  gui->showMenu = false;
  gui->showColorPicker = false;

  if (!HasGuiSession()) {
    GuiToastSet(gui, "No GUI session for file dialogs.");
    return;
  }

  char defaultPath[512];
  BuildDefaultDir(defaultPath, sizeof(defaultPath), gui->lastDir);

  nfdchar_t *path = NULL;
  nfdresult_t result = NFD_OpenDialog(kCdrawFilterList, defaultPath, &path);
  if (result == NFD_OKAY && path) {
    int prevActive = gui->activeDocument;
    Document *prevDoc = GuiGetActiveDocument(gui);
    bool defaultGrid = prevDoc ? prevDoc->canvas.showGrid : true;

    Document *doc = GuiAddDocument(gui, GetScreenWidth(), GetScreenHeight(),
                                   defaultGrid, true);
    if (!doc) {
      GuiToastSet(gui, "Open failed.");
      free(path);
      return;
    }

    bool ok = LoadCanvasFromFile(&doc->canvas, path);
    GuiToastSet(gui, ok ? "Loaded." : "Load failed.");
    if (ok) {
      CopyPath(doc->path, sizeof(doc->path), path);
      doc->hasPath = true;
      UpdateLastDir(gui, path);
      UpdateLastFileName(gui, path);
      free(path);
      return;
    }

    GuiCloseDocument(gui, gui->activeDocument, GetScreenWidth(), GetScreenHeight(),
                     defaultGrid);
    if (prevActive >= 0 && prevActive < gui->documentCount)
      gui->activeDocument = prevActive;
    free(path);
    return;
  }

  if (result == NFD_ERROR)
    ShowDialogError(gui, "Open failed");
}

void GuiRequestSave(GuiState *gui, Document *doc) {
  gui->showMenu = false;
  gui->showColorPicker = false;

  if (!doc) {
    GuiToastSet(gui, "No document.");
    return;
  }

  if (doc->hasPath && doc->path[0] != '\0') {
    bool ok = SaveCanvasToFile(&doc->canvas, doc->path);
    GuiToastSet(gui, ok ? "Saved." : "Save failed.");
    if (ok) {
      UpdateLastDir(gui, doc->path);
      UpdateLastFileName(gui, doc->path);
    }
    return;
  }

  if (!HasGuiSession()) {
    GuiToastSet(gui, "No GUI session for file dialogs.");
    return;
  }

  const char *name = gui->lastFileName[0] != '\0' ? gui->lastFileName
                                                   : "drawing.cdraw";
  char defaultPath[512];
  BuildDefaultPath(defaultPath, sizeof(defaultPath), gui->lastDir, name);

  nfdchar_t *path = NULL;
  nfdresult_t result = NFD_SaveDialog(kCdrawFilterList, defaultPath, &path);
  if (result == NFD_OKAY && path) {
    char resolved[512];
    CopyPath(resolved, sizeof(resolved), path);
    EnsureCdrawExtension(resolved, sizeof(resolved));
    bool ok = SaveCanvasToFile(&doc->canvas, resolved);
    GuiToastSet(gui, ok ? "Saved." : "Save failed.");
    if (ok) {
      CopyPath(doc->path, sizeof(doc->path), resolved);
      doc->hasPath = true;
      UpdateLastDir(gui, resolved);
      UpdateLastFileName(gui, resolved);
    }
    free(path);
    return;
  }

  if (result == NFD_ERROR)
    ShowDialogError(gui, "Save failed");
}

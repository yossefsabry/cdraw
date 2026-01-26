#include "gui_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *kCdrawExt = ".cdraw";

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

static void FileDialogFreeEntries(GuiState *gui) {
  if (gui->fileDialogEntries) {
    free(gui->fileDialogEntries);
    gui->fileDialogEntries = NULL;
  }
  gui->fileDialogEntryCount = 0;
  gui->fileDialogSelected = -1;
  gui->fileDialogScroll = 0;
}

static int EntryCompare(const void *a, const void *b) {
  const FileDialogEntry *ea = (const FileDialogEntry *)a;
  const FileDialogEntry *eb = (const FileDialogEntry *)b;
  if (ea->isUp != eb->isUp)
    return ea->isUp ? -1 : 1;
  if (ea->isDir != eb->isDir)
    return ea->isDir ? -1 : 1;
  return strcmp(ea->name, eb->name);
}

static void FileDialogRefresh(GuiState *gui) {
  FileDialogFreeEntries(gui);

  const char *dir = gui->fileDialogDir[0] ? gui->fileDialogDir : GetWorkingDirectory();
  if (dir && dir[0] != '\0')
    snprintf(gui->fileDialogDir, sizeof(gui->fileDialogDir), "%s", dir);

  FilePathList list = LoadDirectoryFiles(gui->fileDialogDir);

  int count = 0;
  char parent[256] = {0};
  const char *parentPath = GetDirectoryPath(gui->fileDialogDir);
  if (parentPath && parentPath[0] != '\0' && strcmp(parentPath, gui->fileDialogDir) != 0)
    snprintf(parent, sizeof(parent), "%s", parentPath);
  if (parent[0] != '\0')
    count++;

  for (unsigned int i = 0; i < list.count; i++) {
    const char *path = list.paths[i];
    if (DirectoryExists(path)) {
      count++;
      continue;
    }
    if (IsFileExtension(path, kCdrawExt))
      count++;
  }

  if (count == 0) {
    UnloadDirectoryFiles(list);
    return;
  }

  gui->fileDialogEntries = (FileDialogEntry *)calloc((size_t)count, sizeof(FileDialogEntry));
  if (!gui->fileDialogEntries) {
    UnloadDirectoryFiles(list);
    return;
  }

  int index = 0;
  if (parent[0] != '\0') {
    FileDialogEntry *entry = &gui->fileDialogEntries[index++];
    snprintf(entry->path, sizeof(entry->path), "%s", parent);
    snprintf(entry->name, sizeof(entry->name), "%s", "..");
    entry->isDir = true;
    entry->isUp = true;
  }

  for (unsigned int i = 0; i < list.count; i++) {
    const char *path = list.paths[i];
    if (!DirectoryExists(path))
      continue;
    FileDialogEntry *entry = &gui->fileDialogEntries[index++];
    snprintf(entry->path, sizeof(entry->path), "%s", path);
    snprintf(entry->name, sizeof(entry->name), "%s", GetFileName(path));
    entry->isDir = true;
    entry->isUp = false;
  }

  for (unsigned int i = 0; i < list.count; i++) {
    const char *path = list.paths[i];
    if (DirectoryExists(path))
      continue;
    if (!IsFileExtension(path, kCdrawExt))
      continue;
    FileDialogEntry *entry = &gui->fileDialogEntries[index++];
    snprintf(entry->path, sizeof(entry->path), "%s", path);
    snprintf(entry->name, sizeof(entry->name), "%s", GetFileName(path));
    entry->isDir = false;
    entry->isUp = false;
  }

  gui->fileDialogEntryCount = index;
  if (gui->fileDialogEntryCount > 1)
    qsort(gui->fileDialogEntries, (size_t)gui->fileDialogEntryCount,
          sizeof(FileDialogEntry), EntryCompare);

  UnloadDirectoryFiles(list);
}

static void FileDialogSetDirectory(GuiState *gui, const char *dir) {
  if (!dir || dir[0] == '\0')
    return;
  snprintf(gui->fileDialogDir, sizeof(gui->fileDialogDir), "%s", dir);
  gui->fileDialogSelected = -1;
  gui->fileDialogScroll = 0;
  FileDialogRefresh(gui);
}

void GuiFileDialogEnterDirectory(GuiState *gui, const char *dir) {
  FileDialogSetDirectory(gui, dir);
}

static void BeginFileDialog(GuiState *gui, bool saveMode) {
  gui->showFileDialog = true;
  gui->fileDialogIsSave = saveMode;
  gui->showMenu = false;
  gui->showColorPicker = false;
  gui->isTyping = saveMode;

  if (gui->lastDir[0] != '\0')
    FileDialogSetDirectory(gui, gui->lastDir);
  else
    FileDialogSetDirectory(gui, GetWorkingDirectory());

  if (saveMode) {
    if (gui->currentFile[0] != '\0') {
      snprintf(gui->fileDialogName, sizeof(gui->fileDialogName), "%s",
               GetFileName(gui->currentFile));
    } else if (gui->fileDialogName[0] == '\0') {
      snprintf(gui->fileDialogName, sizeof(gui->fileDialogName), "%s", "drawing.cdraw");
    }
  }
}

void GuiMarkNewDocument(GuiState *gui) {
  gui->hasFilePath = false;
  gui->currentFile[0] = '\0';
  gui->fileDialogName[0] = '\0';
}

void GuiRequestOpen(GuiState *gui, Canvas *canvas) {
  (void)canvas;
  BeginFileDialog(gui, false);
}

void GuiRequestSave(GuiState *gui, Canvas *canvas) {
  if (gui->hasFilePath && gui->currentFile[0] != '\0') {
    GuiToastSet(gui, SaveCanvasToFile(canvas, gui->currentFile) ? "Saved."
                                                                : "Save failed.");
    return;
  }
  BeginFileDialog(gui, true);
}

void GuiFileDialogConfirm(GuiState *gui, Canvas *canvas) {
  if (gui->fileDialogIsSave) {
    if (gui->fileDialogName[0] == '\0') {
      GuiToastSet(gui, "Enter a file name.");
      return;
    }
    char path[256];
    JoinPath(path, sizeof(path), gui->fileDialogDir, gui->fileDialogName);
    EnsureCdrawExtension(path, sizeof(path));
    bool ok = SaveCanvasToFile(canvas, path);
    GuiToastSet(gui, ok ? "Saved." : "Save failed.");
    if (ok) {
      snprintf(gui->currentFile, sizeof(gui->currentFile), "%s", path);
      snprintf(gui->lastDir, sizeof(gui->lastDir), "%s", gui->fileDialogDir);
      gui->hasFilePath = true;
      gui->showFileDialog = false;
      gui->isTyping = false;
      FileDialogFreeEntries(gui);
    }
    return;
  }

  if (gui->fileDialogSelected < 0 ||
      gui->fileDialogSelected >= gui->fileDialogEntryCount) {
    GuiToastSet(gui, "Select a file.");
    return;
  }

  FileDialogEntry *entry = &gui->fileDialogEntries[gui->fileDialogSelected];
  if (entry->isDir || entry->isUp) {
    FileDialogSetDirectory(gui, entry->path);
    return;
  }

  bool ok = LoadCanvasFromFile(canvas, entry->path);
  GuiToastSet(gui, ok ? "Loaded." : "Load failed.");
  if (ok) {
    snprintf(gui->currentFile, sizeof(gui->currentFile), "%s", entry->path);
    snprintf(gui->lastDir, sizeof(gui->lastDir), "%s", gui->fileDialogDir);
    gui->hasFilePath = true;
    gui->showFileDialog = false;
    gui->isTyping = false;
    FileDialogFreeEntries(gui);
  }
}

void GuiFileDialogCancel(GuiState *gui) {
  gui->showFileDialog = false;
  gui->isTyping = false;
  FileDialogFreeEntries(gui);
}

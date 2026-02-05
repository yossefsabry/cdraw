#include "gui_internal.h"
#include "nfd.h"
#include "raymath.h"
#include <ctype.h>
#include <float.h>
#include <math.h>
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

static void BuildDownloadsDir(char *out, size_t size) {
#if defined(_WIN32)
  const char *home = getenv("USERPROFILE");
  if (home && home[0] != '\0') {
    snprintf(out, size, "%s\\Downloads", home);
    return;
  }
#else
  const char *home = getenv("HOME");
  if (home && home[0] != '\0') {
    snprintf(out, size, "%s/Downloads", home);
    return;
  }
#endif
  BuildDefaultDir(out, size, NULL);
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

static bool EndsWithExt(const char *path, const char *ext) {
  if (!path || !ext)
    return false;
  size_t len = strlen(path);
  size_t extLen = strlen(ext);
  if (len < extLen)
    return false;
  return StrCaseCmp(path + (len - extLen), ext) == 0;
}

static void EnsureExportExtension(char *path, size_t size, ExportFormat format) {
  if (!path || size == 0)
    return;
  if (format == EXPORT_FORMAT_JPG) {
    if (EndsWithExt(path, ".jpg") || EndsWithExt(path, ".jpeg"))
      return;
    if (strlen(path) + 4 + 1 >= size)
      return;
    strcat(path, ".jpg");
    return;
  }

  const char *ext = (format == EXPORT_FORMAT_SVG) ? ".svg" : ".png";
  if (EndsWithExt(path, ext))
    return;
  if (strlen(path) + strlen(ext) + 1 >= size)
    return;
  strcat(path, ext);
}

static void BuildDefaultExportPath(char *out, size_t size, const char *name,
                                   ExportFormat format) {
  char dir[512];
  BuildDownloadsDir(dir, sizeof(dir));

  const char *base = (name && name[0] != '\0') ? name : "drawing";
  const char *ext = (format == EXPORT_FORMAT_SVG)
                        ? ".svg"
                        : (format == EXPORT_FORMAT_JPG) ? ".jpg" : ".png";

  char filename[256];
  snprintf(filename, sizeof(filename), "%s%s", base, ext);
  JoinPath(out, size, dir, filename);
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

static Rectangle GetCameraViewRect(const Camera2D *camera, int sw, int sh) {
  Vector2 tl = GetScreenToWorld2D((Vector2){0, 0}, *camera);
  Vector2 br =
      GetScreenToWorld2D((Vector2){(float)sw, (float)sh}, *camera);
  float minX = fminf(tl.x, br.x);
  float minY = fminf(tl.y, br.y);
  float maxX = fmaxf(tl.x, br.x);
  float maxY = fmaxf(tl.y, br.y);
  return (Rectangle){minX, minY, maxX - minX, maxY - minY};
}

static bool CanvasStrokeBounds(const Canvas *canvas, Rectangle *out) {
  if (!canvas || !out)
    return false;
  bool hasAny = false;
  float minX = 0.0f, minY = 0.0f, maxX = 0.0f, maxY = 0.0f;
  for (int i = 0; i < canvas->strokeCount; i++) {
    const Stroke *s = &canvas->strokes[i];
    float radius = fmaxf(1.0f, s->thickness) * 0.5f;
    for (int p = 0; p < s->pointCount; p++) {
      float x = s->points[p].x;
      float y = s->points[p].y;
      if (!hasAny) {
        minX = x - radius;
        maxX = x + radius;
        minY = y - radius;
        maxY = y + radius;
        hasAny = true;
      } else {
        minX = fminf(minX, x - radius);
        maxX = fmaxf(maxX, x + radius);
        minY = fminf(minY, y - radius);
        maxY = fmaxf(maxY, y + radius);
      }
    }
  }
  if (!hasAny)
    return false;
  out->x = minX;
  out->y = minY;
  out->width = maxX - minX;
  out->height = maxY - minY;
  return true;
}

static float GridSpacingForZoom(float zoom) {
  float spacing = 40.0f;
  if (zoom > 2.0f)
    spacing *= 0.5f;
  if (zoom < 0.5f)
    spacing *= 2.0f;
  if (zoom < 0.25f)
    spacing *= 4.0f;
  return spacing;
}

static void ColorToHex(Color c, char *out, size_t size) {
  snprintf(out, size, "#%02X%02X%02X", c.r, c.g, c.b);
}

static bool StrokeLooksLikeArrow(const Stroke *s) {
  if (!s || s->pointCount != 5)
    return false;
  Point tip = s->points[1];
  Point last = s->points[4];
  float dx = tip.x - last.x;
  float dy = tip.y - last.y;
  return (dx * dx + dy * dy) <= 0.0001f;
}

static float CatmullRom(float p0, float p1, float p2, float p3, float t) {
  float t2 = t * t;
  float t3 = t2 * t;
  return 0.5f * ((2.0f * p1) + (-p0 + p2) * t +
                 (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                 (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

static int BuildSmoothedPoints(const Stroke *s, Vector2 **outPts) {
  if (!s || !outPts || s->pointCount < 2)
    return 0;
  const int samplesPerSegment = 7;
  int segments = s->pointCount - 1;
  int outCount = segments * samplesPerSegment + 1;
  Vector2 *pts = (Vector2 *)malloc(sizeof(Vector2) * (size_t)outCount);
  if (!pts)
    return 0;

  int index = 0;
  for (int i = 0; i < segments; i++) {
    int i0 = (i == 0) ? 0 : i - 1;
    int i1 = i;
    int i2 = i + 1;
    int i3 = (i + 2 < s->pointCount) ? i + 2 : s->pointCount - 1;

    Point p0 = s->points[i0];
    Point p1 = s->points[i1];
    Point p2 = s->points[i2];
    Point p3 = s->points[i3];

    for (int j = 0; j < samplesPerSegment; j++) {
      float t = (float)j / (float)samplesPerSegment;
      pts[index].x = CatmullRom(p0.x, p1.x, p2.x, p3.x, t);
      pts[index].y = CatmullRom(p0.y, p1.y, p2.y, p3.y, t);
      index++;
    }
  }

  Point last = s->points[s->pointCount - 1];
  pts[index++] = (Vector2){last.x, last.y};
  *outPts = pts;
  return index;
}

static int CollectStrokePoints(const Stroke *s, Vector2 **outPts) {
  if (!s || !outPts || s->pointCount < 2)
    return 0;
  bool smooth = s->usePressure || s->pointCount > 12;
  if (smooth)
    return BuildSmoothedPoints(s, outPts);

  Vector2 *pts = (Vector2 *)malloc(sizeof(Vector2) * (size_t)s->pointCount);
  if (!pts)
    return 0;
  for (int i = 0; i < s->pointCount; i++)
    pts[i] = (Vector2){s->points[i].x, s->points[i].y};
  *outPts = pts;
  return s->pointCount;
}

static void SvgWriteLine(FILE *f, float x1, float y1, float x2, float y2,
                         Color c, float thickness) {
  char hex[16];
  ColorToHex(c, hex, sizeof(hex));
  fprintf(f,
          "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" "
          "stroke=\"%s\" stroke-width=\"%.2f\" stroke-linecap=\"round\"",
          x1, y1, x2, y2, hex, thickness);
  if (c.a < 255)
    fprintf(f, " stroke-opacity=\"%.3f\"", (float)c.a / 255.0f);
  fprintf(f, " />\n");
}

static void SvgWritePolygon(FILE *f, Vector2 a, Vector2 b, Vector2 c, Color col) {
  char hex[16];
  ColorToHex(col, hex, sizeof(hex));
  fprintf(f,
          "<polygon points=\"%.2f,%.2f %.2f,%.2f %.2f,%.2f\" fill=\"%s\"",
          a.x, a.y, b.x, b.y, c.x, c.y, hex);
  if (col.a < 255)
    fprintf(f, " fill-opacity=\"%.3f\"", (float)col.a / 255.0f);
  fprintf(f, " />\n");
}

static void SvgWritePolyline(FILE *f, const Vector2 *pts, int count, Color col,
                             float thickness) {
  if (!f || !pts || count < 2)
    return;
  char hex[16];
  ColorToHex(col, hex, sizeof(hex));
  fprintf(f,
          "<polyline fill=\"none\" stroke=\"%s\" stroke-width=\"%.2f\" "
          "stroke-linecap=\"round\" stroke-linejoin=\"round\"",
          hex, thickness);
  if (col.a < 255)
    fprintf(f, " stroke-opacity=\"%.3f\"", (float)col.a / 255.0f);
  fprintf(f, " points=\"");
  for (int i = 0; i < count; i++) {
    fprintf(f, "%.2f,%.2f", pts[i].x, pts[i].y);
    if (i + 1 < count)
      fprintf(f, " ");
  }
  fprintf(f, "\" />\n");
}

static void SvgWriteGrid(FILE *f, Rectangle view, Color grid, float zoom) {
  float spacing = GridSpacingForZoom(zoom);
  if (spacing <= 0.0f)
    return;

  int startCol = (int)floorf(view.x / spacing);
  int endCol = (int)ceilf((view.x + view.width) / spacing);
  int startRow = (int)floorf(view.y / spacing);
  int endRow = (int)ceilf((view.y + view.height) / spacing);

  int maxLines = 2000;
  int cols = endCol - startCol + 1;
  int rows = endRow - startRow + 1;
  while (cols > maxLines || rows > maxLines) {
    spacing *= 2.0f;
    startCol = (int)floorf(view.x / spacing);
    endCol = (int)ceilf((view.x + view.width) / spacing);
    startRow = (int)floorf(view.y / spacing);
    endRow = (int)ceilf((view.y + view.height) / spacing);
    cols = endCol - startCol + 1;
    rows = endRow - startRow + 1;
  }

  char hex[16];
  ColorToHex(grid, hex, sizeof(hex));
  fprintf(f, "<g stroke=\"%s\" stroke-width=\"1\"", hex);
  if (grid.a < 255)
    fprintf(f, " stroke-opacity=\"%.3f\"", (float)grid.a / 255.0f);
  fprintf(f, ">\n");

  for (int i = startCol; i <= endCol; i++) {
    float x = (float)i * spacing;
    fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" />\n",
            x, view.y, x, view.y + view.height);
  }
  for (int i = startRow; i <= endRow; i++) {
    float y = (float)i * spacing;
    fprintf(f, "<line x1=\"%.2f\" y1=\"%.2f\" x2=\"%.2f\" y2=\"%.2f\" />\n",
            view.x, y, view.x + view.width, y);
  }

  fprintf(f, "</g>\n");
}

static bool ExportCanvasRaster(const Canvas *canvas, ExportScope scope,
                               ExportFormat format, const char *path) {
  if (!canvas || !path)
    return false;

  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  if (sw <= 0 || sh <= 0)
    return false;

  Camera2D cam = canvas->camera;
  if (scope == EXPORT_SCOPE_CANVAS) {
    Rectangle bounds;
    if (CanvasStrokeBounds(canvas, &bounds)) {
      float pad = 24.0f;
      float bw = fmaxf(bounds.width + pad * 2.0f, 1.0f);
      float bh = fmaxf(bounds.height + pad * 2.0f, 1.0f);
      float zoomX = (float)sw / bw;
      float zoomY = (float)sh / bh;
      float zoom = fminf(zoomX, zoomY);
      cam.target = (Vector2){bounds.x + bounds.width * 0.5f,
                             bounds.y + bounds.height * 0.5f};
      cam.offset = (Vector2){(float)sw / 2.0f, (float)sh / 2.0f};
      cam.zoom = (zoom > 0.0f) ? zoom : 1.0f;
      cam.rotation = 0.0f;
    }
  }

  Canvas temp = *canvas;
  temp.camera = cam;
  temp.selectedStrokeIndex = -1;
  temp.isDrawing = false;

  RenderTexture2D target = LoadRenderTexture(sw, sh);
  if (target.texture.id == 0)
    return false;

  BeginTextureMode(target);
  DrawCanvas(&temp);
  EndTextureMode();

  Image img = LoadImageFromTexture(target.texture);
  ImageFlipVertical(&img);
  UnloadRenderTexture(target);

  bool ok = ExportImage(img, path);
  UnloadImage(img);
  (void)format;
  return ok;
}

static bool ExportCanvasSvg(const Canvas *canvas, ExportScope scope,
                            const char *path) {
  if (!canvas || !path)
    return false;

  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  Rectangle view = {0, 0, (float)sw, (float)sh};
  float zoom = canvas->camera.zoom;

  if (scope == EXPORT_SCOPE_VIEW) {
    view = GetCameraViewRect(&canvas->camera, sw, sh);
  } else {
    Rectangle bounds;
    if (CanvasStrokeBounds(canvas, &bounds)) {
      float pad = 24.0f;
      view.x = bounds.x - pad;
      view.y = bounds.y - pad;
      view.width = bounds.width + pad * 2.0f;
      view.height = bounds.height + pad * 2.0f;
      zoom = 1.0f;
    } else {
      view = GetCameraViewRect(&canvas->camera, sw, sh);
    }
  }

  if (view.width <= 0.0f)
    view.width = 1.0f;
  if (view.height <= 0.0f)
    view.height = 1.0f;

  FILE *f = fopen(path, "wb");
  if (!f)
    return false;

  fprintf(f, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  fprintf(f,
          "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%.2f\" "
          "height=\"%.2f\" viewBox=\"%.2f %.2f %.2f %.2f\">\n",
          view.width, view.height, view.x, view.y, view.width, view.height);

  char bgHex[16];
  ColorToHex(canvas->backgroundColor, bgHex, sizeof(bgHex));
  fprintf(f,
          "<rect x=\"%.2f\" y=\"%.2f\" width=\"%.2f\" height=\"%.2f\" "
          "fill=\"%s\"",
          view.x, view.y, view.width, view.height, bgHex);
  if (canvas->backgroundColor.a < 255)
    fprintf(f, " fill-opacity=\"%.3f\"",
            (float)canvas->backgroundColor.a / 255.0f);
  fprintf(f, " />\n");

  if (canvas->showGrid)
    SvgWriteGrid(f, view, canvas->gridColor, zoom);

  for (int i = 0; i < canvas->strokeCount; i++) {
    const Stroke *s = &canvas->strokes[i];
    if (s->pointCount < 2)
      continue;

    if (StrokeLooksLikeArrow(s)) {
      Vector2 start = {s->points[0].x, s->points[0].y};
      Vector2 tip = {s->points[1].x, s->points[1].y};
      Vector2 st = Vector2Subtract(tip, start);
      float len = Vector2Length(st);
      if (len <= 0.0001f) {
        SvgWriteLine(f, start.x, start.y, tip.x, tip.y, s->color, s->thickness);
        continue;
      }

      Vector2 dir = Vector2Scale(st, 1.0f / len);
      Vector2 perp = (Vector2){-dir.y, dir.x};

      float headLen = fmaxf(16.0f, s->thickness * 4.0f);
      headLen = fminf(headLen, len * 0.5f);
      float headW = fmaxf(s->thickness * 3.0f, headLen * 1.10f);

      Vector2 base = Vector2Subtract(tip, Vector2Scale(dir, headLen));
      Vector2 left = Vector2Add(base, Vector2Scale(perp, headW * 0.5f));
      Vector2 right = Vector2Subtract(base, Vector2Scale(perp, headW * 0.5f));

      Vector2 shaftEnd = Vector2Add(base, Vector2Scale(dir, s->thickness * 0.25f));
      SvgWriteLine(f, start.x, start.y, shaftEnd.x, shaftEnd.y, s->color,
                   s->thickness);
      SvgWritePolygon(f, tip, left, right, s->color);
      continue;
    }

    Vector2 *pts = NULL;
    int count = CollectStrokePoints(s, &pts);
    if (count >= 2) {
      float thickness = (s->thickness > 0.0f) ? s->thickness : 1.0f;
      SvgWritePolyline(f, pts, count, s->color, thickness);
    }
    free(pts);
  }

  fprintf(f, "</svg>\n");
  fclose(f);
  return true;
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

void GuiRequestExport(GuiState *gui, const Canvas *canvas, ExportFormat format,
                      ExportScope scope) {
  gui->showMenu = false;
  gui->showColorPicker = false;

  if (!canvas) {
    GuiToastSet(gui, "No canvas.");
    return;
  }

  if (!HasGuiSession()) {
    GuiToastSet(gui, "No GUI session for file dialogs.");
    return;
  }

  const char *filter = (format == EXPORT_FORMAT_SVG)
                           ? "svg"
                           : (format == EXPORT_FORMAT_JPG) ? "jpg,jpeg" : "png";

  char base[128];
  Document *doc = GuiGetActiveDocument(gui);
  const char *name = (doc && doc->hasPath) ? GetFileName(doc->path) : "drawing";
  CopyPath(base, sizeof(base), name);
  char *dot = strrchr(base, '.');
  if (dot)
    *dot = '\0';
  if (base[0] == '\0')
    snprintf(base, sizeof(base), "drawing");

  char defaultPath[512];
  BuildDefaultExportPath(defaultPath, sizeof(defaultPath), base, format);

  nfdchar_t *path = NULL;
  nfdresult_t result = NFD_SaveDialog(filter, defaultPath, &path);
  if (result == NFD_OKAY && path) {
    char resolved[512];
    CopyPath(resolved, sizeof(resolved), path);
    EnsureExportExtension(resolved, sizeof(resolved), format);

    bool ok = false;
    if (format == EXPORT_FORMAT_SVG)
      ok = ExportCanvasSvg(canvas, scope, resolved);
    else
      ok = ExportCanvasRaster(canvas, scope, format, resolved);

    const char *label = (format == EXPORT_FORMAT_SVG)
                            ? "SVG"
                            : (format == EXPORT_FORMAT_JPG) ? "JPG" : "PNG";
    if (ok) {
      char msg[64];
      snprintf(msg, sizeof(msg), "Exported %s.", label);
      GuiToastSet(gui, msg);
      UpdateLastDir(gui, resolved);
      UpdateLastFileName(gui, resolved);
    } else {
      GuiToastSet(gui, "Export failed.");
    }

    free(path);
    return;
  }

  if (result == NFD_ERROR)
    ShowDialogError(gui, "Export failed");
}

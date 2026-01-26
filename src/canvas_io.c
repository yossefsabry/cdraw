#include "canvas.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char kBinaryMagic[4] = {'C', 'D', 'R', 'B'};
static const uint32_t kBinaryVersion = 1;
static const uint32_t kBinaryFlags = 0;
static const uint32_t kMaxStrokes = 1000000;
static const uint64_t kMaxTotalPoints = 10000000;

static bool WriteBytes(FILE *f, const void *data, size_t size) {
  return fwrite(data, 1, size, f) == size;
}

static bool ReadBytes(FILE *f, void *data, size_t size) {
  return fread(data, 1, size, f) == size;
}

static bool WriteU8(FILE *f, uint8_t value) {
  return WriteBytes(f, &value, sizeof(value));
}

static bool WriteU32(FILE *f, uint32_t value) {
  return WriteBytes(f, &value, sizeof(value));
}

static bool WriteF32(FILE *f, float value) {
  return WriteBytes(f, &value, sizeof(value));
}

static bool ReadU8(FILE *f, uint8_t *value) {
  return ReadBytes(f, value, sizeof(*value));
}

static bool ReadU32(FILE *f, uint32_t *value) {
  return ReadBytes(f, value, sizeof(*value));
}

static bool ReadF32(FILE *f, float *value) {
  return ReadBytes(f, value, sizeof(*value));
}

static bool WriteBinaryCanvas(const Canvas *canvas, FILE *f) {
  if (!canvas)
    return false;
  if (canvas->strokeCount < 0 || canvas->strokeCount > (int)kMaxStrokes)
    return false;

  uint32_t strokeCount = (uint32_t)canvas->strokeCount;

  if (!WriteBytes(f, kBinaryMagic, sizeof(kBinaryMagic)))
    return false;
  if (!WriteU32(f, kBinaryVersion))
    return false;
  if (!WriteU32(f, kBinaryFlags))
    return false;
  if (!WriteU32(f, strokeCount))
    return false;

  if (!WriteF32(f, canvas->camera.target.x) ||
      !WriteF32(f, canvas->camera.target.y) ||
      !WriteF32(f, canvas->camera.offset.x) ||
      !WriteF32(f, canvas->camera.offset.y) ||
      !WriteF32(f, canvas->camera.zoom) ||
      !WriteF32(f, canvas->camera.rotation))
    return false;

  if (!WriteU8(f, canvas->backgroundColor.r) ||
      !WriteU8(f, canvas->backgroundColor.g) ||
      !WriteU8(f, canvas->backgroundColor.b) ||
      !WriteU8(f, canvas->backgroundColor.a))
    return false;
  if (!WriteU8(f, canvas->gridColor.r) || !WriteU8(f, canvas->gridColor.g) ||
      !WriteU8(f, canvas->gridColor.b) || !WriteU8(f, canvas->gridColor.a))
    return false;
  if (!WriteU8(f, canvas->selectionColor.r) ||
      !WriteU8(f, canvas->selectionColor.g) ||
      !WriteU8(f, canvas->selectionColor.b) ||
      !WriteU8(f, canvas->selectionColor.a))
    return false;

  uint8_t showGrid = canvas->showGrid ? 1u : 0u;
  if (!WriteU8(f, showGrid))
    return false;
  uint8_t pad[3] = {0, 0, 0};
  if (!WriteBytes(f, pad, sizeof(pad)))
    return false;

  for (uint32_t i = 0; i < strokeCount; i++) {
    const Stroke *s = &canvas->strokes[i];
    if (s->pointCount < 0)
      return false;
    if ((uint64_t)s->pointCount > kMaxTotalPoints)
      return false;
    uint32_t pointCount = (uint32_t)s->pointCount;
    if (!WriteU8(f, s->color.r) || !WriteU8(f, s->color.g) ||
        !WriteU8(f, s->color.b) || !WriteU8(f, s->color.a))
      return false;
    if (!WriteF32(f, s->thickness))
      return false;
    uint8_t usePressure = s->usePressure ? 1u : 0u;
    if (!WriteU8(f, usePressure))
      return false;
    if (!WriteBytes(f, pad, sizeof(pad)))
      return false;
    if (!WriteU32(f, pointCount))
      return false;
    for (uint32_t p = 0; p < pointCount; p++) {
      if (!WriteF32(f, s->points[p].x) || !WriteF32(f, s->points[p].y) ||
          !WriteF32(f, s->points[p].width))
        return false;
    }
  }

  return true;
}

static bool LoadCanvasFromText(Canvas *canvas, FILE *f) {
  char header[32] = {0};
  bool isV1 = false;
  if (!fgets(header, (int)sizeof(header), f))
    return false;
  if (strncmp(header, "CDRAW2", 5) != 0) {
    if (strncmp(header, "CDRAW1", 5) == 0)
      isV1 = true;
    else
      return false;
  }

  ClearCanvas(canvas);

  char tok[32] = {0};
  int strokeCount = 0;
  if (fscanf(f, "%31s %d", tok, &strokeCount) != 2 || strcmp(tok, "strokes") != 0 ||
      strokeCount < 0)
    return false;

  for (int i = 0; i < strokeCount; i++) {
    unsigned int r, g, b, a;
    float thickness;
    int pointCount;
    int usePressure = 0;
    if (isV1) {
      if (fscanf(f, "%31s %u %u %u %u %f %d", tok, &r, &g, &b, &a, &thickness,
                 &pointCount) != 7 ||
          strcmp(tok, "stroke") != 0 || pointCount < 0) {
        ClearCanvas(canvas);
        return false;
      }
    } else if (fscanf(f, "%31s %u %u %u %u %f %d %d", tok, &r, &g, &b, &a,
                      &thickness, &pointCount, &usePressure) != 8 ||
               strcmp(tok, "stroke") != 0 || pointCount < 0) {
      ClearCanvas(canvas);
      return false;
    }

    Stroke s = {0};
    s.color = (Color){(unsigned char)r, (unsigned char)g, (unsigned char)b,
                      (unsigned char)a};
    s.thickness = thickness;
    s.usePressure = (usePressure != 0) && !isV1;
    s.pointCount = pointCount;
    s.capacity = pointCount;
    if (pointCount > 0) {
      if ((size_t)pointCount > SIZE_MAX / sizeof(Point)) {
        ClearCanvas(canvas);
        return false;
      }
      s.points = (Point *)malloc(sizeof(Point) * (size_t)pointCount);
      if (!s.points) {
        ClearCanvas(canvas);
        return false;
      }
      for (int p = 0; p < pointCount; p++) {
        float x, y, w = 0.0f;
        if (isV1) {
          if (fscanf(f, "%f %f", &x, &y) != 2) {
            free(s.points);
            ClearCanvas(canvas);
            return false;
          }
        } else if (fscanf(f, "%f %f %f", &x, &y, &w) != 3) {
          free(s.points);
          ClearCanvas(canvas);
          return false;
        }
        s.points[p] = (Point){x, y, w};
      }
    }
    AddStroke(canvas, s);
  }

  return true;
}

static bool LoadCanvasFromBinary(Canvas *canvas, FILE *f) {
  uint32_t version = 0;
  uint32_t flags = 0;
  uint32_t strokeCount = 0;
  float targetX = 0.0f, targetY = 0.0f;
  float offsetX = 0.0f, offsetY = 0.0f;
  float zoom = 1.0f, rotation = 0.0f;
  uint8_t bg[4] = {0, 0, 0, 255};
  uint8_t grid[4] = {0, 0, 0, 255};
  uint8_t selection[4] = {0, 0, 0, 255};
  uint8_t showGrid = 1u;
  uint8_t pad[3] = {0, 0, 0};

  if (!ReadU32(f, &version) || !ReadU32(f, &flags) || !ReadU32(f, &strokeCount))
    return false;
  (void)flags;
  if (version != kBinaryVersion)
    return false;
  if (strokeCount > kMaxStrokes)
    return false;

  if (!ReadF32(f, &targetX) || !ReadF32(f, &targetY) || !ReadF32(f, &offsetX) ||
      !ReadF32(f, &offsetY) || !ReadF32(f, &zoom) || !ReadF32(f, &rotation))
    return false;

  if (!ReadU8(f, &bg[0]) || !ReadU8(f, &bg[1]) || !ReadU8(f, &bg[2]) ||
      !ReadU8(f, &bg[3]))
    return false;
  if (!ReadU8(f, &grid[0]) || !ReadU8(f, &grid[1]) || !ReadU8(f, &grid[2]) ||
      !ReadU8(f, &grid[3]))
    return false;
  if (!ReadU8(f, &selection[0]) || !ReadU8(f, &selection[1]) ||
      !ReadU8(f, &selection[2]) || !ReadU8(f, &selection[3]))
    return false;
  if (!ReadU8(f, &showGrid))
    return false;
  if (!ReadBytes(f, pad, sizeof(pad)))
    return false;

  ClearCanvas(canvas);

  Stroke *strokes = NULL;
  if (strokeCount > 0) {
    strokes = (Stroke *)calloc(strokeCount, sizeof(Stroke));
    if (!strokes)
      return false;
  }

  uint64_t totalPoints = 0;
  for (uint32_t i = 0; i < strokeCount; i++) {
    uint8_t color[4] = {0, 0, 0, 255};
    float thickness = 0.0f;
    uint8_t usePressure = 0u;
    uint32_t pointCount = 0;

    if (!ReadU8(f, &color[0]) || !ReadU8(f, &color[1]) || !ReadU8(f, &color[2]) ||
        !ReadU8(f, &color[3]))
      goto fail;
    if (!ReadF32(f, &thickness))
      goto fail;
    if (!ReadU8(f, &usePressure))
      goto fail;
    if (!ReadBytes(f, pad, sizeof(pad)))
      goto fail;
    if (!ReadU32(f, &pointCount))
      goto fail;

    if (pointCount > kMaxTotalPoints || totalPoints + pointCount > kMaxTotalPoints)
      goto fail;
    if (pointCount > 0 && (size_t)pointCount > SIZE_MAX / sizeof(Point))
      goto fail;

    Stroke s = {0};
    s.color = (Color){color[0], color[1], color[2], color[3]};
    s.thickness = thickness;
    s.usePressure = (usePressure != 0);
    s.pointCount = (int)pointCount;
    s.capacity = (int)pointCount;
    if (pointCount > 0) {
      s.points = (Point *)malloc(sizeof(Point) * (size_t)pointCount);
      if (!s.points)
        goto fail;
      for (uint32_t p = 0; p < pointCount; p++) {
        float x = 0.0f, y = 0.0f, w = 0.0f;
        if (!ReadF32(f, &x) || !ReadF32(f, &y) || !ReadF32(f, &w)) {
          free(s.points);
          goto fail;
        }
        s.points[p] = (Point){x, y, w};
      }
    }

    strokes[i] = s;
    totalPoints += pointCount;
  }

  if (totalPoints > INT_MAX)
    goto fail;

  canvas->camera.target = (Vector2){targetX, targetY};
  canvas->camera.offset = (Vector2){offsetX, offsetY};
  canvas->camera.zoom = zoom;
  canvas->camera.rotation = rotation;
  canvas->backgroundColor = (Color){bg[0], bg[1], bg[2], bg[3]};
  canvas->gridColor = (Color){grid[0], grid[1], grid[2], grid[3]};
  canvas->selectionColor = (Color){selection[0], selection[1], selection[2], selection[3]};
  canvas->showGrid = (showGrid != 0);

  canvas->strokes = strokes;
  canvas->strokeCount = (int)strokeCount;
  canvas->capacity = (int)strokeCount;
  canvas->totalPoints = (int)totalPoints;

  return true;

fail:
  if (strokes) {
    for (uint32_t i = 0; i < strokeCount; i++)
      free(strokes[i].points);
    free(strokes);
  }
  ClearCanvas(canvas);
  return false;
}

bool SaveCanvasToFile(const Canvas *canvas, const char *path) {
  FILE *f = fopen(path, "wb");
  if (!f)
    return false;
  bool ok = WriteBinaryCanvas(canvas, f);
  fclose(f);
  return ok;
}

bool LoadCanvasFromFile(Canvas *canvas, const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return false;

  char magic[4] = {0};
  if (fread(magic, 1, sizeof(magic), f) != sizeof(magic)) {
    fclose(f);
    return false;
  }

  bool ok = false;
  if (memcmp(magic, kBinaryMagic, sizeof(kBinaryMagic)) == 0) {
    ok = LoadCanvasFromBinary(canvas, f);
  } else {
    rewind(f);
    ok = LoadCanvasFromText(canvas, f);
  }

  fclose(f);
  return ok;
}

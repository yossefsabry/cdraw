#include "canvas.h"
#include "raymath.h"
#include <float.h>
#include <malloc.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

void InitCanvas(Canvas *canvas, int screenWidth, int screenHeight) {
  canvas->strokeCount = 0;
  canvas->capacity = 1000;
  canvas->strokes = (Stroke *)malloc(sizeof(Stroke) * canvas->capacity);

  canvas->redoCount = 0;
  canvas->redoCapacity = 1000;
  canvas->redoStrokes = (Stroke *)malloc(sizeof(Stroke) * canvas->redoCapacity);

  canvas->camera.target = (Vector2){0.0f, 0.0f};
  canvas->camera.offset = (Vector2){screenWidth / 2.0f, screenHeight / 2.0f};
  canvas->camera.rotation = 0.0f;
  canvas->camera.zoom = 1.0f;

  canvas->isDrawing = false;
  canvas->showGrid = true;
  canvas->currentStroke.points = NULL;
  canvas->currentStroke.pointCount = 0;
  canvas->backgroundColor = (Color){20, 20, 20, 255};
  canvas->gridColor = (Color){50, 50, 50, 255};
  canvas->selectionColor = (Color){56, 189, 248, 255};

  canvas->selectedStrokeIndex = -1;
  canvas->isDraggingSelection = false;
  canvas->lastMouseWorld = (Vector2){0, 0};
}

void FreeCanvas(Canvas *canvas) {
  for (int i = 0; i < canvas->strokeCount; i++) {
    if (canvas->strokes[i].points)
      free(canvas->strokes[i].points);
  }
  free(canvas->strokes);

  for (int i = 0; i < canvas->redoCount; i++) {
    if (canvas->redoStrokes[i].points)
      free(canvas->redoStrokes[i].points);
  }
  free(canvas->redoStrokes);

  if (canvas->isDrawing && canvas->currentStroke.points) {
    free(canvas->currentStroke.points);
  }
}

static void AddPointToStroke(Stroke *stroke, Point p) {
  if (stroke->pointCount >= stroke->capacity) {
    stroke->capacity = (stroke->capacity == 0) ? 64 : stroke->capacity * 2;
    stroke->points =
        (Point *)realloc(stroke->points, sizeof(Point) * stroke->capacity);
  }
  stroke->points[stroke->pointCount++] = p;
}

static void ClearRedo(Canvas *canvas) {
  for (int i = 0; i < canvas->redoCount; i++) {
    if (canvas->redoStrokes[i].points)
      free(canvas->redoStrokes[i].points);
  }
  canvas->redoCount = 0;
}

void AddStroke(Canvas *canvas, Stroke stroke) {
  ClearRedo(canvas);

  if (canvas->strokeCount >= canvas->capacity) {
    canvas->capacity *= 2;
    canvas->strokes =
        (Stroke *)realloc(canvas->strokes, sizeof(Stroke) * canvas->capacity);
  }
  canvas->strokes[canvas->strokeCount++] = stroke;
}

void Undo(Canvas *canvas) {
  if (canvas->strokeCount > 0) {
    Stroke s = canvas->strokes[--canvas->strokeCount];
    if (canvas->redoCount >= canvas->redoCapacity) {
      canvas->redoCapacity *= 2;
      canvas->redoStrokes = (Stroke *)realloc(
          canvas->redoStrokes, sizeof(Stroke) * canvas->redoCapacity);
    }
    canvas->redoStrokes[canvas->redoCount++] = s;
    if (canvas->selectedStrokeIndex >= canvas->strokeCount)
      canvas->selectedStrokeIndex = -1;
    fprintf(stderr, "Action Undone. Strokes: %d\n", canvas->strokeCount);
  }
}

void Redo(Canvas *canvas) {
  if (canvas->redoCount > 0) {
    Stroke s = canvas->redoStrokes[--canvas->redoCount];
    if (canvas->strokeCount >= canvas->capacity) {
      canvas->capacity *= 2;
      canvas->strokes =
          (Stroke *)realloc(canvas->strokes, sizeof(Stroke) * canvas->capacity);
    }
    canvas->strokes[canvas->strokeCount++] = s;
    fprintf(stderr, "Action Redone. Strokes: %d\n", canvas->strokeCount);
  }
}

void ClearCanvas(Canvas *canvas) {
  for (int i = 0; i < canvas->strokeCount; i++) {
    if (canvas->strokes[i].points)
      free(canvas->strokes[i].points);
  }
  canvas->strokeCount = 0;

  ClearRedo(canvas);
  canvas->selectedStrokeIndex = -1;
  canvas->isDraggingSelection = false;

  if (canvas->currentStroke.points) {
    free(canvas->currentStroke.points);
    canvas->currentStroke.points = NULL;
  }
  canvas->isDrawing = false;
  canvas->currentStroke.pointCount = 0;
  canvas->currentStroke.capacity = 0;
  fprintf(stderr, "Canvas Cleared.\n");
}

static void CreateCircleStroke(Stroke *s, Point start, Point end) {
  Vector2 c = {start.x, start.y};
  float dx = end.x - start.x;
  float dy = end.y - start.y;
  float radius = sqrtf(dx * dx + dy * dy);

  int segments = 64;
  for (int i = 0; i <= segments; i++) {
    float angle = (float)i / (float)segments * PI * 2.0f;
    Point p = {c.x + cosf(angle) * radius, c.y + sinf(angle) * radius};
    AddPointToStroke(s, p);
  }
}

static float DistancePointToSegmentSquared(Vector2 p, Vector2 a, Vector2 b) {
  Vector2 ab = Vector2Subtract(b, a);
  float abLen2 = Vector2DotProduct(ab, ab);
  if (abLen2 <= 0.000001f)
    return Vector2DistanceSqr(p, a);
  float t = Vector2DotProduct(Vector2Subtract(p, a), ab) / abLen2;
  t = Clamp(t, 0.0f, 1.0f);
  Vector2 proj = Vector2Add(a, Vector2Scale(ab, t));
  return Vector2DistanceSqr(p, proj);
}

static float StrokeMinDistanceSquared(const Stroke *s, Vector2 p) {
  if (s->pointCount == 0)
    return FLT_MAX;
  if (s->pointCount == 1)
    return Vector2DistanceSqr(p, (Vector2){s->points[0].x, s->points[0].y});

  float best = FLT_MAX;
  for (int i = 0; i < s->pointCount - 1; i++) {
    Vector2 a = {s->points[i].x, s->points[i].y};
    Vector2 b = {s->points[i + 1].x, s->points[i + 1].y};
    float d2 = DistancePointToSegmentSquared(p, a, b);
    if (d2 < best)
      best = d2;
  }
  return best;
}

static int FindStrokeHit(const Canvas *canvas, Vector2 p, float radiusWorld) {
  float bestD2 = radiusWorld * radiusWorld;
  int bestIdx = -1;
  for (int i = canvas->strokeCount - 1; i >= 0; i--) {
    const Stroke *s = &canvas->strokes[i];
    float d2 = StrokeMinDistanceSquared(s, p);
    float localR = radiusWorld;
    if (s->thickness > 0.0f)
      localR = fmaxf(localR, (s->thickness * 0.75f) / canvas->camera.zoom);
    float thresh2 = localR * localR;
    if (d2 <= thresh2 && d2 <= bestD2) {
      bestD2 = d2;
      bestIdx = i;
    }
  }
  return bestIdx;
}

static void TranslateStroke(Stroke *s, Vector2 delta) {
  for (int i = 0; i < s->pointCount; i++) {
    s->points[i].x += delta.x;
    s->points[i].y += delta.y;
  }
}

static void RemoveStrokeAtIndex(Canvas *canvas, int index) {
  if (index < 0 || index >= canvas->strokeCount)
    return;
  if (canvas->strokes[index].points)
    free(canvas->strokes[index].points);

  for (int i = index; i < canvas->strokeCount - 1; i++) {
    canvas->strokes[i] = canvas->strokes[i + 1];
  }
  canvas->strokeCount--;

  if (canvas->selectedStrokeIndex == index)
    canvas->selectedStrokeIndex = -1;
  else if (canvas->selectedStrokeIndex > index)
    canvas->selectedStrokeIndex--;
}

void UpdateCanvasState(Canvas *canvas, bool inputCaptured, int activeTool) {
  float wheel = GetMouseWheelMove();
  if (wheel != 0 && !inputCaptured) {
    Vector2 mouseWorldPos =
        GetScreenToWorld2D(GetMousePosition(), canvas->camera);
    canvas->camera.offset = GetMousePosition();
    canvas->camera.target = mouseWorldPos;
    float scaleFactor = 1.0f + (0.25f * fabsf(wheel));
    if (wheel < 0)
      scaleFactor = 1.0f / scaleFactor;
    canvas->camera.zoom =
        Clamp(canvas->camera.zoom * scaleFactor, 0.125f, 64.0f);
  }

  bool isPanning = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
  if (activeTool == TOOL_PAN && IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
      !inputCaptured) {
    isPanning = true;
  }

  if (isPanning) {
    Vector2 delta = GetMouseDelta();
    delta = Vector2Scale(delta, -1.0f / canvas->camera.zoom);
    canvas->camera.target = Vector2Add(canvas->camera.target, delta);
  }

  if (!inputCaptured && activeTool == TOOL_SELECT) {
    Vector2 mouseWorld =
        GetScreenToWorld2D(GetMousePosition(), canvas->camera);
    float pickRadius = 8.0f / canvas->camera.zoom;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      int hit = FindStrokeHit(canvas, mouseWorld, pickRadius);
      canvas->selectedStrokeIndex = hit;
      canvas->isDraggingSelection = (hit >= 0);
      canvas->lastMouseWorld = mouseWorld;
    }

    if (canvas->isDraggingSelection && canvas->selectedStrokeIndex >= 0 &&
        IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      Vector2 delta = Vector2Subtract(mouseWorld, canvas->lastMouseWorld);
      TranslateStroke(&canvas->strokes[canvas->selectedStrokeIndex], delta);
      canvas->lastMouseWorld = mouseWorld;
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
      canvas->isDraggingSelection = false;

    if (canvas->selectedStrokeIndex >= 0 && IsKeyPressed(KEY_DELETE)) {
      RemoveStrokeAtIndex(canvas, canvas->selectedStrokeIndex);
    }
  }

  if (!inputCaptured && activeTool == TOOL_ERASER && !isPanning &&
      IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    Vector2 mouseWorld =
        GetScreenToWorld2D(GetMousePosition(), canvas->camera);
    float radiusWorld = (8.0f + canvas->currentStroke.thickness) /
                        fmaxf(canvas->camera.zoom, 0.001f);
    for (int i = canvas->strokeCount - 1; i >= 0; i--) {
      const Stroke *s = &canvas->strokes[i];
      float d2 = StrokeMinDistanceSquared(s, mouseWorld);
      if (d2 <= radiusWorld * radiusWorld)
        RemoveStrokeAtIndex(canvas, i);
    }
  }

  if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !isPanning && !inputCaptured) {
    if (activeTool == TOOL_PEN || activeTool == TOOL_LINE ||
        activeTool == TOOL_RECT || activeTool == TOOL_CIRCLE) {
      Vector2 mousePos = GetScreenToWorld2D(GetMousePosition(), canvas->camera);
      Point p = {mousePos.x, mousePos.y};

      if (!canvas->isDrawing) {
        canvas->isDrawing = true;
        canvas->startPoint = p;

        canvas->currentStroke.pointCount = 0;
        canvas->currentStroke.capacity = 0;
        canvas->currentStroke.points = NULL;
        if (canvas->currentStroke.thickness == 0)
          canvas->currentStroke.thickness = 3.0f;

        AddPointToStroke(&canvas->currentStroke, p);
        fprintf(stderr, "Start Drawing. Tool: %d at %.1f, %.1f\n", activeTool,
                p.x, p.y);
      } else {
        Stroke *s = &canvas->currentStroke;
        if (activeTool == TOOL_PEN) {
          if (s->pointCount > 0) {
            Point last = s->points[s->pointCount - 1];
            float dx = p.x - last.x;
            float dy = p.y - last.y;
            if (dx * dx + dy * dy > 2.0f)
              AddPointToStroke(s, p);
          }
        } else {
          s->pointCount = 0;
          Point start = canvas->startPoint;

          if (activeTool == TOOL_LINE) {
            AddPointToStroke(s, start);
            AddPointToStroke(s, p);
          } else if (activeTool == TOOL_RECT) {
            AddPointToStroke(s, start);
            AddPointToStroke(s, (Point){p.x, start.y});
            AddPointToStroke(s, (Point){p.x, p.y});
            AddPointToStroke(s, (Point){start.x, p.y});
            AddPointToStroke(s, start);
          } else if (activeTool == TOOL_CIRCLE) {
            CreateCircleStroke(s, start, p);
          }
        }
      }
    }
  } else {
    if (canvas->isDrawing) {
      canvas->isDrawing = false;
      if (canvas->currentStroke.pointCount > 1) {
        AddStroke(canvas, canvas->currentStroke);
        fprintf(stderr, "Finished Stroke. Points: %d\n",
                canvas->currentStroke.pointCount);

        canvas->currentStroke.points = NULL;
        canvas->currentStroke.pointCount = 0;
        canvas->currentStroke.capacity = 0;
      } else {
        if (canvas->currentStroke.points)
          free(canvas->currentStroke.points);
        canvas->currentStroke.points = NULL;
        canvas->currentStroke.pointCount = 0;
      }
    }
  }
}

static void DrawInfiniteGrid(Camera2D camera, Color gridColor) {
  Vector2 screenTopLeft = GetScreenToWorld2D((Vector2){0, 0}, camera);
  Vector2 screenBottomRight = GetScreenToWorld2D(
      (Vector2){GetScreenWidth(), GetScreenHeight()}, camera);
  float baseSpacing = 40.0f;
  float currentSpacing = baseSpacing;
  if (camera.zoom > 2.0f)
    currentSpacing = baseSpacing / 2.0f;
  if (camera.zoom < 0.5f)
    currentSpacing = baseSpacing * 2.0f;
  if (camera.zoom < 0.25f)
    currentSpacing = baseSpacing * 4.0f;

  int startCol = (int)floorf(screenTopLeft.x / currentSpacing);
  int endCol = (int)ceilf(screenBottomRight.x / currentSpacing);
  int startRow = (int)floorf(screenTopLeft.y / currentSpacing);
  int endRow = (int)ceilf(screenBottomRight.y / currentSpacing);

  for (int i = startCol; i <= endCol; i++) {
    float x = i * currentSpacing;
    DrawLineV((Vector2){x, screenTopLeft.y}, (Vector2){x, screenBottomRight.y},
              gridColor);
  }
  for (int i = startRow; i <= endRow; i++) {
    float y = i * currentSpacing;
    DrawLineV((Vector2){screenTopLeft.x, y}, (Vector2){screenBottomRight.x, y},
              gridColor);
  }
}

void DrawCanvas(Canvas *canvas) {
  ClearBackground(canvas->backgroundColor);

  BeginMode2D(canvas->camera);
  if (canvas->showGrid)
    DrawInfiniteGrid(canvas->camera, canvas->gridColor);

  for (int i = 0; i < canvas->strokeCount; i++) {
    Stroke *s = &canvas->strokes[i];
    if (s->pointCount < 2)
      continue;
    if (s->pointCount < 6) {
      for (int p = 0; p < s->pointCount - 1; p++) {
        DrawLineEx((Vector2){s->points[p].x, s->points[p].y},
                   (Vector2){s->points[p + 1].x, s->points[p + 1].y},
                   s->thickness, s->color);
      }
    } else {
      DrawSplineCatmullRom((Vector2 *)s->points, s->pointCount, s->thickness,
                           s->color);
    }
  }

  if (canvas->selectedStrokeIndex >= 0 &&
      canvas->selectedStrokeIndex < canvas->strokeCount) {
    Stroke *s = &canvas->strokes[canvas->selectedStrokeIndex];
    float th = s->thickness + 2.0f;
    if (s->pointCount >= 2) {
      if (s->pointCount < 6) {
        for (int p = 0; p < s->pointCount - 1; p++) {
          DrawLineEx((Vector2){s->points[p].x, s->points[p].y},
                     (Vector2){s->points[p + 1].x, s->points[p + 1].y}, th,
                     canvas->selectionColor);
        }
      } else {
        DrawSplineCatmullRom((Vector2 *)s->points, s->pointCount, th,
                             canvas->selectionColor);
      }
    }
  }

  if (canvas->isDrawing && canvas->currentStroke.pointCount > 1) {
    Stroke *s = &canvas->currentStroke;
    if (s->pointCount < 6) {
      for (int p = 0; p < s->pointCount - 1; p++) {
        DrawLineEx((Vector2){s->points[p].x, s->points[p].y},
                   (Vector2){s->points[p + 1].x, s->points[p + 1].y},
                   s->thickness, s->color);
      }
    } else {
      DrawSplineCatmullRom((Vector2 *)s->points, s->pointCount, s->thickness,
                           s->color);
    }
  }

  EndMode2D();
}

bool SaveCanvasToFile(const Canvas *canvas, const char *path) {
  FILE *f = fopen(path, "wb");
  if (!f)
    return false;

  fprintf(f, "CDRAW1\n");
  fprintf(f, "strokes %d\n", canvas->strokeCount);
  for (int i = 0; i < canvas->strokeCount; i++) {
    const Stroke *s = &canvas->strokes[i];
    fprintf(f, "stroke %u %u %u %u %.3f %d\n", s->color.r, s->color.g,
            s->color.b, s->color.a, s->thickness, s->pointCount);
    for (int p = 0; p < s->pointCount; p++) {
      fprintf(f, "%.6f %.6f\n", s->points[p].x, s->points[p].y);
    }
  }
  fclose(f);
  return true;
}

bool LoadCanvasFromFile(Canvas *canvas, const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return false;

  char header[32] = {0};
  if (!fgets(header, (int)sizeof(header), f)) {
    fclose(f);
    return false;
  }
  if (strncmp(header, "CDRAW1", 5) != 0) {
    fclose(f);
    return false;
  }

  ClearCanvas(canvas);
  ClearRedo(canvas);

  char tok[32] = {0};
  int strokeCount = 0;
  if (fscanf(f, "%31s %d", tok, &strokeCount) != 2) {
    fclose(f);
    return false;
  }
  if (strcmp(tok, "strokes") != 0 || strokeCount < 0) {
    fclose(f);
    return false;
  }

  for (int i = 0; i < strokeCount; i++) {
    unsigned int r, g, b, a;
    float thickness;
    int pointCount;
    if (fscanf(f, "%31s %u %u %u %u %f %d", tok, &r, &g, &b, &a, &thickness,
               &pointCount) != 7) {
      fclose(f);
      return false;
    }
    if (strcmp(tok, "stroke") != 0 || pointCount < 0) {
      fclose(f);
      return false;
    }

    Stroke s = {0};
    s.color = (Color){(unsigned char)r, (unsigned char)g, (unsigned char)b,
                      (unsigned char)a};
    s.thickness = thickness;
    s.pointCount = pointCount;
    s.capacity = pointCount;
    if (pointCount > 0) {
      s.points = (Point *)malloc(sizeof(Point) * (size_t)pointCount);
      if (!s.points) {
        fclose(f);
        return false;
      }
      for (int p = 0; p < pointCount; p++) {
        float x, y;
        if (fscanf(f, "%f %f", &x, &y) != 2) {
          free(s.points);
          fclose(f);
          return false;
        }
        s.points[p] = (Point){x, y};
      }
    }
    AddStroke(canvas, s);
  }

  fclose(f);
  return true;
}

int GetTotalPoints(const Canvas *canvas) {
  int total = 0;
  for (int i = 0; i < canvas->strokeCount; i++) {
    total += canvas->strokes[i].pointCount;
  }
  if (canvas->isDrawing)
    total += canvas->currentStroke.pointCount;
  return total;
}

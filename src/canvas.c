#include "canvas.h"
#include "raymath.h"
#include <malloc.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

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

void AddStroke(Canvas *canvas, Stroke stroke) {
  for (int i = 0; i < canvas->redoCount; i++) {
    if (canvas->redoStrokes[i].points)
      free(canvas->redoStrokes[i].points);
  }
  canvas->redoCount = 0;

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

  for (int i = 0; i < canvas->redoCount; i++) {
    if (canvas->redoStrokes[i].points)
      free(canvas->redoStrokes[i].points);
  }
  canvas->redoCount = 0;
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

static void DrawInfiniteGrid(Camera2D camera) {
  Vector2 screenTopLeft = GetScreenToWorld2D((Vector2){0, 0}, camera);
  Vector2 screenBottomRight = GetScreenToWorld2D(
      (Vector2){GetScreenWidth(), GetScreenHeight()}, camera);
  float baseSpacing = 50.0f;
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

  Color gridColor = (Color){50, 50, 50, 255};
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
  ClearBackground((Color){20, 20, 20, 255});

  BeginMode2D(canvas->camera);
  if (canvas->showGrid)
    DrawInfiniteGrid(canvas->camera);

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

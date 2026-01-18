#include "canvas.h"
#include <math.h>

static void DrawInfiniteGrid(Camera2D camera, Color gridColor) {
  Vector2 tl = GetScreenToWorld2D((Vector2){0, 0}, camera);
  Vector2 br = GetScreenToWorld2D(
      (Vector2){GetScreenWidth(), GetScreenHeight()}, camera);
  float spacing = 40.0f;
  if (camera.zoom > 2.0f)
    spacing *= 0.5f;
  if (camera.zoom < 0.5f)
    spacing *= 2.0f;
  if (camera.zoom < 0.25f)
    spacing *= 4.0f;

  int startCol = (int)floorf(tl.x / spacing);
  int endCol = (int)ceilf(br.x / spacing);
  int startRow = (int)floorf(tl.y / spacing);
  int endRow = (int)ceilf(br.y / spacing);

  for (int i = startCol; i <= endCol; i++) {
    float x = i * spacing;
    DrawLineV((Vector2){x, tl.y}, (Vector2){x, br.y}, gridColor);
  }
  for (int i = startRow; i <= endRow; i++) {
    float y = i * spacing;
    DrawLineV((Vector2){tl.x, y}, (Vector2){br.x, y}, gridColor);
  }
}

static void DrawStroke(const Stroke *s, float thickness, Color color) {
  if (s->pointCount < 2)
    return;
  if (s->pointCount < 6) {
    for (int p = 0; p < s->pointCount - 1; p++) {
      DrawLineEx((Vector2){s->points[p].x, s->points[p].y},
                 (Vector2){s->points[p + 1].x, s->points[p + 1].y}, thickness,
                 color);
    }
  } else {
    DrawSplineCatmullRom((Vector2 *)s->points, s->pointCount, thickness, color);
  }
}

void DrawCanvas(Canvas *canvas) {
  ClearBackground(canvas->backgroundColor);
  BeginMode2D(canvas->camera);
  if (canvas->showGrid)
    DrawInfiniteGrid(canvas->camera, canvas->gridColor);

  for (int i = 0; i < canvas->strokeCount; i++)
    DrawStroke(&canvas->strokes[i], canvas->strokes[i].thickness,
               canvas->strokes[i].color);

  if (canvas->selectedStrokeIndex >= 0 &&
      canvas->selectedStrokeIndex < canvas->strokeCount) {
    Stroke *s = &canvas->strokes[canvas->selectedStrokeIndex];
    DrawStroke(s, s->thickness + 2.0f, canvas->selectionColor);
  }

  if (canvas->isDrawing)
    DrawStroke(&canvas->currentStroke, canvas->currentStroke.thickness,
               canvas->currentStroke.color);

  EndMode2D();
}


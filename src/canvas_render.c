#include "canvas.h"
#include "raymath.h"
#include <math.h>

static Vector2 PointAsVector2(Point p) { return (Vector2){p.x, p.y}; }

static bool StrokeLooksLikeArrow(const Stroke *s) {
  if (s->pointCount != 5)
    return false;

  // Current arrow representation from TOOL_LINE:
  // [start, tip, left, right, tip]
  Point tip = s->points[1];
  Point last = s->points[4];
  float dx = tip.x - last.x;
  float dy = tip.y - last.y;
  return (dx * dx + dy * dy) <= 0.0001f;
}

static void DrawArrowStroke(const Stroke *s, float thickness, Color color) {
  Vector2 start = PointAsVector2(s->points[0]);
  Vector2 tip = PointAsVector2(s->points[1]);

  Vector2 st = Vector2Subtract(tip, start);
  float len = Vector2Length(st);
  if (len <= 0.0001f) {
    DrawLineEx(start, tip, thickness, color);
    return;
  }

  Vector2 dir = Vector2Scale(st, 1.0f / len);
  Vector2 perp = (Vector2){-dir.y, dir.x};

  float headLen = fmaxf(16.0f, thickness * 4.0f);
  headLen = fminf(headLen, len * 0.5f);
  float headW = fmaxf(thickness * 3.0f, headLen * 1.10f);

  Vector2 base = Vector2Subtract(tip, Vector2Scale(dir, headLen));
  Vector2 left = Vector2Add(base, Vector2Scale(perp, headW * 0.5f));
  Vector2 right = Vector2Subtract(base, Vector2Scale(perp, headW * 0.5f));

  // Draw shaft slightly into the head to avoid a visible seam.
  Vector2 shaftEnd = Vector2Add(base, Vector2Scale(dir, thickness * 0.25f));
  DrawLineEx(start, shaftEnd, thickness, color);

  // raylib expects CCW order for filled triangles.
  Vector2 l = left;
  Vector2 r = right;
  Vector2 tl = Vector2Subtract(l, tip);
  Vector2 tr = Vector2Subtract(r, tip);
  float cross = tl.x * tr.y - tl.y * tr.x;
  // NOTE: In raylib's default 2D projection the Y axis is flipped (top-left
  // origin), so "front-facing" winding is the opposite of the usual math
  // coordinate system.
  if (cross > 0.0f) {
    Vector2 tmp = l;
    l = r;
    r = tmp;
  }
  DrawTriangle(tip, l, r, color);
}

static bool StrokeIsClosed(const Stroke *s) {
  if (s->pointCount < 3)
    return false;
  Point first = s->points[0];
  Point last = s->points[s->pointCount - 1];
  float dx = first.x - last.x;
  float dy = first.y - last.y;

  // World-space epsilon; tight on purpose (our shape tools close exactly).
  return (dx * dx + dy * dy) <= 0.0001f;
}

static void DrawStrokeLinearOpen(const Stroke *s, float thickness, Color color) {
  for (int p = 0; p < s->pointCount - 1; p++) {
    DrawLineEx((Vector2){s->points[p].x, s->points[p].y},
               (Vector2){s->points[p + 1].x, s->points[p + 1].y}, thickness,
               color);
  }
}

static void DrawStrokeLinearClosed(const Stroke *s, int loopCount, float thickness,
                                   Color color) {
  if (loopCount < 2)
    return;
  for (int i = 0; i < loopCount; i++) {
    int j = (i + 1) % loopCount;
    DrawLineEx((Vector2){s->points[i].x, s->points[i].y},
               (Vector2){s->points[j].x, s->points[j].y}, thickness, color);
  }
}

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

  if (StrokeLooksLikeArrow(s)) {
    DrawArrowStroke(s, thickness, color);
    return;
  }

  bool closed = StrokeIsClosed(s);
  if (!closed) {
    if (s->pointCount < 6)
      DrawStrokeLinearOpen(s, thickness, color);
    else
      DrawSplineCatmullRom((const Vector2 *)s->points, s->pointCount, thickness,
                           color);
    return;
  }

  int loopCount = s->pointCount;
  if (loopCount > 3) {
    Point first = s->points[0];
    Point last = s->points[loopCount - 1];
    float dx = first.x - last.x;
    float dy = first.y - last.y;
    if ((dx * dx + dy * dy) <= 0.0001f)
      loopCount -= 1;
  }

  if (loopCount < 4) {
    DrawStrokeLinearClosed(s, loopCount, thickness, color);
    return;
  }

  // raylib's DrawSplineCatmullRom() is an "open" spline and won't connect the
  // last point back to the first; draw the loop with spline segments instead.
  for (int i = 0; i < loopCount; i++) {
    int i0 = (i - 1 + loopCount) % loopCount;
    int i1 = i;
    int i2 = (i + 1) % loopCount;
    int i3 = (i + 2) % loopCount;
    DrawSplineSegmentCatmullRom(PointAsVector2(s->points[i0]),
                                PointAsVector2(s->points[i1]),
                                PointAsVector2(s->points[i2]),
                                PointAsVector2(s->points[i3]), thickness, color);
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

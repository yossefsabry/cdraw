#include "canvas.h"
#include "raymath.h"
#include <math.h>
#include <stdint.h>

static Vector2 PointAsVector2(Point p) { return (Vector2){p.x, p.y}; }

static uint32_t HashU32(uint32_t x) {
  x ^= x >> 16;
  x *= 0x7feb352dU;
  x ^= x >> 15;
  x *= 0x846ca68bU;
  x ^= x >> 16;
  return x;
}

static uint32_t FloatBits(float f) {
  union {
    float f;
    uint32_t u;
  } v;
  v.f = f;
  return v.u;
}

static float HashToSignedFloat(uint32_t x) {
  // Convert to [-1, 1] using 24 bits of precision to keep it stable on all
  // platforms/compilers.
  float v = (float)(x & 0x00FFFFFFu) / (float)0x00FFFFFFu; // [0, 1]
  return v * 2.0f - 1.0f;
}

static float SmoothStep(float t) { return t * t * (3.0f - 2.0f * t); }

static float ValueNoise1D(uint32_t seed, float x) {
  float fx = floorf(x);
  int xi = (int)fx;
  float t = x - fx;
  float a = HashToSignedFloat(HashU32(seed ^ (uint32_t)xi));
  float b = HashToSignedFloat(HashU32(seed ^ (uint32_t)(xi + 1)));
  return a + (b - a) * SmoothStep(t);
}

static uint32_t StrokeSeed(const Stroke *s) {
  uint32_t seed = 0xC0FFEE11u;
  seed ^= (uint32_t)s->color.r | ((uint32_t)s->color.g << 8) |
          ((uint32_t)s->color.b << 16) | ((uint32_t)s->color.a << 24);
  if (s->pointCount > 0) {
    seed ^= HashU32(FloatBits(s->points[0].x));
    seed ^= HashU32(FloatBits(s->points[0].y));
  }
  if (seed == 0)
    seed = 1u;
  return seed;
}

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

static void DrawStrokePolylineRound(const Point *points, int pointCount, bool closed,
                                    float thickness, Color color) {
  if (pointCount < 2)
    return;

  float radius = thickness * 0.5f;
  Vector2 first = PointAsVector2(points[0]);
  Vector2 prev = first;
  DrawCircleV(prev, radius, color);

  for (int i = 1; i < pointCount; i++) {
    Vector2 curr = PointAsVector2(points[i]);
    DrawLineEx(prev, curr, thickness, color);
    DrawCircleV(curr, radius, color);
    prev = curr;
  }

  if (closed) {
    DrawLineEx(prev, first, thickness, color);
  }
}

static Vector2 JitterPoint(const Point *points, int pointCount, bool closed, int i,
                           float dist, float amplitude, float wavelength,
                           uint32_t seed) {
  int prevIndex = i - 1;
  int nextIndex = i + 1;
  if (closed) {
    if (prevIndex < 0)
      prevIndex = pointCount - 1;
    if (nextIndex >= pointCount)
      nextIndex = 0;
  } else {
    if (prevIndex < 0)
      prevIndex = 0;
    if (nextIndex >= pointCount)
      nextIndex = pointCount - 1;
  }

  Vector2 pPrev = PointAsVector2(points[prevIndex]);
  Vector2 pNext = PointAsVector2(points[nextIndex]);
  Vector2 dir = Vector2Subtract(pNext, pPrev);
  float len = Vector2Length(dir);
  if (len <= 0.0001f)
    dir = (Vector2){1.0f, 0.0f};
  else
    dir = Vector2Scale(dir, 1.0f / len);

  Vector2 perp = (Vector2){-dir.y, dir.x};

  float x = (wavelength <= 0.0001f) ? dist : (dist / wavelength);
  float n = ValueNoise1D(seed, x);
  float m = ValueNoise1D(seed ^ 0xA511E9B3u, x + 17.0f);

  Vector2 p = PointAsVector2(points[i]);
  p = Vector2Add(p, Vector2Scale(perp, n * amplitude));
  p = Vector2Add(p, Vector2Scale(dir, m * amplitude * 0.20f));
  return p;
}

static void DrawStrokeSketchyPass(const Point *points, int pointCount, bool closed,
                                  float thickness, Color color, uint32_t seed,
                                  float amplitude, float wavelength) {
  if (pointCount < 2)
    return;

  float radius = thickness * 0.5f;
  float dist = 0.0f;

  Vector2 first = JitterPoint(points, pointCount, closed, 0, 0.0f, amplitude,
                              wavelength, seed);
  Vector2 prev = first;
  DrawCircleV(prev, radius, color);

  for (int i = 1; i < pointCount; i++) {
    Vector2 a = PointAsVector2(points[i - 1]);
    Vector2 b = PointAsVector2(points[i]);
    dist += Vector2Distance(a, b);

    Vector2 curr = JitterPoint(points, pointCount, closed, i, dist, amplitude,
                               wavelength, seed);
    DrawLineEx(prev, curr, thickness, color);
    DrawCircleV(curr, radius, color);
    prev = curr;
  }

  if (closed) {
    DrawLineEx(prev, first, thickness, color);
  }
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

static void DrawStrokeSolid(const Stroke *s, float thickness, Color color) {
  if (s->pointCount < 2)
    return;

  if (StrokeLooksLikeArrow(s)) {
    DrawArrowStroke(s, thickness, color);
    return;
  }

  bool closed = StrokeIsClosed(s);
  int loopCount = s->pointCount;
  if (closed && loopCount > 3) {
    Point first = s->points[0];
    Point last = s->points[loopCount - 1];
    float dx = first.x - last.x;
    float dy = first.y - last.y;
    if ((dx * dx + dy * dy) <= 0.0001f)
      loopCount -= 1;
  }

  if (closed && loopCount < 4) {
    DrawStrokeLinearClosed(s, loopCount, thickness, color);
    return;
  }

  DrawStrokePolylineRound(s->points, closed ? loopCount : s->pointCount, closed,
                          thickness, color);
}

static void DrawStroke(const Stroke *s, float thickness, Color color) {
  if (s->pointCount < 2)
    return;

  if (StrokeLooksLikeArrow(s)) {
    DrawArrowStroke(s, thickness, color);
    return;
  }

  bool closed = StrokeIsClosed(s);
  int loopCount = s->pointCount;
  if (closed && loopCount > 3) {
    Point first = s->points[0];
    Point last = s->points[loopCount - 1];
    float dx = first.x - last.x;
    float dy = first.y - last.y;
    if ((dx * dx + dy * dy) <= 0.0001f)
      loopCount -= 1;
  }

  if (closed && loopCount < 4) {
    DrawStrokeLinearClosed(s, loopCount, thickness, color);
    return;
  }

  uint32_t seed = StrokeSeed(s);
  float amp = fmaxf(0.35f, thickness * 0.18f);
  float wavelength = fmaxf(10.0f, thickness * 2.5f);

  DrawStrokeSketchyPass(s->points, closed ? loopCount : s->pointCount, closed,
                        thickness, color, seed, amp, wavelength);
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
    DrawStrokeSolid(s, s->thickness + 2.0f, canvas->selectionColor);
  }

  if (canvas->isDrawing)
    DrawStroke(&canvas->currentStroke, canvas->currentStroke.thickness,
               canvas->currentStroke.color);

  EndMode2D();
}

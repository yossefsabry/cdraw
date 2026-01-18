#include "canvas_internal.h"
#include "raymath.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>

static float DistPointSegSq(Vector2 p, Vector2 a, Vector2 b) {
  Vector2 ab = Vector2Subtract(b, a);
  float abLen2 = Vector2DotProduct(ab, ab);
  if (abLen2 <= 0.000001f)
    return Vector2DistanceSqr(p, a);
  float t = Vector2DotProduct(Vector2Subtract(p, a), ab) / abLen2;
  t = Clamp(t, 0.0f, 1.0f);
  Vector2 proj = Vector2Add(a, Vector2Scale(ab, t));
  return Vector2DistanceSqr(p, proj);
}

static float StrokeMinDistSq(const Stroke *s, Vector2 p) {
  if (s->pointCount == 0)
    return FLT_MAX;
  if (s->pointCount == 1)
    return Vector2DistanceSqr(p, (Vector2){s->points[0].x, s->points[0].y});
  float best = FLT_MAX;
  for (int i = 0; i < s->pointCount - 1; i++) {
    Vector2 a = {s->points[i].x, s->points[i].y};
    Vector2 b = {s->points[i + 1].x, s->points[i + 1].y};
    float d2 = DistPointSegSq(p, a, b);
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
    float d2 = StrokeMinDistSq(s, p);
    float localR = radiusWorld;
    if (s->thickness > 0.0f)
      localR = fmaxf(localR, (s->thickness * 0.75f) / canvas->camera.zoom);
    if (d2 <= localR * localR && d2 <= bestD2) {
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
  free(canvas->strokes[index].points);
  for (int i = index; i < canvas->strokeCount - 1; i++)
    canvas->strokes[i] = canvas->strokes[i + 1];
  canvas->strokeCount--;
  if (canvas->selectedStrokeIndex == index)
    canvas->selectedStrokeIndex = -1;
  else if (canvas->selectedStrokeIndex > index)
    canvas->selectedStrokeIndex--;
}

void CanvasInputHandleEditTools(Canvas *canvas, bool inputCaptured, bool isPanning,
                                int activeTool) {
  if (!inputCaptured && activeTool == TOOL_SELECT) {
    Vector2 mouseWorld =
        GetScreenToWorld2D(GetMousePosition(), canvas->camera);
    float pickRadius = 8.0f / canvas->camera.zoom;

    if (!isPanning && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      int hit = FindStrokeHit(canvas, mouseWorld, pickRadius);
      canvas->selectedStrokeIndex = hit;
      canvas->isDraggingSelection = (hit >= 0);
      canvas->lastMouseWorld = mouseWorld;
    }

    if (!isPanning && canvas->isDraggingSelection && canvas->selectedStrokeIndex >= 0 &&
        IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
      Vector2 delta = Vector2Subtract(mouseWorld, canvas->lastMouseWorld);
      TranslateStroke(&canvas->strokes[canvas->selectedStrokeIndex], delta);
      canvas->lastMouseWorld = mouseWorld;
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
      canvas->isDraggingSelection = false;
    if (canvas->selectedStrokeIndex >= 0 && IsKeyPressed(KEY_DELETE))
      RemoveStrokeAtIndex(canvas, canvas->selectedStrokeIndex);
  }

  if (!inputCaptured && activeTool == TOOL_ERASER && !isPanning &&
      IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
    Vector2 mouseWorld =
        GetScreenToWorld2D(GetMousePosition(), canvas->camera);
    float radiusWorld = (8.0f + canvas->currentStroke.thickness) /
                        fmaxf(canvas->camera.zoom, 0.001f);
    for (int i = canvas->strokeCount - 1; i >= 0; i--) {
      float d2 = StrokeMinDistSq(&canvas->strokes[i], mouseWorld);
      if (d2 <= radiusWorld * radiusWorld)
        RemoveStrokeAtIndex(canvas, i);
    }
  }
}


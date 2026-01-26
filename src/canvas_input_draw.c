#include "canvas_internal.h"
#include "raymath.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void AddPoint(Stroke *stroke, Point p) {
  if (stroke->pointCount >= stroke->capacity) {
    stroke->capacity = (stroke->capacity == 0) ? 64 : stroke->capacity * 2;
    stroke->points =
        (Point *)realloc(stroke->points, sizeof(Point) * (size_t)stroke->capacity);
  }
  stroke->points[stroke->pointCount++] = p;
}

static float ClampFloat(float v, float min, float max) {
  if (v < min)
    return min;
  if (v > max)
    return max;
  return v;
}

static float PenWidthFromSpeed(float base, float speed) {
  float minW = fmaxf(1.0f, base * 0.70f);
  float maxW = fmaxf(minW + 0.4f, base * 1.40f);
  float slow = 60.0f;
  float fast = 1400.0f;
  float t = ClampFloat((speed - slow) / (fast - slow), 0.0f, 1.0f);
  return maxW + (minW - maxW) * t;
}

static void CreateCircleStroke(Stroke *s, Point start, Point end) {
  Vector2 c = {start.x, start.y};
  float dx = end.x - start.x;
  float dy = end.y - start.y;
  float radius = sqrtf(dx * dx + dy * dy);

  const int segments = 64;
  for (int i = 0; i < segments; i++) {
    float a = (float)i / (float)segments * PI * 2.0f;
    AddPoint(s, (Point){c.x + cosf(a) * radius, c.y + sinf(a) * radius});
  }

  // Close the loop exactly (avoid float drift between 0 and 2*PI).
  if (s->pointCount > 0)
    AddPoint(s, s->points[0]);
}

static void CreateArrowStroke(Stroke *s, Point start, Point end, float thickness) {
  Vector2 a = {start.x, start.y};
  Vector2 b = {end.x, end.y};
  Vector2 ab = Vector2Subtract(b, a);
  float len = Vector2Length(ab);

  AddPoint(s, start);
  if (len <= 0.0001f) {
    AddPoint(s, end);
    return;
  }

  Vector2 dir = Vector2Scale(ab, 1.0f / len);
  Vector2 perp = (Vector2){-dir.y, dir.x};

  float headLen = fmaxf(16.0f, thickness * 4.0f);
  headLen = fminf(headLen, len * 0.5f);
  float headW = fmaxf(thickness * 3.0f, headLen * 1.10f);

  Vector2 base = Vector2Subtract(b, Vector2Scale(dir, headLen));
  Vector2 left = Vector2Add(base, Vector2Scale(perp, headW * 0.5f));
  Vector2 right = Vector2Subtract(base, Vector2Scale(perp, headW * 0.5f));

  AddPoint(s, end);
  AddPoint(s, (Point){left.x, left.y});
  AddPoint(s, (Point){right.x, right.y});
  AddPoint(s, end);
}

void CanvasInputHandleDrawTools(Canvas *canvas, bool inputCaptured, bool isPanning,
                                int activeTool) {
  bool drawTool = activeTool == TOOL_PEN || activeTool == TOOL_LINE ||
                  activeTool == TOOL_RECT || activeTool == TOOL_CIRCLE;
  bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
  bool drawingInput =
      drawTool && IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !isPanning && !inputCaptured;

  if (drawingInput) {
    Vector2 m = GetScreenToWorld2D(GetMousePosition(), canvas->camera);
    Point p = {m.x, m.y};
    if (!canvas->isDrawing) {
      canvas->isDrawing = true;
      canvas->currentStroke.pointCount = 0;
      canvas->currentStroke.capacity = 0;
      canvas->currentStroke.points = NULL;
      canvas->currentStroke.usePressure = (activeTool == TOOL_PEN);
      if (canvas->currentStroke.thickness == 0)
        canvas->currentStroke.thickness = 3.0f;
      float startWidth =
          canvas->currentStroke.usePressure ? canvas->currentStroke.thickness : 0.0f;
      canvas->startPoint = (Point){p.x, p.y, startWidth};
      AddPoint(&canvas->currentStroke, (Point){p.x, p.y, startWidth});
      fprintf(stderr, "Start Drawing. Tool: %d at %.1f, %.1f\n", activeTool, p.x,
              p.y);
      return;
    }

    Stroke *s = &canvas->currentStroke;
    if (activeTool == TOOL_PEN && shift) {
      s->pointCount = 0;
      Point start = canvas->startPoint;
      float width = canvas->currentStroke.thickness;
      AddPoint(s, (Point){start.x, start.y, width});
      AddPoint(s, (Point){p.x, p.y, width});
      return;
    }
    if (activeTool == TOOL_PEN) {
      Point last = s->points[s->pointCount - 1];
      float dx = p.x - last.x;
      float dy = p.y - last.y;
      float base = canvas->currentStroke.thickness;
      float distSq = dx * dx + dy * dy;
      float minDist = fmaxf(0.75f, base * 0.2f);
      if (distSq > minDist * minDist) {
        float dist = sqrtf(distSq);
        float dt = fmaxf(GetFrameTime(), 0.0001f);
        float speed = dist / dt;
        float width = PenWidthFromSpeed(base, speed);
        AddPoint(s, (Point){p.x, p.y, width});
        if (s->pointCount == 2)
          s->points[0].width = (s->points[0].width + width) * 0.5f;
      }
      return;
    }

    s->pointCount = 0;
    Point start = canvas->startPoint;
    if (activeTool == TOOL_LINE) {
      CreateArrowStroke(s, start, p, canvas->currentStroke.thickness);
    } else if (activeTool == TOOL_RECT) {
      AddPoint(s, start);
      AddPoint(s, (Point){p.x, start.y});
      AddPoint(s, (Point){p.x, p.y});
      AddPoint(s, (Point){start.x, p.y});
      AddPoint(s, start);
    } else if (activeTool == TOOL_CIRCLE) {
      CreateCircleStroke(s, start, p);
    }
    return;
  }

  if (!canvas->isDrawing)
    return;

  canvas->isDrawing = false;
  if (canvas->currentStroke.pointCount > 1) {
    AddStroke(canvas, canvas->currentStroke);
    fprintf(stderr, "Finished Stroke. Points: %d\n", canvas->currentStroke.pointCount);
    canvas->currentStroke.points = NULL;
    canvas->currentStroke.pointCount = 0;
    canvas->currentStroke.capacity = 0;
  } else {
    free(canvas->currentStroke.points);
    canvas->currentStroke.points = NULL;
    canvas->currentStroke.pointCount = 0;
  }
}

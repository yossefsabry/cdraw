#include "canvas_internal.h"
#include "raymath.h"
#include <math.h>

void UpdateCanvasState(Canvas *canvas, bool inputCaptured, int activeTool) {
  float wheel = GetMouseWheelMove();
  bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
  bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
  if (wheel != 0 && !inputCaptured && !(ctrl && shift)) {
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
  if (!inputCaptured && IsKeyDown(KEY_SPACE) &&
      IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    isPanning = true;
  if (activeTool == TOOL_PAN && IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
      !inputCaptured)
    isPanning = true;

  if (isPanning) {
    Vector2 delta = GetMouseDelta();
    delta = Vector2Scale(delta, -1.0f / canvas->camera.zoom);
    canvas->camera.target = Vector2Add(canvas->camera.target, delta);
  }

  CanvasInputHandleEditTools(canvas, inputCaptured, isPanning, activeTool);
  CanvasInputHandleDrawTools(canvas, inputCaptured, isPanning, activeTool);
}


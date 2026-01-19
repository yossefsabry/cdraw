#include "view_reset.h"

void ResetCanvasView(Canvas *canvas) {
  if (!canvas)
    return;
  canvas->camera.target = (Vector2){0.0f, 0.0f};
  canvas->camera.offset =
      (Vector2){GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
  canvas->camera.rotation = 0.0f;
  canvas->camera.zoom = 1.0f;

  canvas->isDraggingSelection = false;
}


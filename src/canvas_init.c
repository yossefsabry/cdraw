#include "canvas.h"
#include <stdlib.h>

void InitCanvas(Canvas *canvas, int screenWidth, int screenHeight) {
  canvas->strokeCount = 0;
  canvas->capacity = 1000;
  canvas->strokes = (Stroke *)malloc(sizeof(Stroke) * (size_t)canvas->capacity);

  canvas->redoCount = 0;
  canvas->redoCapacity = 1000;
  canvas->redoStrokes =
      (Stroke *)malloc(sizeof(Stroke) * (size_t)canvas->redoCapacity);

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
  for (int i = 0; i < canvas->strokeCount; i++)
    free(canvas->strokes[i].points);
  free(canvas->strokes);

  for (int i = 0; i < canvas->redoCount; i++)
    free(canvas->redoStrokes[i].points);
  free(canvas->redoStrokes);

  free(canvas->currentStroke.points);
  canvas->currentStroke.points = NULL;
}


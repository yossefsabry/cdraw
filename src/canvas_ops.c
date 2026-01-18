#include "canvas.h"
#include <stdio.h>
#include <stdlib.h>

static void ClearRedo(Canvas *canvas) {
  for (int i = 0; i < canvas->redoCount; i++)
    free(canvas->redoStrokes[i].points);
  canvas->redoCount = 0;
}

void AddStroke(Canvas *canvas, Stroke stroke) {
  ClearRedo(canvas);
  if (canvas->strokeCount >= canvas->capacity) {
    int newCap = (canvas->capacity == 0) ? 64 : canvas->capacity * 2;
    canvas->strokes =
        (Stroke *)realloc(canvas->strokes, sizeof(Stroke) * (size_t)newCap);
    canvas->capacity = newCap;
  }
  canvas->strokes[canvas->strokeCount++] = stroke;
  canvas->totalPoints += stroke.pointCount;
}

void Undo(Canvas *canvas) {
  if (canvas->strokeCount <= 0)
    return;
  Stroke s = canvas->strokes[--canvas->strokeCount];
  canvas->totalPoints -= s.pointCount;
  if (canvas->redoCount >= canvas->redoCapacity) {
    int newCap = (canvas->redoCapacity == 0) ? 64 : canvas->redoCapacity * 2;
    canvas->redoStrokes =
        (Stroke *)realloc(canvas->redoStrokes, sizeof(Stroke) * (size_t)newCap);
    canvas->redoCapacity = newCap;
  }
  canvas->redoStrokes[canvas->redoCount++] = s;
  if (canvas->selectedStrokeIndex >= canvas->strokeCount)
    canvas->selectedStrokeIndex = -1;
  fprintf(stderr, "Action Undone. Strokes: %d\n", canvas->strokeCount);
}

void Redo(Canvas *canvas) {
  if (canvas->redoCount <= 0)
    return;
  Stroke s = canvas->redoStrokes[--canvas->redoCount];
  if (canvas->strokeCount >= canvas->capacity) {
    int newCap = (canvas->capacity == 0) ? 64 : canvas->capacity * 2;
    canvas->strokes =
        (Stroke *)realloc(canvas->strokes, sizeof(Stroke) * (size_t)newCap);
    canvas->capacity = newCap;
  }
  canvas->strokes[canvas->strokeCount++] = s;
  canvas->totalPoints += s.pointCount;
  fprintf(stderr, "Action Redone. Strokes: %d\n", canvas->strokeCount);
}

void ClearCanvas(Canvas *canvas) {
  for (int i = 0; i < canvas->strokeCount; i++)
    free(canvas->strokes[i].points);
  canvas->strokeCount = 0;
  canvas->totalPoints = 0;
  ClearRedo(canvas);

  canvas->selectedStrokeIndex = -1;
  canvas->isDraggingSelection = false;

  free(canvas->currentStroke.points);
  canvas->currentStroke.points = NULL;
  canvas->currentStroke.pointCount = 0;
  canvas->currentStroke.capacity = 0;
  canvas->isDrawing = false;

  fprintf(stderr, "Canvas Cleared.\n");
}

int GetTotalPoints(const Canvas *canvas) {
  int total = canvas->totalPoints;
  if (canvas->isDrawing)
    total += canvas->currentStroke.pointCount;
  return total;
}

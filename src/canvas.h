#ifndef CANVAS_H
#define CANVAS_H

#include "raylib.h"

// Define available tools here to be shared
typedef enum {
  TOOL_PEN = 0,
  TOOL_ERASER,
  TOOL_PAN,
  TOOL_LINE,
  TOOL_RECT,
  TOOL_CIRCLE
} ToolType;

typedef struct {
  float x;
  float y;
} Point;

typedef struct {
  Point *points;
  int pointCount;
  int capacity;
  Color color;
  float thickness;
} Stroke;

typedef struct {
  Stroke *strokes;
  int strokeCount;
  int capacity;

  // Redo Stack
  Stroke *redoStrokes;
  int redoCount;
  int redoCapacity;

  Camera2D camera;

  // Drawing state
  bool isDrawing;
  Point startPoint; // Store start point explicitly for shapes
  bool showGrid;
  Stroke currentStroke;
} Canvas;

void InitCanvas(Canvas *canvas, int screenWidth, int screenHeight);
void FreeCanvas(Canvas *canvas);
void UpdateCanvasState(Canvas *canvas, bool inputCaptured, int activeTool);
void DrawCanvas(Canvas *canvas);
void AddStroke(Canvas *canvas, Stroke stroke);
void Undo(Canvas *canvas);
void Redo(Canvas *canvas);
void ClearCanvas(Canvas *canvas);

#endif // CANVAS_H

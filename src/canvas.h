#ifndef CANVAS_H
#define CANVAS_H

#include "raylib.h"
#include <stdint.h>

#define DEFAULT_ZOOM 5.0f

// Define available tools here to be shared
typedef enum {
  TOOL_PEN = 0,
  TOOL_ERASER,
  TOOL_SELECT,
  TOOL_PAN,
  TOOL_LINE,
  TOOL_RECT,
  TOOL_CIRCLE
} ToolType;

typedef struct {
  float x;
  float y;
  float width;
} Point;

typedef struct {
  Point *points;
  int pointCount;
  int capacity;
  Color color;
  float thickness;
  bool usePressure;
  Point *cachedPoints;
  int cachedCount;
  int cachedCapacity;
  uint32_t cacheVersion;
  uint32_t lastBuiltVersion;
  bool cacheDirty;
} Stroke;

typedef struct {
  Stroke *strokes;
  int strokeCount;
  int capacity;
  int totalPoints;

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

  // Theme
  Color backgroundColor;
  Color gridColor;
  Color selectionColor;

  // Selection
  int selectedStrokeIndex;
  bool isDraggingSelection;
  Vector2 lastMouseWorld;
} Canvas;

void InitCanvas(Canvas *canvas, int screenWidth, int screenHeight);
void FreeCanvas(Canvas *canvas);
void UpdateCanvasState(Canvas *canvas, bool inputCaptured, int activeTool);
void DrawCanvas(Canvas *canvas);
void AddStroke(Canvas *canvas, Stroke stroke);
void Undo(Canvas *canvas);
void Redo(Canvas *canvas);
void ClearCanvas(Canvas *canvas);
bool SaveCanvasToFile(const Canvas *canvas, const char *path);
bool LoadCanvasFromFile(Canvas *canvas, const char *path);
int GetTotalPoints(const Canvas *canvas);

#endif // CANVAS_H

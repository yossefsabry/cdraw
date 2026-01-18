#ifndef CANVAS_INTERNAL_H
#define CANVAS_INTERNAL_H

#include "canvas.h"

void CanvasInputHandleDrawTools(Canvas *canvas, bool inputCaptured, bool isPanning,
                                int activeTool);
void CanvasInputHandleEditTools(Canvas *canvas, bool inputCaptured, bool isPanning,
                                int activeTool);

#endif

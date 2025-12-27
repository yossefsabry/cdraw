#ifndef GUI_H
#define GUI_H

#include "canvas.h"
#include "raylib.h"

// ToolType is defined in canvas.h

typedef struct {
  int activeTool;
  Color currentColor;
  float currentThickness;
  Rectangle toolbarRect;

  // Slider State
  bool isDraggingSize;

  // Palette State
  Rectangle paletteRect;

  // Color Picker State
  bool showColorPicker;
  Rectangle colorPickerRect;
  float hueValue;
  float satValue;
  float valValue;
} GuiState;

void InitGui(GuiState *gui);
void BaseLoadIcons(GuiState *gui);
void UnloadGui(GuiState *gui);
void DrawGui(GuiState *gui, Canvas *canvas);
void UpdateGui(GuiState *gui, Canvas *canvas);
bool IsMouseOverGui(GuiState *gui);

#endif // GUI_H

#ifndef GUI_H
#define GUI_H

#include "canvas.h"
#include "raylib.h"

// ToolType is defined in canvas.h

typedef struct {
  Texture2D add;
  Texture2D circle;
  Texture2D close;
  Texture2D colorPicker;
  Texture2D edit;
  Texture2D eraser;
  Texture2D fullscreen;
  Texture2D grid;
  Texture2D line;
  Texture2D menu;
  Texture2D openFile;
  Texture2D rectangle;
  Texture2D redo;
  Texture2D saveFile;
  Texture2D select;
  Texture2D undo;
  Texture2D brush;
} GuiIcons;

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

  // UI State
  bool darkMode;
  bool showMenu;
  Rectangle menuRect;
  bool requestExit;

  char currentFile[256];
  char toast[128];
  double toastUntil;

  GuiIcons icons;
} GuiState;

void InitGui(GuiState *gui);
void BaseLoadIcons(GuiState *gui);
void UnloadGui(GuiState *gui);
void DrawGui(GuiState *gui, Canvas *canvas);
void UpdateGui(GuiState *gui, Canvas *canvas);
bool IsMouseOverGui(GuiState *gui);

#endif // GUI_H

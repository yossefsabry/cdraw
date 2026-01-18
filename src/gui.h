#ifndef GUI_H
#define GUI_H

#include "canvas.h"
#include "raylib.h"

// ToolType is defined in canvas.h

typedef struct {
  Texture2D menu;
  Texture2D add;
  Texture2D openFile;
  Texture2D saveFile;
  Texture2D undo;
  Texture2D redo;

  Texture2D pen;
  Texture2D rect;
  Texture2D circle;
  Texture2D line;
  Texture2D eraser;
  Texture2D select;
  Texture2D pan;

  Texture2D grid;
  Texture2D fullscreen;

  Texture2D colorPicker;

  Texture2D darkMode;
  Texture2D lightMode;

  Texture2D windowMinimize;
  Texture2D windowToggleSize;
  Texture2D windowClose;
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
  Rectangle paletteButtonRect;

  // Color Picker State
  bool showColorPicker;
  Rectangle colorPickerRect;
  Rectangle colorButtonRect;
  float hueValue;
  float satValue;
  float valValue;

  // UI State
  bool darkMode;
  bool showMenu;
  Rectangle menuRect;
  Rectangle menuButtonRect;
  bool requestExit;

  char currentFile[256];
  char toast[128];
  double toastUntil;

  GuiIcons icons;
} GuiState;

void InitGui(GuiState *gui);
void UnloadGui(GuiState *gui);
void DrawGui(GuiState *gui, Canvas *canvas);
void UpdateGui(GuiState *gui, Canvas *canvas);
bool IsMouseOverGui(GuiState *gui);

#endif // GUI_H

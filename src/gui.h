#ifndef GUI_H
#define GUI_H

#include "canvas.h"
#include "raylib.h"

// ToolType is defined in canvas.h

typedef struct {
  Rectangle src;
} GuiIcon;

typedef struct {
  Texture2D atlas;

  GuiIcon menu;
  GuiIcon add;
  GuiIcon openFile;
  GuiIcon saveFile;
  GuiIcon undo;
  GuiIcon redo;

  GuiIcon pen;
  GuiIcon rect;
  GuiIcon circle;
  GuiIcon line;
  GuiIcon eraser;
  GuiIcon select;
  GuiIcon pan;

  GuiIcon grid;
  GuiIcon fullscreen;

  GuiIcon colorPicker;

  GuiIcon darkMode;
  GuiIcon lightMode;

  GuiIcon windowMinimize;
  GuiIcon windowToggleSize;
  GuiIcon windowClose;
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
  bool isTyping;

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
void UpdateCursor(GuiState *gui, const Canvas *canvas, bool mouseOverGui);

#endif // GUI_H

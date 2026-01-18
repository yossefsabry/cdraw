#include "gui_internal.h"
#include "raymath.h"

void UpdateGui(GuiState *gui, Canvas *canvas) {
  Theme t = GuiThemeGet(gui->darkMode);

  canvas->backgroundColor = t.canvas;
  canvas->gridColor = t.grid;
  canvas->selectionColor = t.primary;

  bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
  bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

  float wheel = GetMouseWheelMove();
  if (ctrl && shift && wheel != 0.0f) {
    gui->currentThickness = Clamp(gui->currentThickness + wheel, 1.0f, 21.0f);
  }

  canvas->currentStroke.color = gui->currentColor;
  canvas->currentStroke.thickness = gui->currentThickness;

  if (ctrl && IsKeyPressed(KEY_Z))
    Undo(canvas);
  if (ctrl && IsKeyPressed(KEY_Y))
    Redo(canvas);
  if (ctrl && IsKeyPressed(KEY_S))
    GuiToastSet(gui, SaveCanvasToFile(canvas, gui->currentFile) ? "Saved."
                                                               : "Save failed.");
  if (ctrl && IsKeyPressed(KEY_O))
    GuiToastSet(gui, LoadCanvasFromFile(canvas, gui->currentFile) ? "Loaded."
                                                                 : "Load failed.");
  if (ctrl && IsKeyPressed(KEY_N)) {
    ClearCanvas(canvas);
    GuiToastSet(gui, "New canvas.");
  }

  if (IsKeyPressed(KEY_G))
    canvas->showGrid = !canvas->showGrid;
  if (IsKeyPressed(KEY_F))
    ToggleFullscreen();
  if (IsKeyPressed(KEY_ESCAPE)) {
    gui->showMenu = false;
    gui->showColorPicker = false;
  }
}


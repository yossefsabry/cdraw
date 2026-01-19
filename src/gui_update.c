#include "gui_internal.h"
#include "raymath.h"

void UpdateGui(GuiState *gui, Canvas *canvas) {
  Theme t = GuiThemeGet(gui->darkMode);

  canvas->backgroundColor = t.canvas;
  canvas->gridColor = t.grid;
  canvas->selectionColor = t.primary;

  bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
  bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
  bool alt = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);

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

  if (!gui->isTyping && !ctrl && !alt) {
    typedef struct {
      KeyboardKey key;
      int tool;
    } ToolShortcut;

    static const ToolShortcut toolShortcuts[] = {
        {KEY_P, TOOL_PEN},      {KEY_B, TOOL_RECT},   {KEY_C, TOOL_CIRCLE},
        {KEY_A, TOOL_LINE},     {KEY_E, TOOL_ERASER}, {KEY_M, TOOL_SELECT},
    };

    const int count =
        (int)(sizeof(toolShortcuts) / sizeof(toolShortcuts[0]));
    for (int i = 0; i < count; i++) {
      if (IsKeyPressed(toolShortcuts[i].key)) {
        gui->activeTool = toolShortcuts[i].tool;
        break;
      }
    }
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

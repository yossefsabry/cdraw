#include "gui_internal.h"

static bool MenuItem(Rectangle r, const char *label, Theme t) {
  Vector2 mouse = GetMousePosition();
  bool hover = CheckCollisionPointRec(mouse, r);
  if (hover)
    DrawRectangleRec(r, t.hover);
  DrawText(label, (int)r.x + 10, (int)r.y + 7, 12, hover ? t.text : t.textDim);
  return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void GuiDrawMenu(GuiState *gui, Canvas *canvas, Theme t) {
  if (!gui->showMenu)
    return;

  DrawRectangleRec(gui->menuRect, t.surface);
  DrawRectangleLinesEx(gui->menuRect, 1, t.border);

  float x = gui->menuRect.x;
  float y = gui->menuRect.y;
  float w = gui->menuRect.width;
  float h = 26;

  if (MenuItem((Rectangle){x, y + h * 0, w, h}, "New (Ctrl+N)", t)) {
    ClearCanvas(canvas);
    GuiToastSet(gui, "New canvas.");
    gui->showMenu = false;
  }
  if (MenuItem((Rectangle){x, y + h * 1, w, h}, "Open (Ctrl+O)", t)) {
    GuiToastSet(gui,
               LoadCanvasFromFile(canvas, gui->currentFile) ? "Loaded."
                                                           : "Load failed.");
    gui->showMenu = false;
  }
  if (MenuItem((Rectangle){x, y + h * 2, w, h}, "Save (Ctrl+S)", t)) {
    GuiToastSet(gui, SaveCanvasToFile(canvas, gui->currentFile) ? "Saved."
                                                               : "Save failed.");
    gui->showMenu = false;
  }
  if (MenuItem((Rectangle){x, y + h * 3, w, h}, "Toggle Grid (G)", t)) {
    canvas->showGrid = !canvas->showGrid;
    gui->showMenu = false;
  }
  if (MenuItem((Rectangle){x, y + h * 4, w, h},
               gui->darkMode ? "Light Mode" : "Dark Mode", t)) {
    gui->darkMode = !gui->darkMode;
    GuiToastSet(gui, gui->darkMode ? "Dark mode." : "Light mode.");
    gui->showMenu = false;
  }
  if (MenuItem((Rectangle){x, y + h * 5, w, h}, "Exit", t)) {
    gui->requestExit = true;
    gui->showMenu = false;
  }

  Vector2 mouse = GetMousePosition();
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
      !CheckCollisionPointRec(mouse, gui->menuRect) &&
      !CheckCollisionPointRec(mouse, gui->menuButtonRect)) {
    gui->showMenu = false;
  }
}


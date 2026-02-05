#include "gui_internal.h"
#include <stddef.h>

static bool MenuItem(GuiState *gui, Rectangle r, const char *label,
                     const char *shortcut, Theme t, bool destructive) {
  Vector2 mouse = GetMousePosition();
  bool hover = CheckCollisionPointRec(mouse, r);
  Color bg = hover ? t.hover : (Color){0, 0, 0, 0};
  if (bg.a > 0)
    DrawRectangleRec(r, bg);

  Color labelColor = destructive ? (Color){239, 68, 68, 255}
                                 : (hover ? t.text : t.textDim);
  DrawTextEx(gui->uiFont, label, (Vector2){r.x + 12, r.y + 7}, 12, 1.0f,
             labelColor);

  if (shortcut && shortcut[0] != '\0') {
    Vector2 size = MeasureTextEx(gui->uiFont, shortcut, 11, 1.0f);
    DrawTextEx(gui->uiFont, shortcut,
               (Vector2){r.x + r.width - size.x - 12, r.y + 7}, 11, 1.0f,
               t.textDim);
  }
  return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static void MenuDivider(float x, float y, float w, Theme t) {
  DrawLineEx((Vector2){x + 10, y}, (Vector2){x + w - 10, y}, 1.0f, t.border);
}

void GuiDrawMenu(GuiState *gui, Canvas *canvas, Theme t) {
  if (!gui->showMenu)
    return;

  const float pad = 8.0f;
  const float titleH = 28.0f;
  const float itemH = 28.0f;
  const float dividerH = 10.0f;

  const int itemCount = 7;
  const int dividerCount = 2;
  float w = 230.0f;
  float h = titleH + pad + itemCount * itemH + dividerCount * dividerH + pad;

  gui->menuRect.width = w;
  gui->menuRect.height = h;

  DrawRectangleRounded(gui->menuRect, 0.15f, 8, t.surface);
  DrawRectangleRoundedLinesEx(gui->menuRect, 0.15f, 8, 1.0f, t.border);

  float x = gui->menuRect.x;
  float y = gui->menuRect.y;

  DrawTextEx(gui->uiFont, "Menu", (Vector2){x + pad + 2, y + 7}, 12, 1.0f,
             t.text);
  MenuDivider(x, y + titleH, w, t);

  y += titleH + pad;
  if (MenuItem(gui, (Rectangle){x + pad, y, w - pad * 2, itemH}, "New tab",
               "Ctrl+N", t, false)) {
    Document *active = GuiGetActiveDocument(gui);
    bool showGrid = active ? active->canvas.showGrid : true;
    GuiAddDocument(gui, GetScreenWidth(), GetScreenHeight(), showGrid, true);
    GuiToastSet(gui, "New canvas.");
    gui->showMenu = false;
  }
  y += itemH;
  if (MenuItem(gui, (Rectangle){x + pad, y, w - pad * 2, itemH}, "Clear canvas",
               NULL, t, false)) {
    ClearCanvas(canvas);
    GuiMarkNewDocument(gui);
    GuiToastSet(gui, "Canvas cleared.");
    gui->showMenu = false;
  }
  y += itemH;
  if (MenuItem(gui, (Rectangle){x + pad, y, w - pad * 2, itemH}, "Open",
               "Ctrl+O", t, false)) {
    GuiRequestOpen(gui);
    gui->showMenu = false;
  }
  y += itemH;
  if (MenuItem(gui, (Rectangle){x + pad, y, w - pad * 2, itemH}, "Save",
               "Ctrl+S", t, false)) {
    GuiRequestSave(gui, GuiGetActiveDocument(gui));
    gui->showMenu = false;
  }

  y += itemH;
  MenuDivider(x, y + 2, w, t);
  y += dividerH;

  if (MenuItem(gui, (Rectangle){x + pad, y, w - pad * 2, itemH}, "Toggle grid",
               "G", t, false)) {
    canvas->showGrid = !canvas->showGrid;
    gui->showMenu = false;
  }
  y += itemH;
  if (MenuItem(gui, (Rectangle){x + pad, y, w - pad * 2, itemH},
               gui->darkMode ? "Light mode" : "Dark mode", NULL, t, false)) {
    gui->darkMode = !gui->darkMode;
    GuiToastSet(gui, gui->darkMode ? "Dark mode." : "Light mode.");
    gui->showMenu = false;
  }

  y += itemH;
  MenuDivider(x, y + 2, w, t);
  y += dividerH;
  if (MenuItem(gui, (Rectangle){x + pad, y, w - pad * 2, itemH}, "Exit", NULL, t,
               true)) {
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

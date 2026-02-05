#include "gui_internal.h"
#include <stddef.h>

static bool MenuItem(GuiState *gui, Rectangle r, const char *label,
                     const char *shortcut, Theme t, bool destructive,
                     bool *outHover) {
  Vector2 mouse = GetMousePosition();
  bool hover = CheckCollisionPointRec(mouse, r);
  if (outHover)
    *outHover = hover;
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
  if (!gui->showMenu) {
    gui->showExportMenu = false;
    return;
  }

  const float pad = 8.0f;
  const float titleH = 28.0f;
  const float itemH = 28.0f;
  const float dividerH = 10.0f;

  const int itemCount = 8;
  const int dividerCount = 3;
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
               "Ctrl+N", t, false, NULL)) {
    Document *active = GuiGetActiveDocument(gui);
    bool showGrid = active ? active->canvas.showGrid : true;
    GuiAddDocument(gui, GetScreenWidth(), GetScreenHeight(), showGrid, true);
    GuiToastSet(gui, "New canvas.");
    gui->showMenu = false;
  }
  y += itemH;
  if (MenuItem(gui, (Rectangle){x + pad, y, w - pad * 2, itemH}, "Clear canvas",
               NULL, t, false, NULL)) {
    ClearCanvas(canvas);
    GuiMarkNewDocument(gui);
    GuiToastSet(gui, "Canvas cleared.");
    gui->showMenu = false;
  }
  y += itemH;
  if (MenuItem(gui, (Rectangle){x + pad, y, w - pad * 2, itemH}, "Open",
               "Ctrl+O", t, false, NULL)) {
    GuiRequestOpen(gui);
    gui->showMenu = false;
  }
  y += itemH;
  if (MenuItem(gui, (Rectangle){x + pad, y, w - pad * 2, itemH}, "Save",
               "Ctrl+S", t, false, NULL)) {
    GuiRequestSave(gui, GuiGetActiveDocument(gui));
    gui->showMenu = false;
  }

  y += itemH;
  MenuDivider(x, y + 2, w, t);
  y += dividerH;

  bool exportHover = false;
  Rectangle exportRow = {x + pad, y, w - pad * 2, itemH};
  bool exportClicked =
      MenuItem(gui, exportRow, "Export", ">", t, false, &exportHover);
  if (exportClicked)
    gui->showExportMenu = true;

  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  const int exportItemCount = 6;
  float exportW = 220.0f;
  float exportH = titleH + pad + exportItemCount * itemH + pad;
  float exportX = x + w + 8.0f;
  float exportY = exportRow.y - pad - titleH;
  if (exportX + exportW > (float)sw - 6.0f)
    exportX = x - exportW - 8.0f;
  if (exportY < 6.0f)
    exportY = 6.0f;
  if (exportY + exportH > (float)sh - 6.0f)
    exportY = (float)sh - exportH - 6.0f;

  gui->exportMenuRect = (Rectangle){exportX, exportY, exportW, exportH};
  Vector2 mouse = GetMousePosition();
  Rectangle exportHoverRect = gui->exportMenuRect;
  exportHoverRect.x -= 12.0f;
  exportHoverRect.y -= 6.0f;
  exportHoverRect.width += 24.0f;
  exportHoverRect.height += 12.0f;
  bool overExportMenu = CheckCollisionPointRec(mouse, exportHoverRect);
  gui->showExportMenu = exportHover || overExportMenu;

  if (gui->showExportMenu) {
    DrawRectangleRounded(gui->exportMenuRect, 0.15f, 8, t.surface);
    DrawRectangleRoundedLinesEx(gui->exportMenuRect, 0.15f, 8, 1.0f, t.border);

    float ex = gui->exportMenuRect.x;
    float ey = gui->exportMenuRect.y;
    DrawTextEx(gui->uiFont, "Export", (Vector2){ex + pad + 2, ey + 7}, 12, 1.0f,
               t.text);
    MenuDivider(ex, ey + titleH, exportW, t);

    ey += titleH + pad;
    if (MenuItem(gui, (Rectangle){ex + pad, ey, exportW - pad * 2, itemH},
                 "PNG (view)", NULL, t, false, NULL)) {
      GuiRequestExport(gui, canvas, EXPORT_FORMAT_PNG, EXPORT_SCOPE_VIEW);
      gui->showMenu = false;
      gui->showExportMenu = false;
    }
    ey += itemH;
    if (MenuItem(gui, (Rectangle){ex + pad, ey, exportW - pad * 2, itemH},
                 "PNG (full)", NULL, t, false, NULL)) {
      GuiRequestExport(gui, canvas, EXPORT_FORMAT_PNG, EXPORT_SCOPE_CANVAS);
      gui->showMenu = false;
      gui->showExportMenu = false;
    }
    ey += itemH;
    if (MenuItem(gui, (Rectangle){ex + pad, ey, exportW - pad * 2, itemH},
                 "JPG (view)", NULL, t, false, NULL)) {
      GuiRequestExport(gui, canvas, EXPORT_FORMAT_JPG, EXPORT_SCOPE_VIEW);
      gui->showMenu = false;
      gui->showExportMenu = false;
    }
    ey += itemH;
    if (MenuItem(gui, (Rectangle){ex + pad, ey, exportW - pad * 2, itemH},
                 "JPG (full)", NULL, t, false, NULL)) {
      GuiRequestExport(gui, canvas, EXPORT_FORMAT_JPG, EXPORT_SCOPE_CANVAS);
      gui->showMenu = false;
      gui->showExportMenu = false;
    }
    ey += itemH;
    if (MenuItem(gui, (Rectangle){ex + pad, ey, exportW - pad * 2, itemH},
                 "SVG (view)", NULL, t, false, NULL)) {
      GuiRequestExport(gui, canvas, EXPORT_FORMAT_SVG, EXPORT_SCOPE_VIEW);
      gui->showMenu = false;
      gui->showExportMenu = false;
    }
    ey += itemH;
    if (MenuItem(gui, (Rectangle){ex + pad, ey, exportW - pad * 2, itemH},
                 "SVG (full)", NULL, t, false, NULL)) {
      GuiRequestExport(gui, canvas, EXPORT_FORMAT_SVG, EXPORT_SCOPE_CANVAS);
      gui->showMenu = false;
      gui->showExportMenu = false;
    }
  }

  y += itemH;
  MenuDivider(x, y + 2, w, t);
  y += dividerH;

  if (MenuItem(gui, (Rectangle){x + pad, y, w - pad * 2, itemH}, "Toggle grid",
               "G", t, false, NULL)) {
    canvas->showGrid = !canvas->showGrid;
    gui->showMenu = false;
  }
  y += itemH;
  if (MenuItem(gui, (Rectangle){x + pad, y, w - pad * 2, itemH},
               gui->darkMode ? "Light mode" : "Dark mode", NULL, t, false,
               NULL)) {
    gui->darkMode = !gui->darkMode;
    GuiToastSet(gui, gui->darkMode ? "Dark mode." : "Light mode.");
    gui->showMenu = false;
  }

  y += itemH;
  MenuDivider(x, y + 2, w, t);
  y += dividerH;
  if (MenuItem(gui, (Rectangle){x + pad, y, w - pad * 2, itemH}, "Exit", NULL, t,
               true, NULL)) {
    gui->requestExit = true;
    gui->showMenu = false;
  }

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
      !CheckCollisionPointRec(mouse, gui->menuRect) &&
      !CheckCollisionPointRec(mouse, gui->menuButtonRect) &&
      !(gui->showExportMenu &&
        CheckCollisionPointRec(mouse, gui->exportMenuRect))) {
    gui->showMenu = false;
    gui->showExportMenu = false;
  }
}

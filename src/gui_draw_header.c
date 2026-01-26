#include "gui_internal.h"

void GuiDrawHeader(GuiState *gui, Canvas *canvas, Theme t, Color iconIdle,
                   Color iconHover, int sw) {
  Rectangle header = {0, 0, (float)sw, 40};
  DrawRectangleRec(header, t.surface);
  DrawLineEx((Vector2){0, 40}, (Vector2){(float)sw, 40}, 1, t.border);

  DrawCircle(20, 20, 6, (Color){239, 68, 68, 255});
  DrawCircle(40, 20, 6, (Color){234, 179, 8, 255});
  DrawCircle(60, 20, 6, (Color){34, 197, 94, 255});

  float tabX = 90.0f;
  float tabY = 8.0f;
  float tabW = 150.0f;
  float tabH = 32.0f;
  float tabGap = 8.0f;

  Vector2 mouse = GetMousePosition();
  for (int i = 0; i < gui->documentCount; i++) {
    Rectangle tab = {tabX + i * (tabW + tabGap), tabY, tabW, tabH};
    DrawRectangleRounded(tab, 0.20f, 6, t.tab);
    DrawRectangleRoundedLinesEx(tab, 0.20f, 6, 1.0f, t.border);
    GuiDrawIconTexture(&gui->icons, gui->icons.pen,
                       (Rectangle){tab.x + 8, tab.y + 6, 18, 18}, t.primary);

    Document *doc = &gui->documents[i];
    const char *label =
        doc->hasPath ? GetFileName(doc->path) : "Untitled Sketch";
    DrawTextEx(gui->uiFont, label, (Vector2){tab.x + 30, tab.y + 9}, 12, 1.0f,
               t.text);

    Rectangle tabClose = {tab.x + tab.width - 24, tab.y + 6, 18, 18};
    if (GuiIconButton(&gui->icons, tabClose, gui->icons.windowClose, false, t.hover,
                      t.hover, t.text, iconIdle, iconHover)) {
      bool showGrid = doc->canvas.showGrid;
      GuiCloseDocument(gui, i, GetScreenWidth(), GetScreenHeight(), showGrid);
      GuiToastSet(gui, "Closed.");
      return;
    }

    if (CheckCollisionPointRec(mouse, tab) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      gui->activeDocument = i;
    }
  }

  Rectangle newBtn = {tabX + gui->documentCount * (tabW + tabGap), 9, 26, 26};
  if (GuiIconButton(&gui->icons, newBtn, gui->icons.add, false, t.hover, t.hover,
                    t.text, iconIdle, iconHover)) {
    Document *active = GuiGetActiveDocument(gui);
    bool showGrid = active ? active->canvas.showGrid : true;
    GuiAddDocument(gui, GetScreenWidth(), GetScreenHeight(), showGrid, true);
    GuiToastSet(gui, "New canvas.");
  }

  const char *vText = "cdraw";
  Vector2 vSize = MeasureTextEx(gui->uiFont, vText, 12, 1.0f);
  DrawTextEx(gui->uiFont, vText, (Vector2){(float)sw / 2.0f - vSize.x / 2.0f, 14},
             12, 1.0f, t.textDim);

  float rx = (float)sw - 120;
  Rectangle darkBtn = {rx, 7, 26, 26};
  if (CheckCollisionPointRec(mouse, darkBtn))
    DrawRectangleRounded(darkBtn, 0.25f, 6, t.hover);
  GuiDrawIconTexture(&gui->icons,
                     gui->darkMode ? gui->icons.lightMode : gui->icons.darkMode,
                     darkBtn, iconIdle);
  if (CheckCollisionPointRec(mouse, darkBtn) &&
      IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    gui->darkMode = !gui->darkMode;
    GuiToastSet(gui, gui->darkMode ? "Dark mode." : "Light mode.");
  }

  Rectangle minBtn = {rx + 30, 7, 26, 26};
  if (CheckCollisionPointRec(mouse, minBtn))
    DrawRectangleRounded(minBtn, 0.25f, 6, t.hover);
  GuiDrawIconTexture(&gui->icons, gui->icons.windowMinimize, minBtn, iconIdle);
  if (CheckCollisionPointRec(mouse, minBtn) &&
      IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    MinimizeWindow();

  Rectangle maxBtn = {rx + 60, 7, 26, 26};
  if (CheckCollisionPointRec(mouse, maxBtn))
    DrawRectangleRounded(maxBtn, 0.25f, 6, t.hover);
  GuiDrawIconTexture(&gui->icons, gui->icons.windowToggleSize, maxBtn, iconIdle);
  if (CheckCollisionPointRec(mouse, maxBtn) &&
      IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    if (IsWindowMaximized())
      RestoreWindow();
    else
      MaximizeWindow();
  }

  Rectangle closeBtn = {rx + 90, 7, 26, 26};
  if (GuiIconButton(&gui->icons, closeBtn, gui->icons.windowClose, false, t.hover,
                    t.hover, t.text, iconIdle, iconHover))
    gui->requestExit = true;
}

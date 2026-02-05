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
    bool isActive = (i == gui->activeDocument);
    Color tabFill = isActive ? ColorAlpha(t.surface, 0.98f) : t.tab;
    Color tabBorder = isActive ? t.primary : t.border;
    Color tabText = isActive ? t.text : t.textDim;
    Color tabIcon = isActive ? t.primary : t.textDim;

    DrawRectangleRounded(tab, 0.20f, 6, tabFill);
    DrawRectangleRoundedLinesEx(tab, 0.20f, 6, 1.0f, tabBorder);
    GuiDrawIconTexture(&gui->icons, gui->icons.pen,
                       (Rectangle){tab.x + 8, tab.y + 6, 18, 18}, tabIcon);

    Document *doc = &gui->documents[i];
    const char *label =
        doc->hasPath ? GetFileName(doc->path) : "Untitled Sketch";
    DrawTextEx(gui->uiFont, label, (Vector2){tab.x + 30, tab.y + 9}, 12, 1.0f,
               tabText);

    Rectangle tabClose = {tab.x + tab.width - 24, tab.y + 6, 18, 18};
    if (GuiIconButton(gui, &gui->icons, tabClose, gui->icons.windowClose, false,
                      t.hover, t.hover, t.text, iconIdle, iconHover,
                      "Close tab")) {
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
  if (GuiIconButton(gui, &gui->icons, newBtn, gui->icons.add, false, t.hover,
                    t.hover, t.text, iconIdle, iconHover, "New canvas")) {
    Document *active = GuiGetActiveDocument(gui);
    bool showGrid = active ? active->canvas.showGrid : true;
    GuiAddDocument(gui, GetScreenWidth(), GetScreenHeight(), showGrid, true);
    GuiToastSet(gui, "New canvas.");
  }

  const char *vText = "cdraw";
  Vector2 vSize = MeasureTextEx(gui->uiFont, vText, 12, 1.0f);
  float titleX = (float)sw / 2.0f - vSize.x / 2.0f;
  float titleY = 14.0f;
  float titlePad = 6.0f;

  float tabsRightEdge = tabX + gui->documentCount * (tabW + tabGap) + 26.0f;
  bool titleBlocked = tabsRightEdge + 8.0f >= (titleX - titlePad);
  if (!titleBlocked) {
    DrawTextEx(gui->uiFont, vText, (Vector2){titleX, titleY}, 12, 1.0f, t.textDim);
  }

  float rx = (float)sw - 120;
  const char *themeTip =
      gui->darkMode ? "Switch to light mode" : "Switch to dark mode";
  Rectangle darkBtn = {rx, 7, 26, 26};
  if (GuiIconButton(gui, &gui->icons, darkBtn,
                    gui->darkMode ? gui->icons.lightMode : gui->icons.darkMode,
                    false, t.hover, t.hover, t.text, iconIdle, iconHover,
                    themeTip)) {
    gui->darkMode = !gui->darkMode;
    GuiToastSet(gui, gui->darkMode ? "Dark mode." : "Light mode.");
  }

  Rectangle minBtn = {rx + 30, 7, 26, 26};
  if (GuiIconButton(gui, &gui->icons, minBtn, gui->icons.windowMinimize, false,
                    t.hover, t.hover, t.text, iconIdle, iconHover, "Minimize"))
    MinimizeWindow();

  const char *maxTip =
      IsWindowMaximized() ? "Restore window" : "Maximize window";
  Rectangle maxBtn = {rx + 60, 7, 26, 26};
  if (GuiIconButton(gui, &gui->icons, maxBtn, gui->icons.windowToggleSize, false,
                    t.hover, t.hover, t.text, iconIdle, iconHover, maxTip)) {
    if (IsWindowMaximized())
      RestoreWindow();
    else
      MaximizeWindow();
  }

  Rectangle closeBtn = {rx + 90, 7, 26, 26};
  if (GuiIconButton(gui, &gui->icons, closeBtn, gui->icons.windowClose, false,
                    t.hover, t.hover, t.text, iconIdle, iconHover, "Close app"))
    gui->requestExit = true;
}

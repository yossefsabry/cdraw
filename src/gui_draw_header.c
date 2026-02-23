#include "gui_internal.h"
#include "raymath.h"

void GuiDrawHeader(GuiState *gui, Canvas *canvas, Theme t, Color iconIdle,
                   Color iconHover, int sw) {
  const float headerH = 40.0f;
  Rectangle header = {0, 0, (float)sw, headerH};
  DrawRectangleRec(header, t.surface);
  DrawLineEx((Vector2){0, headerH}, (Vector2){(float)sw, headerH}, 1, t.border);

  float rightPad = 12.0f;
  float rightGap = 8.0f;
  float helpSize = 32.0f;
  float darkSize = 26.0f;
  float rightEdge = (float)sw - rightPad;
  float darkX = rightEdge - darkSize;
  float helpX = darkX - rightGap - helpSize;

  float tabX = 16.0f;
  float tabH = 32.0f;
  float tabY = (headerH - tabH) / 2.0f;
  float tabGap = 8.0f;
  float newBtnW = 26.0f;
  float newBtnH = 26.0f;
  float newBtnY = (headerH - newBtnH) / 2.0f;

  float tabAreaRight = helpX - 10.0f;
  float tabAreaW = tabAreaRight - tabX;
  if (tabAreaW < 0.0f)
    tabAreaW = 0.0f;

  float tabMaxW = 150.0f;
  float tabMinW = sw < 600 ? 56.0f : (sw < 720 ? 68.0f : 90.0f);
  float tabW = tabMaxW;
  if (gui->documentCount > 0) {
    float availableTabs = tabAreaW - newBtnW - tabGap;
    if (availableTabs < 0.0f)
      availableTabs = 0.0f;
    float maxW = (availableTabs - (gui->documentCount - 1) * tabGap) /
                 (float)gui->documentCount;
    tabW = Clamp(maxW, tabMinW, tabMaxW);
  }

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

  float newBtnX = tabX + gui->documentCount * (tabW + tabGap);
  Rectangle newBtn = {newBtnX, newBtnY, newBtnW, newBtnH};
  bool showNewBtn = newBtn.x + newBtn.width <= tabAreaRight;
  if (showNewBtn &&
      GuiIconButton(gui, &gui->icons, newBtn, gui->icons.add, false, t.hover,
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

  float tabsRightEdge = tabX + gui->documentCount * (tabW + tabGap);
  if (showNewBtn)
    tabsRightEdge += newBtnW + tabGap;
  float titleLeft = titleX - titlePad;
  float titleRight = titleX + vSize.x + titlePad;
  bool titleBlocked = titleLeft <= tabsRightEdge + 8.0f ||
                      titleRight >= helpX - 8.0f;
  if (!titleBlocked)
    DrawTextEx(gui->uiFont, vText, (Vector2){titleX, titleY}, 12, 1.0f, t.textDim);

  Rectangle helpBtn = {helpX, (headerH - helpSize) / 2.0f, helpSize, helpSize};
  gui->helpButtonRect = helpBtn;
  bool helpHover = CheckCollisionPointRec(mouse, helpBtn);
  if (helpHover)
    GuiTooltipSet(gui, "Help", helpBtn);

  Color helpBg = gui->showHelp
                     ? t.primary
                     : (helpHover ? ColorAlpha(t.primary, 0.18f)
                                  : ColorAlpha(t.surface, 0.85f));
  DrawRectangleRounded(helpBtn, 0.25f, 6, helpBg);
  Color helpBorder = gui->showHelp ? t.primary : t.border;
  DrawRectangleRoundedLinesEx(helpBtn, 0.25f, 6, 1.25f, helpBorder);

  const char *helpText = "?";
  float helpFontSize = 20.0f;
  Vector2 helpTextSize =
      MeasureTextEx(gui->uiFont, helpText, helpFontSize, 1.0f);
  DrawTextEx(gui->uiFont, helpText,
             (Vector2){helpBtn.x + (helpBtn.width - helpTextSize.x) / 2.0f,
                       helpBtn.y + (helpBtn.height - helpTextSize.y) / 2.0f - 1.0f},
             helpFontSize, 1.0f,
             gui->showHelp ? BLACK : (helpHover ? t.text : t.text));
  if (helpHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    gui->showHelp = !gui->showHelp;
    if (gui->showHelp) {
      gui->showMenu = false;
      gui->showExportMenu = false;
      gui->showColorPicker = false;
    }
  }

  const char *themeTip =
      gui->darkMode ? "Switch to light mode" : "Switch to dark mode";
  Rectangle darkBtn = {darkX, (headerH - darkSize) / 2.0f, darkSize, darkSize};
  if (GuiIconButton(gui, &gui->icons, darkBtn,
                    gui->darkMode ? gui->icons.lightMode : gui->icons.darkMode,
                    false, t.hover, t.hover, t.text, iconIdle, iconHover,
                    themeTip)) {
    gui->darkMode = !gui->darkMode;
    GuiToastSet(gui, gui->darkMode ? "Dark mode." : "Light mode.");
  }
}

#include "gui_internal.h"
#include "ai/ai_panel.h"
#include "ai/ai_settings.h"
#include "ai/ai_settings_ui.h"
#include <stddef.h>
#include <stdio.h>

static bool MenuItem(GuiState *gui, Rectangle r,
                     const char *label,
                     const char *shortcut, Theme t,
                     bool destructive, bool *outHover) {
  Vector2 mouse = GetMousePosition();
  bool hover = CheckCollisionPointRec(mouse, r);
  if (outHover)
    *outHover = hover;
  Color bg = hover ? t.hover : (Color){0, 0, 0, 0};
  if (bg.a > 0)
    DrawRectangleRec(r, bg);

  Color labelColor = destructive ?
                     (Color){239, 68, 68, 255} :
                     t.text;
  DrawTextEx(gui->uiFont, label,
             (Vector2){r.x + 12, r.y + 8},
             13, 1.0f, labelColor);

  if (shortcut && shortcut[0] != '\0') {
    Vector2 size = MeasureTextEx(gui->uiFont,
                                 shortcut, 12, 1.0f);
    DrawTextEx(gui->uiFont, shortcut,
               (Vector2){r.x + r.width - size.x - 12,
                         r.y + 8},
               12, 1.0f, t.textDim);
  }
  return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static void MenuDivider(float x, float y,
                        float w, Theme t) {
  DrawLineEx((Vector2){x + 10, y},
             (Vector2){x + w - 10, y},
             1.0f, t.border);
}

void GuiDrawMenu(GuiState *gui, Canvas *canvas, Theme t) {
  if (!gui->showMenu) {
    gui->showExportMenu = false;
    gui->showAiMenu = false;
    return;
  }

  const float pad = 10.0f;
  const float titleH = 30.0f;
  const float itemH = 30.0f;
  const float dividerH = 12.0f;

  const int itemCount = 9;
  const int dividerCount = 3;
  float w = 250.0f;
  float h = titleH + pad + itemCount * itemH +
            dividerCount * dividerH + pad;

  gui->menuRect.width = w;
  gui->menuRect.height = h;

  DrawRectangleRounded(gui->menuRect, 0.15f, 8, t.surface);
  DrawRectangleRoundedLinesEx(gui->menuRect,
                              0.15f, 8,
                              1.0f, t.border);

  float x = gui->menuRect.x;
  float y = gui->menuRect.y;

  DrawTextEx(gui->uiFont, "Menu",
             (Vector2){x + pad + 2, y + 8},
             13, 1.0f, t.text);
  MenuDivider(x, y + titleH, w, t);

  y += titleH + pad;
  if (MenuItem(gui,
               (Rectangle){x + pad, y,
                            w - pad * 2,
                            itemH},
               "New tab", "Ctrl+N",
               t, false, NULL)) {
    Document *active = GuiGetActiveDocument(gui);
    bool showGrid = active ? active->canvas.showGrid : true;
    GuiAddDocument(gui, GetScreenWidth(),
                   GetScreenHeight(),
                   showGrid, true);
    GuiToastSet(gui, "New canvas.");
    gui->showMenu = false;
  }
  y += itemH;
  if (MenuItem(gui,
               (Rectangle){x + pad, y,
                            w - pad * 2,
                            itemH},
               "Clear canvas", NULL,
               t, false, NULL)) {
    ClearCanvas(canvas);
    GuiMarkNewDocument(gui);
    GuiToastSet(gui, "Canvas cleared.");
    gui->showMenu = false;
  }
  y += itemH;
  if (MenuItem(gui,
               (Rectangle){x + pad, y,
                            w - pad * 2,
                            itemH},
               "Open", "Ctrl+O",
               t, false, NULL)) {
    GuiRequestOpen(gui);
    gui->showMenu = false;
  }
  y += itemH;
  if (MenuItem(gui,
               (Rectangle){x + pad, y,
                            w - pad * 2,
                            itemH},
               "Save", "Ctrl+S",
               t, false, NULL)) {
    GuiRequestSave(gui, GuiGetActiveDocument(gui));
    gui->showMenu = false;
  }

  y += itemH;
  bool aiHover = false;
  Rectangle aiRow = {x + pad, y,
                     w - pad * 2,
                     itemH};
  bool aiClicked = MenuItem(gui, aiRow, "AI", ">",
                            t, false, &aiHover);
  if (aiClicked) {
    gui->showAiMenu = true;
    gui->showExportMenu = false;
  }

  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  const int aiItemCount = 4;
  float aiW = 240.0f;
  float aiH = titleH + pad +
              aiItemCount * itemH + pad;
  float aiX = x + w + 8.0f;
  float aiY = aiRow.y - pad - titleH;
  if (aiX + aiW > (float)sw - 6.0f)
    aiX = x - aiW - 8.0f;
  if (aiY < 6.0f)
    aiY = 6.0f;
  if (aiY + aiH > (float)sh - 6.0f)
    aiY = (float)sh - aiH - 6.0f;

  gui->aiMenuRect =
      (Rectangle){aiX, aiY, aiW, aiH};
  Vector2 mouse = GetMousePosition();
  Rectangle aiHoverRect = gui->aiMenuRect;
  aiHoverRect.x -= 12.0f;
  aiHoverRect.y -= 6.0f;
  aiHoverRect.width += 24.0f;
  aiHoverRect.height += 12.0f;
  bool overAiMenu =
      CheckCollisionPointRec(mouse, aiHoverRect);
  bool wantAiMenu = aiHover ||
                    (gui->showAiMenu &&
                     overAiMenu);

  y += itemH;
  MenuDivider(x, y + 2, w, t);
  y += dividerH;

  bool exportHover = false;
  Rectangle exportRow = {x + pad, y,
                         w - pad * 2,
                         itemH};
  bool exportClicked =
      MenuItem(gui, exportRow, "Export", ">",
               t, false, &exportHover);
  if (exportClicked) {
    gui->showExportMenu = true;
    gui->showAiMenu = false;
  }

  sw = GetScreenWidth();
  sh = GetScreenHeight();
  const int exportItemCount = 3;
  float exportW = 240.0f;
  float exportH = titleH + pad +
                  exportItemCount * itemH + pad;
  float exportX = x + w +
                  8.0f;
  float exportY = exportRow.y - pad - titleH;
  if (exportX + exportW > (float)sw - 6.0f)
    exportX = x - exportW - 8.0f;
  if (exportY < 6.0f)
    exportY = 6.0f;
  if (exportY + exportH > (float)sh - 6.0f)
    exportY = (float)sh - exportH - 6.0f;

  gui->exportMenuRect =
      (Rectangle){exportX, exportY, exportW, exportH};
  mouse = GetMousePosition();
  Rectangle exportHoverRect = gui->exportMenuRect;
  exportHoverRect.x -= 12.0f;
  exportHoverRect.y -= 6.0f;
  exportHoverRect.width += 24.0f;
  exportHoverRect.height += 12.0f;
  bool overExportMenu =
      CheckCollisionPointRec(mouse, exportHoverRect);
  bool wantExportMenu = exportHover ||
                        (gui->showExportMenu &&
                         overExportMenu);
  if (wantAiMenu) {
    gui->showAiMenu = true;
    gui->showExportMenu = false;
  } else if (wantExportMenu) {
    gui->showExportMenu = true;
    gui->showAiMenu = false;
  } else {
    gui->showAiMenu = false;
    gui->showExportMenu = false;
  }

  if (gui->showAiMenu) {
    DrawRectangleRounded(gui->aiMenuRect,
                         0.15f, 8,
                         t.surface);
    DrawRectangleRoundedLinesEx(gui->aiMenuRect,
                                0.15f, 8,
                                1.0f,
                                t.border);
    float ax = gui->aiMenuRect.x;
    float ay = gui->aiMenuRect.y;
    DrawTextEx(gui->uiFont, "AI",
               (Vector2){ax + pad + 2,
                         ay + 8},
               13, 1.0f, t.text);
    MenuDivider(ax, ay + titleH, aiW, t);
    ay += titleH + pad;
    if (MenuItem(gui,
                 (Rectangle){ax + pad,
                              ay,
                              aiW - pad * 2,
                              itemH},
                 "Analyze", NULL,
                 t, false, NULL)) {
      AiSettings s;
      char err[64] = {0};
      (void)AiSettingsLoad(&s, err, sizeof(err));
      if (!AiSettingsReady(&s, err,
                           sizeof(err))) {
        AiSettingsUiOpen(gui);
        snprintf(gui->aiStatus,
                 sizeof(gui->aiStatus),
                 "AI settings needed");
        GuiToastSet(gui, "AI settings needed");
      } else {
        AiPanelRequest(gui, canvas);
      }
      gui->showMenu = false;
      gui->showAiMenu = false;
    }
    ay += itemH;
    if (MenuItem(gui,
                 (Rectangle){ax + pad,
                              ay,
                              aiW - pad * 2,
                              itemH},
                 "Settings", NULL,
                 t, false, NULL)) {
      AiSettingsUiOpen(gui);
      gui->showMenu = false;
      gui->showAiMenu = false;
    }
    ay += itemH;
    if (MenuItem(gui,
                 (Rectangle){ax + pad,
                              ay,
                              aiW - pad * 2,
                              itemH},
                 "Show analysis",
                 NULL, t, false,
                 NULL)) {
      gui->showAiPanel = true;
      gui->showMenu = false;
      gui->showAiMenu = false;
    }
    ay += itemH;
    if (MenuItem(gui,
                 (Rectangle){ax + pad,
                              ay,
                              aiW - pad * 2,
                              itemH},
                 "Clear analysis",
                 NULL, t, false,
                 NULL)) {
      gui->aiText[0] = '\0';
      gui->showAiPanel = false;
      GuiToastSet(gui, "AI analysis cleared");
      gui->showMenu = false;
      gui->showAiMenu = false;
    }
  }

  if (gui->showExportMenu) {
    DrawRectangleRounded(gui->exportMenuRect,
                         0.15f, 8,
                         t.surface);
    DrawRectangleRoundedLinesEx(gui->exportMenuRect,
                                0.15f, 8,
                                1.0f,
                                t.border);

    float ex = gui->exportMenuRect.x;
    float ey = gui->exportMenuRect.y;
    DrawTextEx(gui->uiFont, "Export",
               (Vector2){ex + pad + 2,
                         ey + 8},
               13, 1.0f, t.text);
    MenuDivider(ex, ey + titleH, exportW, t);

    ey += titleH + pad;
    if (MenuItem(gui,
                 (Rectangle){ex + pad,
                              ey,
                              exportW - pad * 2,
                              itemH},
                 "PNG (FHD)", NULL,
                 t, false, NULL)) {
      GuiRequestExport(gui, canvas,
                       EXPORT_FORMAT_PNG,
                       EXPORT_SCOPE_VIEW_FHD);
      gui->showMenu = false;
      gui->showExportMenu = false;
    }
    ey += itemH;
    if (MenuItem(gui,
                 (Rectangle){ex + pad,
                              ey,
                              exportW - pad * 2,
                              itemH},
                 "JPG (FHD)", NULL,
                 t, false, NULL)) {
      GuiRequestExport(gui, canvas,
                       EXPORT_FORMAT_JPG,
                       EXPORT_SCOPE_VIEW_FHD);
      gui->showMenu = false;
      gui->showExportMenu = false;
    }
    ey += itemH;
    if (MenuItem(gui,
                 (Rectangle){ex + pad,
                              ey,
                              exportW - pad * 2,
                              itemH},
                 "SVG (FHD)", NULL,
                 t, false, NULL)) {
      GuiRequestExport(gui, canvas,
                       EXPORT_FORMAT_SVG,
                       EXPORT_SCOPE_VIEW_FHD);
      gui->showMenu = false;
      gui->showExportMenu = false;
    }
  }

  y += itemH;
  MenuDivider(x, y + 2, w, t);
  y += dividerH;

  if (MenuItem(gui,
               (Rectangle){x + pad, y,
                            w - pad * 2,
                            itemH},
               "Toggle grid", "G",
               t, false, NULL)) {
    canvas->showGrid = !canvas->showGrid;
    gui->showMenu = false;
  }
  y += itemH;
  if (MenuItem(gui,
               (Rectangle){x + pad, y,
                            w - pad * 2,
                            itemH},
               gui->darkMode ? "Light mode"
                             : "Dark mode",
               NULL, t, false, NULL)) {
    gui->darkMode = !gui->darkMode;
    const char *mode =
        gui->darkMode ? "Dark mode." : "Light mode.";
    GuiToastSet(gui, mode);
    gui->showMenu = false;
  }

  y += itemH;
  MenuDivider(x, y + 2, w, t);
  y += dividerH;
  if (MenuItem(gui,
               (Rectangle){x + pad, y,
                            w - pad * 2,
                            itemH},
               "Exit", NULL, t, true, NULL)) {
    gui->requestExit = true;
    gui->showMenu = false;
  }

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
      !CheckCollisionPointRec(mouse, gui->menuRect) &&
      !CheckCollisionPointRec(mouse, gui->menuButtonRect) &&
      !(gui->showExportMenu &&
        CheckCollisionPointRec(mouse, gui->exportMenuRect)) &&
      !(gui->showAiMenu &&
        CheckCollisionPointRec(mouse, gui->aiMenuRect))) {
    gui->showMenu = false;
    gui->showExportMenu = false;
    gui->showAiMenu = false;
  }
}

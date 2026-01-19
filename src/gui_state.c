#include "gui_internal.h"
#include <stdio.h>
#include <string.h>

void GuiToastSet(GuiState *gui, const char *msg) {
  snprintf(gui->toast, sizeof(gui->toast), "%s", msg);
  gui->toastUntil = GetTime() + 2.5;
}

void UnloadGui(GuiState *gui) { GuiIconsUnload(&gui->icons); }

void InitGui(GuiState *gui) {
  memset(gui, 0, sizeof(*gui));

  gui->activeTool = TOOL_PEN;
  gui->currentColor = WHITE;
  gui->currentThickness = 3.0f;
  gui->toolbarRect = (Rectangle){0, 0, (float)GetScreenWidth(), 88};

  gui->showRulers = true;

  gui->paletteRect = (Rectangle){0, 0, 0, 0};
  gui->paletteButtonRect = (Rectangle){0, 0, 0, 0};
  gui->colorButtonRect = (Rectangle){0, 0, 0, 0};
  gui->menuButtonRect = (Rectangle){0, 0, 0, 0};

  gui->showColorPicker = false;
  gui->colorPickerRect = (Rectangle){0, 0, 240, 220};
  Vector3 hsv = ColorToHSV(gui->currentColor);
  gui->hueValue = hsv.x;
  gui->satValue = hsv.y;
  gui->valValue = hsv.z;

  gui->darkMode = true;
  gui->showMenu = false;
  gui->menuRect = (Rectangle){0, 0, 200, 220};
  gui->requestExit = false;

  snprintf(gui->currentFile, sizeof(gui->currentFile), "%s", "drawing.cdraw");
  gui->toast[0] = '\0';
  gui->toastUntil = 0.0;

  GuiIconsLoad(&gui->icons);
}

bool IsMouseOverGui(GuiState *gui) {
  const float topH = 88.0f;
  const float footerH = 24.0f;
  const float rulerSize = 24.0f;

  Vector2 mouse = GetMousePosition();
  bool overTop = CheckCollisionPointRec(
      mouse, (Rectangle){0, 0, (float)GetScreenWidth(), topH});

  if (gui->showRulers) {
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();

    float topW = sw - rulerSize;
    if (topW < 0.0f)
      topW = 0.0f;
    gui->rulerTopRect =
        (Rectangle){rulerSize, topH, topW, rulerSize};

    float leftH = sh - footerH - (topH + rulerSize);
    if (leftH < 0.0f)
      leftH = 0.0f;
    gui->rulerLeftRect =
        (Rectangle){0, topH + rulerSize, rulerSize, leftH};

    Rectangle corner = (Rectangle){0, topH, rulerSize, rulerSize};
    bool overRulers = CheckCollisionPointRec(mouse, gui->rulerTopRect) ||
                      CheckCollisionPointRec(mouse, gui->rulerLeftRect) ||
                      CheckCollisionPointRec(mouse, corner);
    if (overRulers)
      return true;
  }

  bool overPalette = CheckCollisionPointRec(mouse, gui->paletteRect);
  bool overPicker =
      gui->showColorPicker && CheckCollisionPointRec(mouse, gui->colorPickerRect);
  bool overMenu = gui->showMenu && CheckCollisionPointRec(mouse, gui->menuRect);
  return overTop || overPalette || overPicker || overMenu;
}

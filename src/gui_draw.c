#include "gui_internal.h"
#include <math.h>

void DrawGui(GuiState *gui, Canvas *canvas) {
  Theme t = GuiThemeGet(gui->darkMode);
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();

  if (gui->showWelcome) {
    GuiDrawWelcome(gui, canvas, t, sw, sh);
    return;
  }

  if (gui->showFileDialog) {
    GuiDrawFileDialog(gui, canvas, t, sw, sh);
    return;
  }

  const float footerH = 24.0f;
  const float paletteW = 280.0f;
  const float paletteH = 48.0f;
  const float paletteGap = 16.0f;

  float paletteX = (float)sw / 2.0f - paletteW / 2.0f;
  float paletteY = (float)sh - footerH - paletteGap - paletteH;
  paletteY = fmaxf(paletteY, 120.0f);

  Color iconIdle = gui->darkMode ? (Color){227, 227, 227, 255} : t.textDim;
  Color iconHover = gui->darkMode ? (Color){255, 255, 255, 255} : t.text;

  GuiDrawHeader(gui, canvas, t, iconIdle, iconHover, sw);
  GuiDrawToolbar(gui, canvas, t, iconIdle, iconHover, sw, sh, paletteY);
  GuiDrawRulerTop(gui, canvas, t, sw, sh);
  GuiDrawRulerLeft(gui, canvas, t, sw, sh);
  GuiDrawMenu(gui, canvas, t);
  GuiDrawPalette(gui, canvas, t, iconIdle, iconHover, sw, sh, paletteX, paletteY,
                 paletteW, paletteH);
  GuiDrawColorPicker(gui, t);
  GuiDrawFooter(gui, canvas, t, sw, sh);
}

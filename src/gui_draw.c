#include "gui_internal.h"
#include "ai/ai_panel.h"
#include "ai/ai_settings_ui.h"
#include "raymath.h"
#include <math.h>

void DrawGui(GuiState *gui, Canvas *canvas) {
  Theme t = GuiThemeGet(gui->darkMode);
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();

  GuiTooltipReset(gui);
  if (gui->showWelcome) {
    GuiDrawWelcome(gui, canvas, t, sw, sh);
    return;
  }


  const float footerH = 24.0f;
  const float paletteH = 48.0f;
  const float paletteGap = 16.0f;
  float paletteW = 280.0f;

  float maxPaletteW = (float)sw - 32.0f;
  if (maxPaletteW < 160.0f)
    maxPaletteW = (float)sw - 16.0f;
  if (maxPaletteW < 0.0f)
    maxPaletteW = 0.0f;
  if (paletteW > maxPaletteW)
    paletteW = maxPaletteW;

  float paletteX = (float)sw / 2.0f - paletteW / 2.0f;
  float minPaletteX = 12.0f;
  float maxPaletteX = (float)sw - paletteW - 12.0f;
  if (maxPaletteX < minPaletteX)
    maxPaletteX = minPaletteX;
  paletteX = Clamp(paletteX, minPaletteX, maxPaletteX);

  float paletteY = (float)sh - footerH - paletteGap - paletteH;
  float minPaletteY = 120.0f;
  float maxPaletteY = (float)sh - footerH - paletteH - 8.0f;
  if (maxPaletteY < minPaletteY)
    maxPaletteY = minPaletteY;
  paletteY = Clamp(paletteY, minPaletteY, maxPaletteY);

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
  GuiDrawHelpPanel(gui, canvas, t, sw, sh);
  AiPanelDraw(gui, canvas, t, sw, sh);
  AiSettingsUiDraw(gui, t, sw, sh);
  GuiDrawFooter(gui, canvas, t, sw, sh);
  GuiDrawTooltip(gui, t);
}

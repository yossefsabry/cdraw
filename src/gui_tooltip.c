#include "gui_internal.h"
#include <stdio.h>

void GuiTooltipReset(GuiState *gui) {
  if (!gui)
    return;
  gui->showTooltip = false;
  gui->tooltip[0] = '\0';
  gui->tooltipAnchor = (Rectangle){0, 0, 0, 0};
}

void GuiTooltipSet(GuiState *gui, const char *text, Rectangle anchor) {
  if (!gui || !text || text[0] == '\0')
    return;
  gui->showTooltip = true;
  gui->tooltipAnchor = anchor;
  snprintf(gui->tooltip, sizeof(gui->tooltip), "%s", text);
}

void GuiDrawTooltip(GuiState *gui, Theme t) {
  if (!gui || !gui->showTooltip || gui->tooltip[0] == '\0')
    return;

  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  const float fontSize = 12.0f;
  const float padX = 8.0f;
  const float padY = 6.0f;

  Vector2 textSize = MeasureTextEx(gui->uiFont, gui->tooltip, fontSize, 1.0f);
  float w = textSize.x + padX * 2.0f;
  float h = textSize.y + padY * 2.0f;

  float x = gui->tooltipAnchor.x + gui->tooltipAnchor.width * 0.5f - w * 0.5f;
  float y = gui->tooltipAnchor.y - h - 8.0f;
  if (y < 8.0f)
    y = gui->tooltipAnchor.y + gui->tooltipAnchor.height + 8.0f;
  if (x < 8.0f)
    x = 8.0f;
  if (x + w > (float)sw - 8.0f)
    x = (float)sw - w - 8.0f;
  if (y + h > (float)sh - 8.0f)
    y = (float)sh - h - 8.0f;

  Rectangle r = {x, y, w, h};
  Color bg = ColorAlpha(t.surface, 0.95f);
  DrawRectangleRounded(r, 0.25f, 6, bg);
  DrawRectangleRoundedLinesEx(r, 0.25f, 6, 1.0f, t.border);
  DrawTextEx(gui->uiFont, gui->tooltip, (Vector2){x + padX, y + padY}, fontSize,
             1.0f, t.text);
}

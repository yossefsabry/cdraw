#include "gui_internal.h"
#include <math.h>

static float DrawSectionTitle(Font font, float x, float y, const char *label,
                              Theme t) {
  DrawTextEx(font, label, (Vector2){x, y}, 16.0f, 1.0f, t.text);
  return y + 22.0f;
}

static float DrawKeyCap(Font font, float x, float y, const char *text, Theme t) {
  const float fontSize = 14.0f;
  const float padX = 9.0f;
  const float padY = 6.0f;
  Vector2 size = MeasureTextEx(font, text, fontSize, 1.0f);
  Rectangle r = {x, y, size.x + padX * 2.0f, size.y + padY * 2.0f};

  Color bg = ColorAlpha(t.border, 0.45f);
  DrawRectangleRounded(r, 0.35f, 6, bg);
  DrawRectangleRoundedLinesEx(r, 0.35f, 6, 1.0f, t.border);
  DrawTextEx(font, text, (Vector2){x + padX, y + padY}, fontSize, 1.0f, t.text);
  return r.width;
}

static float DrawShortcutRow(Font font, float x, float y, const char *key,
                             const char *label, Theme t) {
  float keyW = DrawKeyCap(font, x, y, key, t);
  float labelX = x + keyW + 10.0f;
  DrawTextEx(font, label, (Vector2){labelX, y + 4.0f}, 15.0f, 1.0f, t.textDim);
  return y + 28.0f;
}

void GuiDrawHelpPanel(GuiState *gui, const Canvas *canvas, Theme t, int sw,
                      int sh) {
  (void)canvas;
  if (!gui->showHelp)
    return;

  const float topH = 88.0f;
  const float footerH = 24.0f;
  const float margin = 32.0f;

  float maxW = (float)sw - margin * 2.0f;
  if (maxW < 120.0f)
    maxW = (float)sw - 8.0f;
  if (maxW < 80.0f)
    maxW = (float)sw;

  float panelW = fminf(460.0f, fmaxf(280.0f, (float)sw * 0.42f));
  if (panelW > maxW)
    panelW = maxW;
  if (panelW < 140.0f)
    panelW = maxW;

  float panelX = (float)sw - panelW - margin;
  float panelY = topH + margin;
  float availableH = (float)sh - footerH - margin - panelY;
  if (availableH < 0.0f)
    availableH = 0.0f;
  float panelH = fmaxf(200.0f, availableH);
  if (panelH > availableH)
    panelH = availableH;

  Rectangle panel = {panelX, panelY, panelW, panelH};
  gui->helpRect = panel;

  if (panel.width <= 0.0f || panel.height <= 0.0f)
    return;

  DrawRectangleRounded(panel, 0.06f, 10, ColorAlpha(t.surface, 0.98f));
  DrawRectangleRoundedLinesEx(panel, 0.06f, 10, 1.0f, t.border);

  float x = panel.x + 20.0f;
  float y = panel.y + 18.0f;

  DrawTextEx(gui->uiFont, "Help & Shortcuts", (Vector2){x, y}, 20.0f, 1.0f,
             t.text);
  y += 28.0f;
  DrawTextEx(gui->uiFont, "Press Esc or click ? to close.", (Vector2){x, y},
             15.0f, 1.0f, t.textDim);
  y += 20.0f;
  DrawLineEx((Vector2){panel.x + 12.0f, y + 6.0f},
             (Vector2){panel.x + panel.width - 12.0f, y + 6.0f}, 1.0f,
             t.border);
  y += 20.0f;

  y = DrawSectionTitle(gui->uiFont, x, y, "Tools", t);
  y = DrawShortcutRow(gui->uiFont, x, y, "P", "Pen", t);
  y = DrawShortcutRow(gui->uiFont, x, y, "B", "Rectangle", t);
  y = DrawShortcutRow(gui->uiFont, x, y, "C", "Circle", t);
  y = DrawShortcutRow(gui->uiFont, x, y, "A", "Arrow / Line", t);
  y = DrawShortcutRow(gui->uiFont, x, y, "E", "Eraser", t);
  y = DrawShortcutRow(gui->uiFont, x, y, "M", "Move / Select", t);
  y += 6.0f;

  y = DrawSectionTitle(gui->uiFont, x, y, "Navigation", t);
  y = DrawShortcutRow(gui->uiFont, x, y, "Wheel", "Zoom", t);
  y = DrawShortcutRow(gui->uiFont, x, y, "Right drag", "Pan", t);
  y = DrawShortcutRow(gui->uiFont, x, y, "Space + drag", "Pan", t);
  y = DrawShortcutRow(gui->uiFont, x, y, "Ctrl + Shift + Wheel",
                      "Thickness", t);
  y = DrawShortcutRow(gui->uiFont, x, y, "G", "Toggle grid", t);
  y = DrawShortcutRow(gui->uiFont, x, y, "F", "Fullscreen", t);
  y += 6.0f;

  y = DrawSectionTitle(gui->uiFont, x, y, "Files", t);
  y = DrawShortcutRow(gui->uiFont, x, y, "Ctrl + S", "Save", t);
  y = DrawShortcutRow(gui->uiFont, x, y, "Ctrl + O", "Open", t);
  y = DrawShortcutRow(gui->uiFont, x, y, "Ctrl + N", "New canvas", t);
  y = DrawShortcutRow(gui->uiFont, x, y, "Ctrl + W", "Close tab", t);
  y += 6.0f;

  y = DrawSectionTitle(gui->uiFont, x, y, "Edit & Selection", t);
  y = DrawShortcutRow(gui->uiFont, x, y, "Ctrl + Z", "Undo", t);
  y = DrawShortcutRow(gui->uiFont, x, y, "Ctrl + Y", "Redo", t);
  y = DrawShortcutRow(gui->uiFont, x, y, "Delete", "Remove selection", t);
  y = DrawShortcutRow(gui->uiFont, x, y, "Click", "Select stroke", t);
  DrawShortcutRow(gui->uiFont, x, y, "Drag", "Move stroke", t);
}

#include "gui_internal.h"
#include <math.h>

void GuiDrawPalette(GuiState *gui, Canvas *canvas, Theme t, Color iconIdle,
                    Color iconHover, int sw, int sh, float paletteX,
                    float paletteY, float paletteW, float paletteH) {
  (void)canvas;
  gui->paletteRect = (Rectangle){paletteX, paletteY, paletteW, paletteH};

  DrawRectangleRounded(gui->paletteRect, 1.0f, 10, t.surface);
  DrawRectangleRoundedLinesEx(gui->paletteRect, 1.0f, 10, 1.0f, t.border);

  Color pal[] = {BLACK, WHITE, (Color){239, 68, 68, 255},
                 (Color){59, 130, 246, 255}, (Color){234, 179, 8, 255}};
  float buttonSize = fminf(32.0f, paletteH - 16.0f);
  float buttonPad = 10.0f;
  float btnX = paletteX + paletteW - buttonSize - buttonPad;
  if (btnX < paletteX + 4.0f)
    btnX = paletteX + 4.0f;

  float leftPad = paletteW < 240.0f ? 12.0f : 18.0f;
  float colorGap = paletteW < 220.0f ? 10.0f : 16.0f;
  float colorLeft = paletteX + leftPad;
  float colorRight = btnX - colorGap;
  if (colorRight < colorLeft)
    colorRight = colorLeft;

  float innerW = colorRight - colorLeft;
  if (innerW < 0.0f)
    innerW = 0.0f;

  float step = innerW / 4.0f;
  float maxStep = 32.0f;
  if (step > maxStep)
    step = maxStep;
  if (step < 1.0f)
    step = 1.0f;
  float groupW = step * 4.0f;
  float startX = colorLeft + (innerW - groupW) * 0.5f;
  if (startX < colorLeft)
    startX = colorLeft;
  float r = fminf(12.0f, fmaxf(3.0f, step * 0.42f));
  float centerY = paletteY + paletteH / 2.0f;
  Vector2 mouse = GetMousePosition();

  for (int i = 0; i < 5; i++) {
    Vector2 pos = {startX + (float)i * step, centerY};
    DrawCircleV(pos, r, pal[i]);
    if (ColorToInt(pal[i]) == ColorToInt(WHITE))
      DrawCircleLines((int)pos.x, (int)pos.y, r, t.border);

    bool selected = ColorToInt(gui->currentColor) == ColorToInt(pal[i]);
    if (selected)
      DrawCircleLines((int)pos.x, (int)pos.y, r + 3, t.primary);

    if (CheckCollisionPointCircle(mouse, pos, r) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      gui->currentColor = pal[i];
      Vector3 hsv = ColorToHSV(gui->currentColor);
      gui->hueValue = hsv.x;
      gui->satValue = hsv.y;
      gui->valValue = hsv.z;
    }
  }

  float dividerX = btnX - buttonPad;
  if (dividerX > paletteX + 6.0f)
    DrawLine((int)dividerX, (int)(paletteY + 10), (int)dividerX,
             (int)(paletteY + paletteH - 10), t.border);

  gui->paletteButtonRect = (Rectangle){btnX,
                                       paletteY + (paletteH - buttonSize) / 2.0f,
                                       buttonSize, buttonSize};
  if (GuiIconButton(gui, &gui->icons, gui->paletteButtonRect,
                    gui->icons.colorPicker, false, t.hover, t.hover, t.text,
                    iconIdle, iconHover, "Color picker")) {
    gui->showColorPicker = !gui->showColorPicker;
    gui->colorPickerRect.x = (float)sw / 2 - gui->colorPickerRect.width / 2;
    gui->colorPickerRect.y = paletteY - gui->colorPickerRect.height - 10;
    float minX = 8.0f;
    float maxX = (float)sw - gui->colorPickerRect.width - 8.0f;
    if (maxX < minX)
      maxX = minX;
    gui->colorPickerRect.x = fminf(fmaxf(gui->colorPickerRect.x, minX), maxX);
    float minY = 88.0f;
    float maxY = (float)sh - gui->colorPickerRect.height - 8.0f;
    if (maxY < minY)
      maxY = minY;
    gui->colorPickerRect.y = fminf(fmaxf(gui->colorPickerRect.y, minY), maxY);
  }
}

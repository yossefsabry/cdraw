#include "gui_internal.h"

void GuiDrawPalette(GuiState *gui, Canvas *canvas, Theme t, Color iconIdle,
                    Color iconHover, int sw, int sh, float paletteX,
                    float paletteY, float paletteW, float paletteH) {
  (void)canvas;
  gui->paletteRect = (Rectangle){paletteX, paletteY, paletteW, paletteH};

  DrawRectangleRounded(gui->paletteRect, 1.0f, 10, t.surface);
  DrawRectangleRoundedLinesEx(gui->paletteRect, 1.0f, 10, 1.0f, t.border);

  Color pal[] = {BLACK, WHITE, (Color){239, 68, 68, 255},
                 (Color){59, 130, 246, 255}, (Color){234, 179, 8, 255}};
  float r = 12;
  float startX = paletteX + 24;
  Vector2 mouse = GetMousePosition();

  for (int i = 0; i < 5; i++) {
    Vector2 pos = {startX + (float)i * 40, paletteY + paletteH / 2};
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

  DrawLine((int)(startX + 4 * 40 + 20), (int)(paletteY + 10),
           (int)(startX + 4 * 40 + 20), (int)(paletteY + paletteH - 10),
           t.border);

  gui->paletteButtonRect = (Rectangle){paletteX + paletteW - 42, paletteY + 8,
                                       32, 32};
  if (GuiIconButton(gui, &gui->icons, gui->paletteButtonRect,
                    gui->icons.colorPicker, false, t.hover, t.hover, t.text,
                    iconIdle, iconHover, "Color picker")) {
    gui->showColorPicker = !gui->showColorPicker;
    gui->colorPickerRect.x = (float)sw / 2 - gui->colorPickerRect.width / 2;
    gui->colorPickerRect.y = paletteY - gui->colorPickerRect.height - 10;
  }
}

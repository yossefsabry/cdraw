#include "gui_internal.h"
#include "raymath.h"

void GuiDrawColorPicker(GuiState *gui, Theme t) {
  if (!gui->showColorPicker)
    return;

  DrawRectangleRounded(gui->colorPickerRect, 0.06f, 6, t.surface);
  DrawRectangleRoundedLinesEx(gui->colorPickerRect, 0.06f, 6, 1.0f, t.border);

  Rectangle sv = {gui->colorPickerRect.x + 12, gui->colorPickerRect.y + 12, 160,
                  160};
  Rectangle hue = {sv.x + sv.width + 12, sv.y, 20, sv.height};

  Color hueColor = ColorFromHSV(gui->hueValue, 1.0f, 1.0f);
  DrawRectangleGradientH((int)sv.x, (int)sv.y, (int)sv.width, (int)sv.height,
                         WHITE, hueColor);
  DrawRectangleGradientV((int)sv.x, (int)sv.y, (int)sv.width, (int)sv.height,
                         (Color){0, 0, 0, 0}, BLACK);

  for (int i = 0; i < 6; i++) {
    float y0 = hue.y + (hue.height / 6.0f) * (float)i;
    float y1 = hue.y + (hue.height / 6.0f) * (float)(i + 1);
    Color c0 = ColorFromHSV((float)i / 6.0f * 360.0f, 1, 1);
    Color c1 = ColorFromHSV((float)(i + 1) / 6.0f * 360.0f, 1, 1);
    DrawRectangleGradientV((int)hue.x, (int)y0, (int)hue.width, (int)(y1 - y0),
                           c0, c1);
  }
  DrawRectangleLinesEx(hue, 1, t.border);

  Vector2 m = GetMousePosition();
  bool down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
  if (down && CheckCollisionPointRec(m, sv)) {
    gui->satValue = Clamp((m.x - sv.x) / sv.width, 0, 1);
    gui->valValue = Clamp(1.0f - (m.y - sv.y) / sv.height, 0, 1);
    gui->currentColor =
        ColorFromHSV(gui->hueValue, gui->satValue, gui->valValue);
  }
  if (down && CheckCollisionPointRec(m, hue)) {
    gui->hueValue = Clamp((m.y - hue.y) / hue.height, 0, 1) * 360.0f;
    gui->currentColor =
        ColorFromHSV(gui->hueValue, gui->satValue, gui->valValue);
  }

  Vector2 svPos = {sv.x + gui->satValue * sv.width,
                   sv.y + (1.0f - gui->valValue) * sv.height};
  DrawCircleLines((int)svPos.x, (int)svPos.y, 5, t.text);
  float hueY = hue.y + (gui->hueValue / 360.0f) * hue.height;
  DrawRectangleLinesEx((Rectangle){hue.x - 2, hueY - 2, hue.width + 4, 4}, 1,
                       t.text);

  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
      !CheckCollisionPointRec(m, gui->colorPickerRect) &&
      !CheckCollisionPointRec(m, gui->paletteButtonRect) &&
      !CheckCollisionPointRec(m, gui->colorButtonRect)) {
    gui->showColorPicker = false;
  }
}


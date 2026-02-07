#include "gui_internal.h"

void GuiDrawFooter(GuiState *gui, Canvas *canvas, Theme t, int sw, int sh) {
  Rectangle footer = {0, (float)sh - 28, (float)sw, 28};
  DrawRectangleRec(footer, ColorAlpha(t.surface, gui->darkMode ? 0.85f : 0.92f));
  DrawLineEx((Vector2){0, footer.y}, (Vector2){(float)sw, footer.y}, 1, t.border);

  Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), canvas->camera);
  int totalPoints = GetTotalPoints(canvas);
  const float fontSize = 12.0f;
  Color textColor = gui->darkMode ? t.text : t.textDim;
  DrawTextEx(gui->uiFont,
             TextFormat("Pos: %.0f, %.0f", mouseWorld.x, mouseWorld.y),
             (Vector2){12, (float)sh - 20}, fontSize, 1.0f, textColor);
  DrawTextEx(gui->uiFont, TextFormat("Zoom: %.2f", canvas->camera.zoom),
             (Vector2){150, (float)sh - 20}, fontSize, 1.0f, textColor);
  DrawTextEx(gui->uiFont, TextFormat("FPS: %d", GetFPS()),
             (Vector2){280, (float)sh - 20}, fontSize, 1.0f, textColor);
  DrawTextEx(gui->uiFont, TextFormat("Strokes: %d", canvas->strokeCount),
             (Vector2){(float)sw - 230, (float)sh - 20}, fontSize, 1.0f,
             textColor);
  DrawTextEx(gui->uiFont, TextFormat("Points: %d", totalPoints),
             (Vector2){(float)sw - 110, (float)sh - 20}, fontSize, 1.0f,
             textColor);

  if (gui->toastUntil > GetTime() && gui->toast[0] != '\0') {
    float tw = MeasureTextEx(gui->uiFont, gui->toast, 12, 1.0f).x;
    Rectangle toast = {(float)sw - tw - 30, 92, tw + 20, 26};
    DrawRectangleRounded(toast, 0.25f, 6, ColorAlpha(t.surface, 0.95f));
    DrawRectangleRoundedLinesEx(toast, 0.25f, 6, 1.0f, t.border);
    DrawTextEx(gui->uiFont, gui->toast, (Vector2){toast.x + 10, toast.y + 7}, 12,
               1.0f, t.text);
  }
}

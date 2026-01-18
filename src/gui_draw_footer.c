#include "gui_internal.h"

void GuiDrawFooter(GuiState *gui, Canvas *canvas, Theme t, int sw, int sh) {
  Rectangle footer = {0, (float)sh - 24, (float)sw, 24};
  DrawRectangleRec(footer, ColorAlpha(t.surface, gui->darkMode ? 0.75f : 0.85f));
  DrawLineEx((Vector2){0, footer.y}, (Vector2){(float)sw, footer.y}, 1, t.border);

  Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), canvas->camera);
  int totalPoints = GetTotalPoints(canvas);
  DrawText(TextFormat("Pos: %.0f, %.0f", mouseWorld.x, mouseWorld.y), 10,
           sh - 18, 10, t.textDim);
  DrawText(TextFormat("Zoom: %.2f", canvas->camera.zoom), 130, sh - 18, 10,
           t.textDim);
  DrawText(TextFormat("FPS: %d", GetFPS()), 240, sh - 18, 10, t.textDim);
  DrawText(TextFormat("Strokes: %d", canvas->strokeCount), sw - 200, sh - 18, 10,
           t.textDim);
  DrawText(TextFormat("Points: %d", totalPoints), sw - 100, sh - 18, 10,
           t.textDim);

  if (gui->toastUntil > GetTime() && gui->toast[0] != '\0') {
    int tw = MeasureText(gui->toast, 12);
    Rectangle toast = {(float)sw - (float)tw - 30, 92, (float)tw + 20, 26};
    DrawRectangleRounded(toast, 0.25f, 6, ColorAlpha(t.surface, 0.95f));
    DrawRectangleRoundedLinesEx(toast, 0.25f, 6, 1.0f, t.border);
    DrawText(gui->toast, (int)toast.x + 10, (int)toast.y + 7, 12, t.text);
  }
}


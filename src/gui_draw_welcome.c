#include "gui_internal.h"

static bool TextButton(Rectangle r, const char *label, Theme t, bool primary) {
  Vector2 mouse = GetMousePosition();
  bool hover = CheckCollisionPointRec(mouse, r);

  Color bg = primary ? t.primary : t.tab;
  if (!primary && hover)
    bg = t.hover;

  DrawRectangleRounded(r, 0.25f, 8, bg);
  DrawRectangleRoundedLinesEx(r, 0.25f, 8, 1.0f, t.border);

  int fontSize = 12;
  int tw = MeasureText(label, fontSize);
  Color textColor = primary ? BLACK : t.text;
  DrawText(label, (int)(r.x + (r.width - (float)tw) / 2.0f),
           (int)(r.y + (r.height - (float)fontSize) / 2.0f), fontSize, textColor);

  return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static float DrawLineText(float x, float y, const char *text, int fontSize,
                          Color c) {
  DrawText(text, (int)x, (int)y, fontSize, c);
  return y + (float)fontSize + 6.0f;
}

void GuiDrawWelcome(GuiState *gui, Canvas *canvas, Theme t, int sw, int sh) {
  (void)canvas;

  Color overlay = ColorAlpha(BLACK, gui->darkMode ? 0.55f : 0.35f);
  DrawRectangle(0, 0, sw, sh, overlay);

  float margin = 24.0f;
  float w = (float)sw - margin * 2.0f;
  float h = (float)sh - margin * 2.0f;
  if (w < 320.0f)
    w = 320.0f;
  if (h < 260.0f)
    h = 260.0f;

  float cardW = w < 640.0f ? w : 640.0f;
  float cardH = h < 420.0f ? h : 420.0f;
  Rectangle card = {(float)sw / 2.0f - cardW / 2.0f,
                    (float)sh / 2.0f - cardH / 2.0f, cardW, cardH};

  DrawRectangleRounded(card, 0.05f, 8, t.surface);
  DrawRectangleRoundedLinesEx(card, 0.05f, 8, 1.0f, t.border);

  float pad = 18.0f;
  float x = card.x + pad;
  float y = card.y + pad;

  y = DrawLineText(x, y, "Welcome to cdraw", 20, t.text);
  y = DrawLineText(x, y, "A lightweight vector sketchpad with an infinite canvas.",
                   12, t.textDim);
  y += 6.0f;

  y = DrawLineText(x, y, "Quick shortcuts:", 12, t.text);
  y = DrawLineText(x, y, "  P Pen   B Rectangle   C Circle   A Arrow/Line", 12,
                   t.textDim);
  y = DrawLineText(x, y, "  E Eraser   M Move/Select", 12, t.textDim);
  y += 6.0f;

  y = DrawLineText(x, y, "Navigation:", 12, t.text);
  y = DrawLineText(x, y, "  Wheel = zoom   Right-drag / Space+drag = pan", 12,
                   t.textDim);
  y += 6.0f;

  y = DrawLineText(x, y, "File:", 12, t.text);
  y = DrawLineText(x, y, "  Ctrl+S = save   Ctrl+O = open   Ctrl+N = new", 12,
                   t.textDim);
  y += 6.0f;

  y = DrawLineText(x, y, "Tip: Press G to toggle the grid, and F for fullscreen.",
                   12, t.textDim);

  float btnH = 34.0f;
  Rectangle primaryBtn = {card.x + card.width - pad - 130.0f,
                          card.y + card.height - pad - btnH, 130.0f, btnH};

  if (TextButton(primaryBtn, "Let's draw", t, true) ||
      IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
    gui->showWelcome = false;
    gui->hasSeenWelcome = true;
    GuiToastSet(gui, "Welcome!");
  }
}

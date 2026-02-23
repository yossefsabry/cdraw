#include "gui_internal.h"
#include "ai/ai_panel.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static bool AiQuickButton(GuiState *gui, Rectangle r, Theme t) {
  Vector2 mouse = GetMousePosition();
  bool hover = CheckCollisionPointRec(mouse, r);
  bool busy = gui->aiBusy;
  Color bg = ColorAlpha(t.surface, 0.92f);
  if (hover && !busy)
    bg = t.hover;
  DrawRectangleRounded(r, 0.35f, 8, bg);
  DrawRectangleRoundedLinesEx(r, 0.35f, 8, 1.0f, t.primary);
  const char *label = busy ? "AI..." : "AI";
  Color text = busy ? t.textDim : t.text;
  Vector2 size = MeasureTextEx(gui->uiFont, label, 12, 1.0f);
  float tx = r.x + (r.width - size.x) * 0.5f;
  float ty = r.y + (r.height - size.y) * 0.5f;
  DrawTextEx(gui->uiFont, label, (Vector2){tx, ty}, 12, 1.0f, text);
  if (!hover || busy)
    return false;
  return IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

typedef enum {
  TOAST_NEUTRAL = 0,
  TOAST_INFO,
  TOAST_SUCCESS,
  TOAST_WARNING,
  TOAST_ERROR
} ToastKind;

static bool StrContainsInsensitive(const char *text, const char *needle) {
  if (!text || !needle || needle[0] == '\0')
    return false;
  size_t nlen = strlen(needle);
  for (const char *p = text; *p; p++) {
    size_t i = 0;
    while (p[i] && i < nlen) {
      unsigned char a = (unsigned char)p[i];
      unsigned char b = (unsigned char)needle[i];
      if (tolower(a) != tolower(b))
        break;
      i++;
    }
    if (i == nlen)
      return true;
  }
  return false;
}

static bool StrStartsWithInsensitive(const char *text, const char *prefix) {
  if (!text || !prefix || prefix[0] == '\0')
    return false;
  for (size_t i = 0; prefix[i] != '\0'; i++) {
    unsigned char a = (unsigned char)text[i];
    unsigned char b = (unsigned char)prefix[i];
    if (a == '\0')
      return false;
    if (tolower(a) != tolower(b))
      return false;
  }
  return true;
}

static ToastKind ToastKindFromText(const char *msg) {
  if (!msg || msg[0] == '\0')
    return TOAST_NEUTRAL;
  bool isAi = StrStartsWithInsensitive(msg, "AI");
  if (!isAi)
    return TOAST_NEUTRAL;
  if (StrContainsInsensitive(msg, "failed") ||
      StrContainsInsensitive(msg, "error"))
    return TOAST_ERROR;
  if (StrContainsInsensitive(msg, "ready") ||
      StrContainsInsensitive(msg, "saved") ||
      StrContainsInsensitive(msg, "cleared"))
    return TOAST_SUCCESS;
  if (StrContainsInsensitive(msg, "cooldown") ||
      StrContainsInsensitive(msg, "needed"))
    return TOAST_WARNING;
  return TOAST_INFO;
}

void GuiDrawFooter(GuiState *gui, Canvas *canvas, Theme t, int sw, int sh) {
  Rectangle footer = {0, (float)sh - 28, (float)sw, 28};
  DrawRectangleRec(footer, ColorAlpha(t.surface, gui->darkMode ? 0.85f : 0.92f));
  DrawLineEx((Vector2){0, footer.y}, (Vector2){(float)sw, footer.y}, 1, t.border);

  Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), canvas->camera);
  int totalPoints = GetTotalPoints(canvas);
  const float fontSize = 12.0f;
  Color textColor = gui->darkMode ? t.text : t.textDim;

  char posText[64];
  char zoomText[32];
  char fpsText[24];
  char strokesText[32];
  char pointsText[32];
  snprintf(posText, sizeof(posText), "Pos: %.0f, %.0f", mouseWorld.x, mouseWorld.y);
  snprintf(zoomText, sizeof(zoomText), "Zoom: %.2f", canvas->camera.zoom);
  snprintf(fpsText, sizeof(fpsText), "FPS: %d", GetFPS());
  snprintf(strokesText, sizeof(strokesText), "Strokes: %d", canvas->strokeCount);
  snprintf(pointsText, sizeof(pointsText), "Points: %d", totalPoints);

  float leftX = 12.0f;
  float rightX = (float)sw - 12.0f;
  float y = (float)sh - 20.0f;
  float gap = 12.0f;

  const char *rightItems[] = {pointsText, strokesText};
  for (int i = 0; i < 2; i++) {
    Vector2 size = MeasureTextEx(gui->uiFont, rightItems[i], fontSize, 1.0f);
    if (rightX - size.x < leftX + 20.0f)
      continue;
    rightX -= size.x;
    DrawTextEx(gui->uiFont, rightItems[i], (Vector2){rightX, y}, fontSize, 1.0f,
               textColor);
    rightX -= gap;
  }

  const char *leftItems[] = {posText, zoomText, fpsText};
  for (int i = 0; i < 3; i++) {
    Vector2 size = MeasureTextEx(gui->uiFont, leftItems[i], fontSize, 1.0f);
    if (leftX + size.x > rightX - 8.0f)
      break;
    DrawTextEx(gui->uiFont, leftItems[i], (Vector2){leftX, y}, fontSize, 1.0f,
               textColor);
    leftX += size.x + gap;
  }

  gui->aiQuickButtonRect = (Rectangle){0, 0, 0, 0};
  if (gui->aiReady) {
    const float btnW = 92.0f;
    const float btnH = 32.0f;
    const float margin = 16.0f;
    float bx = (float)sw - btnW - margin;
    float by = (float)sh - 28.0f - margin - btnH;
    if (by < 96.0f)
      by = 96.0f;
    Rectangle btn = {bx, by, btnW, btnH};
    gui->aiQuickButtonRect = btn;
    if (AiQuickButton(gui, btn, t))
      AiPanelRequest(gui, canvas, false);
  }

  if (gui->toastNext[0] != '\0' &&
      gui->toastUntil <= GetTime()) {
    snprintf(gui->toast, sizeof(gui->toast), "%s", gui->toastNext);
    gui->toastNext[0] = '\0';
    gui->toastUntil = GetTime() + 2.5;
  }
  if (gui->toastUntil > GetTime() && gui->toast[0] != '\0') {
    const float fontSize = 12.0f;
    Vector2 textSize = MeasureTextEx(gui->uiFont, gui->toast, fontSize, 1.0f);
    float padX = 12.0f;
    float padY = 7.0f;
    float barW = 4.0f;
    float gap = 8.0f;
    float tw = textSize.x + padX * 2.0f + barW + gap;
    float th = textSize.y + padY * 2.0f;
    float topH = 88.0f;
    float rulerH = gui->showRulers ? 24.0f : 0.0f;
    float y = topH + rulerH + 8.0f;
    float x = (float)sw - tw - 16.0f;
    Rectangle toast = {x, y, tw, th};

    ToastKind kind = ToastKindFromText(gui->toast);
    Color accent = t.primary;
    if (kind == TOAST_SUCCESS)
      accent = (Color){34, 197, 94, 255};
    else if (kind == TOAST_WARNING)
      accent = (Color){245, 158, 11, 255};
    else if (kind == TOAST_ERROR)
      accent = (Color){239, 68, 68, 255};
    else if (kind == TOAST_NEUTRAL)
      accent = t.border;

    Color base = ColorAlpha(t.surface, gui->darkMode ? 0.96f : 0.98f);
    Color tint = ColorAlpha(accent, gui->darkMode ? 0.14f : 0.08f);
    DrawRectangleRounded(toast, 0.28f, 8, base);
    DrawRectangleRounded(toast, 0.28f, 8, tint);
    DrawRectangleRoundedLinesEx(toast, 0.28f, 8, 1.0f, ColorAlpha(accent, 0.9f));

    Rectangle bar = {toast.x + 1.0f, toast.y + 1.0f, barW, toast.height - 2.0f};
    DrawRectangleRec(bar, accent);

    float tx = toast.x + padX + barW + gap;
    float ty = toast.y + (toast.height - textSize.y) * 0.5f;
    DrawTextEx(gui->uiFont, gui->toast, (Vector2){tx, ty}, fontSize, 1.0f, t.text);
  }
}

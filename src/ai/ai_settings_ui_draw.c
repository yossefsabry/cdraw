#include "ai_settings_ui.h"
#include "ai_settings.h"
#include <stdio.h>
#include <string.h>

typedef struct {
  Rectangle panel;
  Rectangle provGem;
  Rectangle provLocal;
  Rectangle model;
  Rectangle key;
  Rectangle base;
  Rectangle save;
  Rectangle clear;
  Rectangle close;
  Rectangle reveal;
} AiUiLayout;

static void CopyStr(char *dst, size_t cap,
                    const char *src) {
  if (!dst || cap == 0) return;
  if (!src) {
    dst[0] = '\0';
    return;
  }
  size_t len = strlen(src);
  if (len >= cap)
    len = cap - 1;
  memcpy(dst, src, len);
  dst[len] = '\0';
}

static void MaskKey(const char *key, char *out,
                    size_t out_sz) {
  if (!out || out_sz == 0)
    return;
  out[0] = '\0';
  if (!key || key[0] == '\0')
    return;
  size_t len = strlen(key);
  const char *tail = key + (len > 4 ? len - 4 : 0);
  snprintf(out, out_sz, "****%s", tail);
}

static AiUiLayout Layout(int sw, int sh) {
  AiUiLayout l;
  const float top = 88.0f;
  const float foot = 24.0f;
  const float margin = 24.0f;
  float maxW = (float)sw - margin * 2.0f;
  float w = 420.0f;
  if (w > maxW) w = maxW;
  if (w < 260.0f) w = (float)sw - 8.0f;
  float maxH = (float)sh - top - foot -
               margin * 2.0f;
  float h = 320.0f;
  if (h > maxH) h = maxH;
  if (h < 200.0f) h = 200.0f;
  float x = (float)sw - w - margin;
  if (x < 4.0f) x = 4.0f;
  float y = top + margin;
  l.panel = (Rectangle){x, y, w, h};

  float pad = 16.0f;
  float row = 30.0f;
  float field = 32.0f;
  float fx = x + pad;
  float fy = y + 44.0f;
  float fw = w - pad * 2.0f;
  float gap = 8.0f;
  float half = (fw - gap) * 0.5f;

  l.provGem = (Rectangle){fx, fy, half, row};
  l.provLocal = (Rectangle){fx + half + gap,
                            fy, half, row};
  fy += row + 16.0f;
  l.model = (Rectangle){fx, fy, fw, field};
  fy += field + 14.0f;
  l.key = (Rectangle){fx, fy, fw, field};
  fy += field + 14.0f;
  l.base = (Rectangle){fx, fy, fw, field};
  fy += field + 18.0f;
  l.save = (Rectangle){fx, fy, 80, row};
  l.clear = (Rectangle){fx + 88, fy, 80, row};
  l.close = (Rectangle){x + w - pad - 80,
                        fy, 80, row};
  l.reveal = (Rectangle){x + w - pad - 74,
                         y + 8, 74, 26};
  return l;
}

static void DrawField(GuiState *gui, Rectangle r,
                      const char *label, const char *val,
                      bool active, bool dim, Theme t) {
  Color bg = dim ?
             ColorAlpha(t.surface, 0.6f) :
             t.surface;
  DrawRectangleRounded(r, 0.18f, 6, bg);
  Color border = active ? t.primary : t.border;
  DrawRectangleRoundedLinesEx(r, 0.18f, 6,
                              1.0f, border);
  DrawTextEx(gui->uiFont, label,
             (Vector2){r.x, r.y - 18},
             12, 1.0f, t.textDim);
  Color text = dim ? t.textDim : t.text;
  const char *show = (val && val[0]) ? val : "";
  DrawTextEx(gui->uiFont, show,
             (Vector2){r.x + 10,
                       r.y + 8},
             12, 1.0f, text);
}

static void Btn(GuiState *gui, Rectangle r,
                const char *label, Theme t,
                bool active) {
  Vector2 m = GetMousePosition();
  bool hover = CheckCollisionPointRec(m, r);
  Color bg = active ? t.primary : t.surface;
  if (hover && !active)
    bg = t.hover;
  DrawRectangleRounded(r, 0.25f, 6, bg);
  DrawRectangleRoundedLinesEx(r, 0.25f, 6,
                              1.0f, t.border);
  Color text = active ? BLACK : t.text;
  Vector2 size = MeasureTextEx(gui->uiFont,
                               label, 12, 1.0f);
  float tx = r.x + (r.width - size.x) * 0.5f;
  float ty = r.y + (r.height - size.y) * 0.5f;
  DrawTextEx(gui->uiFont, label,
             (Vector2){tx, ty}, 12, 1.0f, text);
}

void AiSettingsUiDraw(GuiState *gui, Theme t,
                      int sw, int sh) {
  if (!gui || !gui->showAiSettings)
    return;
  AiUiLayout l = Layout(sw, sh);
  gui->aiSettingsRect = l.panel;

  DrawRectangleRounded(l.panel, 0.06f, 10,
                       ColorAlpha(t.surface, 0.98f));
  DrawRectangleRoundedLinesEx(l.panel, 0.06f, 10,
                              1.0f, t.border);
  DrawTextEx(gui->uiFont, "AI Settings",
             (Vector2){l.panel.x + 16, l.panel.y + 12},
             18, 1.0f, t.text);

  Btn(gui, l.provGem, "Gemini", t,
      gui->aiProvider == AI_PROVIDER_GEMINI);
  Btn(gui, l.provLocal, "Local", t,
      gui->aiProvider == AI_PROVIDER_LOCAL);

  char keyView[64];
  if (gui->aiKeyReveal)
    CopyStr(keyView, sizeof(keyView), gui->aiKey);
  else
    MaskKey(gui->aiKey, keyView, sizeof(keyView));

  DrawField(gui, l.model, "Model",
            gui->aiModel,
            gui->aiInputFocus == 1,
            false, t);
  DrawField(gui, l.key, "API key",
            keyView,
            gui->aiInputFocus == 2,
            false, t);
  bool dim = gui->aiProvider != AI_PROVIDER_LOCAL;
  DrawField(gui, l.base, "Base URL",
            gui->aiBase,
            gui->aiInputFocus == 3,
            dim, t);

  Btn(gui, l.save, "Save", t, false);
  Btn(gui, l.clear, "Clear", t, false);
  Btn(gui, l.close, "Close", t, false);
  Btn(gui, l.reveal,
      gui->aiKeyReveal ? "Hide"
                       : "Show",
      t, false);

  if (gui->aiStatus[0] != '\0') {
    DrawTextEx(gui->uiFont, gui->aiStatus,
               (Vector2){l.panel.x + 16,
                         l.panel.y + l.panel.height - 22},
               12, 1.0f, t.textDim);
  }
}

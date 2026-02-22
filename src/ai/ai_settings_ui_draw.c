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
  if (!dst || cap == 0)
    return;
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
  const char *tail =
      key + (len > 4 ? len - 4 : 0);
  snprintf(out, out_sz, "****%s", tail);
}
static AiUiLayout Layout(int sw, int sh, int provider) {
  AiUiLayout l;
  const float top = 88.0f;
  const float foot = 24.0f;
  const float margin = 24.0f;
  float maxW = (float)sw - margin * 2.0f;
  float w = 420.0f;
  if (w > maxW) w = maxW;
  if (w < 260.0f)
    w = (float)sw - 8.0f;
  float x = (float)sw - w - margin;
  if (x < 4.0f)
    x = 4.0f;
  float y = top + margin;

  float pad = 18.0f;
  float row = 32.0f;
  float field = 36.0f;
  float fx = x + pad;
  float fy = y + 58.0f;
  float fw = w - pad * 2.0f;
  float gap = 10.0f;
  float half = (fw - gap) * 0.5f;

  l.provGem = (Rectangle){fx, fy, half, row};
  l.provLocal = (Rectangle){fx + half + gap,
                            fy, half, row};
  fy += row + 22.0f;
  bool showModel = provider == AI_PROVIDER_LOCAL;
  bool showBase = provider == AI_PROVIDER_LOCAL;
  l.model = (Rectangle){0, 0, 0, 0};
  l.base = (Rectangle){0, 0, 0, 0};
  if (showModel) {
    l.model = (Rectangle){fx, fy, fw, field};
    fy += field + 22.0f;
  }
  l.key = (Rectangle){fx, fy, fw, field};
  fy += field;
  if (showBase) {
    fy += 22.0f;
    l.base = (Rectangle){fx, fy, fw, field};
    fy += field + 18.0f;
  } else {
    fy += 18.0f;
  }
  float btnW = (fw - gap * 2.0f) / 3.0f;
  l.save = (Rectangle){fx, fy, btnW, row};
  l.clear = (Rectangle){fx + btnW + gap,
                        fy, btnW, row};
  l.close = (Rectangle){fx + (btnW + gap) * 2.0f,
                        fy, btnW, row};
  l.reveal = (Rectangle){x + w - pad - 64,
                         y + 10, 64, 24};
  float maxH = (float)sh - top - foot -
               margin * 2.0f;
  float contentH = (fy - y) + row + 58.0f;
  float h = contentH;
  if (h > maxH) h = maxH;
  if (h < 200.0f) h = 200.0f;
  l.panel = (Rectangle){x, y, w, h};
  return l;
}
static void DrawField(GuiState *gui, Rectangle r,
                      const char *label, const char *val,
                      bool active, bool dim, bool selectAll,
                      Theme t) {
  Color bg = dim ?
             ColorAlpha(t.surface, 0.6f) :
             t.surface;
  DrawRectangleRounded(r, 0.18f, 6, bg);
  Color border = active ?
                 t.primary : t.border;
  DrawRectangleRoundedLinesEx(r, 0.18f, 6,
                              1.0f, border);
  float labelOffset = 20.0f;
  DrawTextEx(gui->uiFont, label,
             (Vector2){r.x, r.y - labelOffset},
             12, 1.0f, t.textDim);
  Color text = dim ? t.textDim : t.text;
  const char *show =
      (val && val[0]) ? val : "";
  float padX = 10.0f;
  float innerX = r.x + padX;
  float innerW = r.width - padX * 2.0f;
  Vector2 size = MeasureTextEx(gui->uiFont, show, 12, 1.0f);
  Vector2 cap = MeasureTextEx(gui->uiFont, "A", 12, 1.0f);
  float textH = size.y > 0.0f ? size.y : cap.y;
  float textX = innerX;
  if (size.x > innerW)
    textX = innerX - (size.x - innerW);
  float textY = r.y + (r.height - textH) * 0.5f;
  BeginScissorMode((int)innerX, (int)r.y,
                   (int)innerW, (int)r.height);
  if (selectAll && show[0] != '\0') {
    Rectangle sel = {innerX, r.y + 4.0f,
                     innerW, r.height - 8.0f};
    DrawRectangleRec(sel, ColorAlpha(t.primary, 0.25f));
  }
  DrawTextEx(gui->uiFont, show,
             (Vector2){textX, textY},
             12, 1.0f, text);
  if (active && !selectAll) {
    double now = GetTime();
    bool showCaret = ((int)(now * 2.0)) % 2 == 0;
    if (showCaret) {
      float caretX = textX + size.x + 2.0f;
      if (caretX < innerX + 2.0f)
        caretX = innerX + 2.0f;
      float caretMax = innerX + innerW - 2.0f;
      if (caretX > caretMax)
        caretX = caretMax;
      float caretY = r.y + (r.height - textH) * 0.5f;
      DrawLineEx((Vector2){caretX, caretY},
                 (Vector2){caretX, caretY + textH},
                 1.5f, t.primary);
    }
  }
  EndScissorMode();
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
  Vector2 size =
      MeasureTextEx(gui->uiFont,
                    label, 12, 1.0f);
  float tx = r.x +
             (r.width - size.x) * 0.5f;
  float ty = r.y +
             (r.height - size.y) * 0.5f;
  DrawTextEx(gui->uiFont, label,
             (Vector2){tx, ty}, 12, 1.0f, text);
}

void AiSettingsUiDraw(GuiState *gui, Theme t,
                      int sw, int sh) {
  if (!gui || !gui->showAiSettings)
    return;
  AiUiLayout l = Layout(sw, sh, gui->aiProvider);
  gui->aiSettingsRect = l.panel;

  DrawRectangleRounded(l.panel, 0.06f, 10,
                       ColorAlpha(t.surface,
                                  0.98f));
  DrawRectangleRoundedLinesEx(l.panel, 0.06f, 10,
                              1.0f, t.border);
  DrawTextEx(gui->uiFont, "AI Settings",
             (Vector2){l.panel.x + 16,
                       l.panel.y + 12},
             18, 1.0f, t.text);

  Btn(gui, l.provGem, "Gemini", t,
      gui->aiProvider ==
      AI_PROVIDER_GEMINI);
  Btn(gui, l.provLocal, "Local", t,
      gui->aiProvider ==
      AI_PROVIDER_LOCAL);

  char keyView[64];
  if (gui->aiKeyReveal)
    CopyStr(keyView, sizeof(keyView), gui->aiKey);
  else
    MaskKey(gui->aiKey, keyView, sizeof(keyView));

  const char *keyLabel =
      (gui->aiProvider == AI_PROVIDER_LOCAL)
          ? "API key (optional)"
          : "API key";
  const char *baseLabel =
      (gui->aiProvider == AI_PROVIDER_LOCAL)
          ? "Base URL (OpenAI /v1)"
          : "Base URL";
  if (gui->aiProvider == AI_PROVIDER_LOCAL) {
    DrawField(gui, l.model, "Model",
              gui->aiModel,
              gui->aiInputFocus == 1,
              false,
              gui->aiSelectAllField == 1, t);
  }
  DrawField(gui, l.key, keyLabel,
            keyView,
            gui->aiInputFocus == 2,
            false,
            gui->aiSelectAllField == 2, t);
  if (gui->aiProvider == AI_PROVIDER_LOCAL) {
    DrawField(gui, l.base, baseLabel,
              gui->aiBase,
              gui->aiInputFocus == 3,
              false,
              gui->aiSelectAllField == 3, t);
  }

  Btn(gui, l.save, "Save", t, false);
  Btn(gui, l.clear, "Clear", t, false);
  Btn(gui, l.close, "Close", t, false);
  Btn(gui, l.reveal,
      gui->aiKeyReveal ? "Hide"
                       : "Show",
      t, false);

  {
    bool showHints = gui->aiProvider == AI_PROVIDER_LOCAL;
    bool showStatus = gui->aiStatus[0] != '\0';
    int lines = (showHints ? 2 : 0) + (showStatus ? 1 : 0);
    if (lines > 0) {
      float lineH = 12.0f;
      float infoY = l.close.y + l.close.height + 8.0f;
      float infoMax = l.panel.y + l.panel.height - 12.0f;
      float lastLine = infoY + (lines - 1) * lineH;
      if (lastLine > infoMax)
        infoY = infoMax - (lines - 1) * lineH;
      float x = l.panel.x + 16.0f;
      float y = infoY;
      if (showHints) {
        DrawTextEx(gui->uiFont,
                   "Local models use an OpenAI endpoint.",
                   (Vector2){x, y},
                   11, 1.0f, t.textDim);
        DrawTextEx(gui->uiFont,
                   "Example: http://localhost:11434/v1",
                   (Vector2){x, y + lineH},
                   11, 1.0f, t.textDim);
        y += lineH * 2.0f;
      }
      if (showStatus) {
        DrawTextEx(gui->uiFont, gui->aiStatus,
                   (Vector2){x, y},
                   12, 1.0f, t.textDim);
      }
    }
  }
}

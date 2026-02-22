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
static void CopyStr(char *dst, size_t cap, const char *src) {
  if (!dst || cap == 0) return;
  if (!src) { dst[0] = '\0'; return; }
  size_t len = strlen(src);
  if (len >= cap) len = cap - 1;
  memcpy(dst, src, len);
  dst[len] = '\0';
}
static void AppendChar(char *buf, size_t cap, int c) {
  size_t len = strlen(buf);
  if (len + 1 >= cap) return;
  buf[len] = (char)c;
  buf[len + 1] = '\0';
}
static void Backspace(char *buf) {
  size_t len = strlen(buf);
  if (len == 0) return;
  buf[len - 1] = '\0';
}
static void PasteText(char *buf, size_t cap) {
  const char *clip = GetClipboardText();
  if (!clip || clip[0] == '\0') return;
  for (const char *p = clip; *p; p++) {
    unsigned char c = (unsigned char)*p;
    if (c >= 32 && c <= 126)
      AppendChar(buf, cap, (int)c);
  }
}
static void InputText(char *buf, size_t cap) {
  int c = GetCharPressed();
  while (c > 0) {
    if (c >= 32 && c <= 126)
      AppendChar(buf, cap, c);
    c = GetCharPressed();
  }
  bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) ||
              IsKeyDown(KEY_RIGHT_CONTROL);
  bool sup = IsKeyDown(KEY_LEFT_SUPER) ||
             IsKeyDown(KEY_RIGHT_SUPER);
  if ((ctrl || sup) && IsKeyPressed(KEY_V))
    PasteText(buf, cap);
  if (IsKeyPressed(KEY_BACKSPACE))
    Backspace(buf);
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
  float maxH = (float)sh - top - foot - margin * 2.0f;
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
  l.clear = (Rectangle){fx + 88,
                        fy, 80, row};
  l.close = (Rectangle){x + w - pad - 80,
                        fy, 80, row};
  l.reveal = (Rectangle){x + w - pad - 74,
                         y + 8, 74, 26};
  return l;
}
void AiSettingsUiOpen(GuiState *gui) {
  if (!gui) return;
  AiSettings s;
  char err[64];
  (void)AiSettingsLoad(&s, err, sizeof(err));
  gui->aiProvider = (int)s.provider;
  CopyStr(gui->aiModel, sizeof(gui->aiModel), s.model);
  CopyStr(gui->aiKey, sizeof(gui->aiKey), s.api_key);
  CopyStr(gui->aiBase, sizeof(gui->aiBase), s.base_url);
  if (gui->aiProvider == AI_PROVIDER_LOCAL &&
      gui->aiBase[0] == '\0') {
    CopyStr(gui->aiBase, sizeof(gui->aiBase),
            "http://localhost:11434/v1");
  }
  gui->aiStatus[0] = '\0';
  gui->showAiSettings = true; gui->showAiPanel = false;
  gui->aiInputFocus =
      (gui->aiKey[0] == '\0') ? 2 : 1;
}
void AiSettingsUiClear(GuiState *gui) {
  if (!gui) return;
  gui->aiModel[0] = '\0'; gui->aiKey[0] = '\0';
  gui->aiBase[0] = '\0'; gui->aiProvider = 0;
  gui->aiStatus[0] = '\0';
}
void AiSettingsUiUpdate(GuiState *gui, int sw, int sh) {
  if (!gui) return;
  if (!gui->showAiSettings) {
    if (gui->aiInputFocus != 0)
      gui->aiInputFocus = 0;
    gui->isTyping = false;
    return;
  }
  AiUiLayout l = Layout(sw, sh);
  gui->aiSettingsRect = l.panel;
  Vector2 m = GetMousePosition();
  bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

  if (click && !CheckCollisionPointRec(m, l.panel)) {
    gui->showAiSettings = false;
    gui->aiInputFocus = 0;
    return;
}
  if (click && CheckCollisionPointRec(m, l.model))
    gui->aiInputFocus = 1;
  else if (click && CheckCollisionPointRec(m, l.key))
    gui->aiInputFocus = 2;
  else if (click &&
           CheckCollisionPointRec(m, l.base) &&
           gui->aiProvider == AI_PROVIDER_LOCAL)
    gui->aiInputFocus = 3;
  if (click && CheckCollisionPointRec(m, l.provGem))
    gui->aiProvider = AI_PROVIDER_GEMINI;
  if (click && CheckCollisionPointRec(m, l.provLocal))
    gui->aiProvider = AI_PROVIDER_LOCAL;
  if (click && CheckCollisionPointRec(m, l.reveal))
    gui->aiKeyReveal = !gui->aiKeyReveal;
  if (click && CheckCollisionPointRec(m, l.close)) {
    gui->showAiSettings = false;
    gui->aiInputFocus = 0;
  }
  if (click && CheckCollisionPointRec(m, l.clear)) {
    char err[64];
    (void)AiSettingsClear(err, sizeof(err));
    AiSettingsUiClear(gui);
    snprintf(gui->aiStatus, sizeof(gui->aiStatus), "Cleared");
  }
  if (click && CheckCollisionPointRec(m, l.save)) {
    AiSettings s;
    memset(&s, 0, sizeof(s));
    s.provider = (AiProvider)gui->aiProvider;
    CopyStr(s.model, sizeof(s.model), gui->aiModel);
    CopyStr(s.api_key, sizeof(s.api_key), gui->aiKey);
    CopyStr(s.base_url, sizeof(s.base_url), gui->aiBase);
    char err[64];
    if (AiSettingsSave(&s, err, sizeof(err))) {
      snprintf(gui->aiStatus, sizeof(gui->aiStatus), "Saved");
    } else {
      snprintf(gui->aiStatus, sizeof(gui->aiStatus), "Save failed");
    }
  }
  if (gui->aiProvider != AI_PROVIDER_LOCAL &&
      gui->aiInputFocus == 3)
    gui->aiInputFocus = 0;

  gui->isTyping = gui->aiInputFocus != 0;
  if (gui->aiInputFocus == 1)
    InputText(gui->aiModel, sizeof(gui->aiModel));
  else if (gui->aiInputFocus == 2)
    InputText(gui->aiKey, sizeof(gui->aiKey));
  else if (gui->aiInputFocus == 3 &&
           gui->aiProvider == AI_PROVIDER_LOCAL)
    InputText(gui->aiBase, sizeof(gui->aiBase));
}

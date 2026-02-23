#include "ai_settings_ui.h"
#include "ai_settings.h"
#include <stdio.h>
#include <string.h>
typedef struct {
  Rectangle panel;
  Rectangle provGem;
  Rectangle provOpen;
  Rectangle provLocal;
  Rectangle model;
  Rectangle preset4o;
  Rectangle preset4oMini;
  Rectangle preset41Mini;
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
static const char kGeminiModel[] = "gemini-1.5-flash";
static const char kOpenAIModel[] = "gpt-4o";
static const char kOpenAIBase[] = "https://api.openai.com/v1";
static void AppendChar(char *buf, size_t cap, int c) {
  size_t len = strlen(buf);
  if (len + 1 >= cap) return;
  buf[len] = (char)c;
  buf[len + 1] = '\0';
}
static void ClearText(char *buf) {
  if (!buf) return;
  buf[0] = '\0';
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
static bool InputText(char *buf, size_t cap, bool selectAll) {
  bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) ||
              IsKeyDown(KEY_RIGHT_CONTROL);
  bool sup = IsKeyDown(KEY_LEFT_SUPER) ||
             IsKeyDown(KEY_RIGHT_SUPER);
  bool mod = ctrl || sup;

  if (mod && IsKeyPressed(KEY_A))
    return buf[0] != '\0';

  if (mod && IsKeyPressed(KEY_BACKSPACE)) {
    ClearText(buf);
    return false;
  }

  if (IsKeyPressed(KEY_DELETE)) {
    if (selectAll) {
      ClearText(buf);
      selectAll = false;
    } else {
      Backspace(buf);
    }
  }

  if (IsKeyPressed(KEY_BACKSPACE)) {
    if (selectAll) {
      ClearText(buf);
      selectAll = false;
    } else {
      Backspace(buf);
    }
  }

  if (mod && IsKeyPressed(KEY_V)) {
    if (selectAll) {
      ClearText(buf);
      selectAll = false;
    }
    PasteText(buf, cap);
  }

  int c = GetCharPressed();
  while (c > 0) {
    if (c >= 32 && c <= 126) {
      if (selectAll) {
        ClearText(buf);
        selectAll = false;
      }
      AppendChar(buf, cap, c);
    }
    c = GetCharPressed();
  }
  return selectAll;
}
static AiUiLayout Layout(int sw, int sh, int provider) {
  AiUiLayout l;
  const float top = 88.0f;
  const float foot = 24.0f;
  const float margin = 24.0f;
  float maxW = (float)sw - margin * 2.0f;
  float w = 420.0f;
  if (w > maxW) w = maxW;
  if (w < 260.0f) w = (float)sw - 8.0f;
  float x = (float)sw - w - margin;
  if (x < 4.0f) x = 4.0f;
  float y = top + margin;
  float pad = 18.0f;
  float row = 32.0f;
  float field = 36.0f;
  float fieldGap = 30.0f;
  float presetRow = 26.0f;
  float presetGap = 8.0f;
  float btnTopPad = 28.0f;
  float fx = x + pad;
  float fy = y + 58.0f;
  float fw = w - pad * 2.0f;
  float gap = 10.0f;
  float third = (fw - gap * 2.0f) / 3.0f;

  l.provGem = (Rectangle){fx, fy, third, row};
  l.provOpen = (Rectangle){fx + third + gap,
                           fy, third, row};
  l.provLocal = (Rectangle){fx + (third + gap) * 2.0f,
                            fy, third, row};
  fy += row + 30.0f;
  bool showModel = provider == AI_PROVIDER_LOCAL ||
                   provider == AI_PROVIDER_OPENAI ||
                   provider == AI_PROVIDER_GEMINI;
  bool showBase = provider == AI_PROVIDER_LOCAL;
  l.model = (Rectangle){0, 0, 0, 0};
  l.preset4o = (Rectangle){0, 0, 0, 0};
  l.preset4oMini = (Rectangle){0, 0, 0, 0};
  l.preset41Mini = (Rectangle){0, 0, 0, 0};
  l.base = (Rectangle){0, 0, 0, 0};
  if (showModel) {
    l.model = (Rectangle){fx, fy, fw, field};
    fy += field;
    if (provider == AI_PROVIDER_OPENAI ||
        provider == AI_PROVIDER_GEMINI) {
      fy += 12.0f;
      float pw = (fw - presetGap * 2.0f) / 3.0f;
      l.preset4o = (Rectangle){fx, fy, pw, presetRow};
      l.preset4oMini = (Rectangle){fx + pw + presetGap, fy, pw, presetRow};
      l.preset41Mini = (Rectangle){fx + (pw + presetGap) * 2.0f, fy, pw, presetRow};
      fy += presetRow + fieldGap;
    } else {
      fy += fieldGap;
    }
  }
  l.key = (Rectangle){fx, fy, fw, field};
  fy += field;
  if (showBase) {
    fy += fieldGap;
    l.base = (Rectangle){fx, fy, fw, field};
    fy += field + btnTopPad;
  } else {
    fy += btnTopPad;
  }
  float btnW = (fw - gap * 2.0f) / 3.0f;
  l.save = (Rectangle){fx, fy, btnW, row};
  l.clear = (Rectangle){fx + btnW + gap,
                        fy, btnW, row};
  l.close = (Rectangle){fx + (btnW + gap) * 2.0f,
                        fy, btnW, row};
  l.reveal = (Rectangle){x + w - pad - 64,
                         y + 10, 64, 24};
  float maxH = (float)sh - top - foot - margin * 2.0f;
  float contentH = (fy - y) + row + 58.0f;
  float h = contentH;
  if (h > maxH) h = maxH;
  if (h < 200.0f) h = 200.0f;
  l.panel = (Rectangle){x, y, w, h};
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
  if (gui->aiProvider == AI_PROVIDER_GEMINI &&
      gui->aiModel[0] == '\0')
    CopyStr(gui->aiModel, sizeof(gui->aiModel), kGeminiModel);
  if (gui->aiProvider == AI_PROVIDER_OPENAI &&
      gui->aiModel[0] == '\0')
    CopyStr(gui->aiModel, sizeof(gui->aiModel), kOpenAIModel);
  gui->aiReady = AiSettingsReady(&s, NULL, 0);
  if (gui->aiProvider == AI_PROVIDER_LOCAL &&
      gui->aiBase[0] == '\0') {
    CopyStr(gui->aiBase, sizeof(gui->aiBase),
            "http://localhost:11434/v1");
  }
  if (gui->aiProvider == AI_PROVIDER_OPENAI &&
      gui->aiBase[0] == '\0') {
    CopyStr(gui->aiBase, sizeof(gui->aiBase), kOpenAIBase);
  }
  gui->aiStatus[0] = '\0';
  gui->showAiSettings = true; gui->showAiPanel = false;
  gui->aiSelectAllField = 0;
  if (gui->aiProvider == AI_PROVIDER_LOCAL)
    gui->aiInputFocus = 1;
  else
    gui->aiInputFocus = 2;
}
void AiSettingsUiClear(GuiState *gui) {
  if (!gui) return;
  gui->aiModel[0] = '\0'; gui->aiKey[0] = '\0';
  gui->aiBase[0] = '\0'; gui->aiProvider = 0;
  gui->aiStatus[0] = '\0';
  gui->aiSelectAllField = 0;
  gui->aiReady = false;
}
void AiSettingsUiUpdate(GuiState *gui, int sw, int sh) {
  if (!gui) return;
  if (!gui->showAiSettings) {
    if (gui->aiInputFocus != 0)
      gui->aiInputFocus = 0;
    gui->aiSelectAllField = 0;
    gui->isTyping = false;
    return;
  }
  AiUiLayout l = Layout(sw, sh, gui->aiProvider);
  gui->aiSettingsRect = l.panel;
  Vector2 m = GetMousePosition();
  bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
  int prevFocus = gui->aiInputFocus;
  bool clickedField = false;

  if (click && !CheckCollisionPointRec(m, l.panel)) {
    gui->showAiSettings = false;
    gui->aiInputFocus = 0;
    gui->aiSelectAllField = 0;
    return;
  }
  if (click &&
      (gui->aiProvider == AI_PROVIDER_LOCAL ||
       gui->aiProvider == AI_PROVIDER_OPENAI ||
       gui->aiProvider == AI_PROVIDER_GEMINI) &&
      CheckCollisionPointRec(m, l.model)) {
    gui->aiInputFocus = 1;
    clickedField = true;
  } else if (click && CheckCollisionPointRec(m, l.key)) {
    gui->aiInputFocus = 2;
    clickedField = true;
  } else if (click &&
             CheckCollisionPointRec(m, l.base) &&
             gui->aiProvider == AI_PROVIDER_LOCAL) {
    gui->aiInputFocus = 3;
    clickedField = true;
  }
  if (click && CheckCollisionPointRec(m, l.provGem)) {
    gui->aiProvider = AI_PROVIDER_GEMINI;
    if (gui->aiModel[0] == '\0')
      CopyStr(gui->aiModel, sizeof(gui->aiModel), kGeminiModel);
    gui->aiInputFocus = 2;
    gui->aiSelectAllField = 0;
  }
  if (click && CheckCollisionPointRec(m, l.provOpen)) {
    gui->aiProvider = AI_PROVIDER_OPENAI;
    if (gui->aiModel[0] == '\0')
      CopyStr(gui->aiModel, sizeof(gui->aiModel), kOpenAIModel);
    CopyStr(gui->aiBase, sizeof(gui->aiBase), kOpenAIBase);
    gui->aiInputFocus = 2;
    gui->aiSelectAllField = 0;
  }
  if (click && gui->aiProvider == AI_PROVIDER_OPENAI) {
    if (CheckCollisionPointRec(m, l.preset4o)) {
      CopyStr(gui->aiModel, sizeof(gui->aiModel), "gpt-4o");
      gui->aiInputFocus = 1;
      gui->aiSelectAllField = 0;
    } else if (CheckCollisionPointRec(m, l.preset4oMini)) {
      CopyStr(gui->aiModel, sizeof(gui->aiModel), "gpt-4o-mini");
      gui->aiInputFocus = 1;
      gui->aiSelectAllField = 0;
    } else if (CheckCollisionPointRec(m, l.preset41Mini)) {
      CopyStr(gui->aiModel, sizeof(gui->aiModel), "gpt-4.1-mini");
      gui->aiInputFocus = 1;
      gui->aiSelectAllField = 0;
    }
  }
  if (click && gui->aiProvider == AI_PROVIDER_GEMINI) {
    if (CheckCollisionPointRec(m, l.preset4o)) {
      CopyStr(gui->aiModel, sizeof(gui->aiModel), "gemini-1.5-flash");
      gui->aiInputFocus = 1;
      gui->aiSelectAllField = 0;
    } else if (CheckCollisionPointRec(m, l.preset4oMini)) {
      CopyStr(gui->aiModel, sizeof(gui->aiModel), "gemini-1.5-pro");
      gui->aiInputFocus = 1;
      gui->aiSelectAllField = 0;
    } else if (CheckCollisionPointRec(m, l.preset41Mini)) {
      CopyStr(gui->aiModel, sizeof(gui->aiModel), "gemini-2.0-flash");
      gui->aiInputFocus = 1;
      gui->aiSelectAllField = 0;
    }
  }
  if (click && CheckCollisionPointRec(m, l.provLocal)) {
    gui->aiProvider = AI_PROVIDER_LOCAL;
    if (gui->aiModel[0] == '\0')
      CopyStr(gui->aiModel, sizeof(gui->aiModel), "llama3");
    if (gui->aiBase[0] == '\0')
      CopyStr(gui->aiBase, sizeof(gui->aiBase),
              "http://localhost:11434/v1");
  }
  if (click && CheckCollisionPointRec(m, l.reveal))
    gui->aiKeyReveal = !gui->aiKeyReveal;
  if (click && CheckCollisionPointRec(m, l.close)) {
    gui->showAiSettings = false;
    gui->aiInputFocus = 0;
    gui->aiSelectAllField = 0;
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
    if (gui->aiProvider == AI_PROVIDER_GEMINI) {
      if (gui->aiModel[0] == '\0')
        CopyStr(s.model, sizeof(s.model), kGeminiModel);
      else
        CopyStr(s.model, sizeof(s.model), gui->aiModel);
    } else if (gui->aiProvider == AI_PROVIDER_OPENAI) {
      if (gui->aiModel[0] == '\0')
        CopyStr(s.model, sizeof(s.model), kOpenAIModel);
      else
        CopyStr(s.model, sizeof(s.model), gui->aiModel);
      CopyStr(s.base_url, sizeof(s.base_url), kOpenAIBase);
    } else
      CopyStr(s.model, sizeof(s.model), gui->aiModel);
    CopyStr(s.api_key, sizeof(s.api_key), gui->aiKey);
    if (gui->aiProvider == AI_PROVIDER_LOCAL)
      CopyStr(s.base_url, sizeof(s.base_url), gui->aiBase);
    char err[64];
    if (AiSettingsSave(&s, err, sizeof(err))) {
      gui->aiReady = AiSettingsReady(&s, NULL, 0);
      snprintf(gui->aiStatus, sizeof(gui->aiStatus), "Saved");
    } else {
      snprintf(gui->aiStatus, sizeof(gui->aiStatus), "Save failed");
    }
  }
  if (gui->aiProvider != AI_PROVIDER_LOCAL &&
      gui->aiInputFocus == 3)
    gui->aiInputFocus = 0;

  if (click && !clickedField)
    gui->aiSelectAllField = 0;

  if (prevFocus != gui->aiInputFocus)
    gui->aiSelectAllField = 0;

  gui->isTyping = gui->aiInputFocus != 0;
  if (gui->aiInputFocus == 1 &&
      (gui->aiProvider == AI_PROVIDER_LOCAL ||
       gui->aiProvider == AI_PROVIDER_OPENAI)) {
    bool selectAll = gui->aiSelectAllField == 1;
    selectAll = InputText(gui->aiModel, sizeof(gui->aiModel), selectAll);
    gui->aiSelectAllField = selectAll ? 1 : 0;
  } else if (gui->aiInputFocus == 2) {
    bool selectAll = gui->aiSelectAllField == 2;
    selectAll = InputText(gui->aiKey, sizeof(gui->aiKey), selectAll);
    gui->aiSelectAllField = selectAll ? 2 : 0;
  } else if (gui->aiInputFocus == 3 &&
             gui->aiProvider == AI_PROVIDER_LOCAL) {
    bool selectAll = gui->aiSelectAllField == 3;
    selectAll = InputText(gui->aiBase, sizeof(gui->aiBase), selectAll);
    gui->aiSelectAllField = selectAll ? 3 : 0;
  }
}

#include "ai_panel.h"
#include "ai_client.h"
#include "ai_settings.h"
#include "ai_settings_ui.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static void WrapText(const char *src, char *dst,
                     size_t dst_sz, int width) {
  if (!dst || dst_sz == 0) {
    return;
  }
  dst[0] = '\0';
  if (!src || width <= 0)
    return;
  size_t out = 0;
  int col = 0;
  for (const char *p = src; *p && out + 2 < dst_sz;
       p++) {
    char c = *p;
    if (c == '\n') {
      dst[out++] = '\n';
      col = 0;
      continue;
    }
    if (col >= width) {
      dst[out++] = '\n';
      col = 0;
    }
    if (c == ' ' && col == 0)
      continue;
    dst[out++] = c;
    col++;
  }
  dst[out] = '\0';
}

static void FormatToastError(const char *err,
                             char *out,
                             size_t out_sz) {
  if (!out || out_sz == 0)
    return;
  out[0] = '\0';
  if (!err || err[0] == '\0') {
    snprintf(out, out_sz, "AI failed");
    return;
  }
  char clean[96];
  size_t j = 0;
  for (size_t i = 0; err[i] && j + 1 < sizeof(clean); i++) {
    unsigned char c = (unsigned char)err[i];
    if (c < 32 || c == 127)
      c = ' ';
    clean[j++] = (char)c;
  }
  clean[j] = '\0';
  snprintf(out, out_sz, "AI failed: %s", clean);
}

static bool PanelButton(GuiState *gui, Rectangle r,
                        const char *label, Theme t) {
  Vector2 mouse = GetMousePosition();
  bool hover = CheckCollisionPointRec(mouse, r);
  Color bg = hover ? t.hover : t.surface;
  DrawRectangleRounded(r, 0.25f, 6, bg);
  DrawRectangleRoundedLinesEx(r, 0.25f, 6,
                              1.0f, t.border);
  Vector2 size = MeasureTextEx(gui->uiFont, label,
                               12, 1.0f);
  float tx = r.x + (r.width - size.x) * 0.5f;
  float ty = r.y + (r.height - size.y) * 0.5f;
  DrawTextEx(gui->uiFont, label,
             (Vector2){tx, ty}, 12, 1.0f, t.text);
  if (!hover)
    return false;
  return IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

void AiPanelInit(GuiState *gui) {
  if (!gui)
    return;
  gui->showAiPanel = false;
  gui->aiBusy = false;
  gui->aiText[0] = '\0';
  gui->aiError[0] = '\0';
  gui->aiRect = (Rectangle){0, 0, 0, 0};
}

void AiPanelRequest(GuiState *gui, const Canvas *canvas,
                    bool showPanel) {
  if (!gui || !canvas)
    return;
  if (gui->aiBusy) {
    GuiToastQueue(gui, "AI already running");
    return;
  }
  double now = GetTime();
  if (now < gui->aiCooldownUntil) {
    int wait = (int)ceil(gui->aiCooldownUntil - now);
    char msg[64];
    snprintf(msg, sizeof(msg), "AI cooldown %ds", wait);
    GuiToastQueue(gui, msg);
    return;
  }

  AiSettings settings;
  char err[96] = {0};
  (void)AiSettingsLoad(&settings, err, sizeof(err));
  if (!AiSettingsReady(&settings, err,
                       sizeof(err))) {
    AiSettingsUiOpen(gui);
    snprintf(gui->aiStatus, sizeof(gui->aiStatus),
             "AI settings needed");
    GuiToastSet(gui, "AI settings needed");
    return;
  }

  gui->aiBusy = true;
  gui->aiCooldownUntil = now + 5.0;
  gui->aiError[0] = '\0';
  snprintf(gui->aiText, sizeof(gui->aiText),
           "Waiting for response...");
  if (showPanel)
    gui->showAiPanel = true;

  GuiToastSet(gui, "AI request sent. Waiting...");

  char raw[2048];
  raw[0] = '\0';
  if (!AiAnalyzeCanvas(canvas, &settings, raw,
                       sizeof(raw), err,
                       sizeof(err))) {
    char msg[196];
    snprintf(msg, sizeof(msg), "AI error: %s", err);
    WrapText(msg, gui->aiText,
             sizeof(gui->aiText), 60);
    snprintf(gui->aiError, sizeof(gui->aiError), "%s", err);
    char toast[128];
    FormatToastError(err, toast, sizeof(toast));
    GuiToastQueue(gui, toast);
    if (showPanel)
      gui->showAiPanel = true;
    gui->aiBusy = false;
    return;
  }

  WrapText(raw, gui->aiText,
           sizeof(gui->aiText), 60);
  GuiToastQueue(gui, "AI analysis ready");
  if (showPanel)
    gui->showAiPanel = true;
  gui->aiBusy = false;
}

void AiPanelDraw(GuiState *gui, const Canvas *canvas,
                 Theme t, int sw, int sh) {
  if (!gui || !gui->showAiPanel)
    return;

  const float topH = 88.0f;
  const float footerH = 24.0f;
  const float margin = 24.0f;
  float maxW = (float)sw - margin * 2.0f;
  if (maxW < 200.0f)
    maxW = (float)sw - 8.0f;
  float panelW = fminf(460.0f, maxW);
  float panelH = (float)sh - topH - footerH -
                 margin * 2.0f;
  if (panelH < 180.0f)
    panelH = 180.0f;
  if (panelH > (float)sh - 8.0f)
    panelH = (float)sh - 8.0f;

  float panelX = (float)sw - panelW - margin;
  float panelY = topH + margin;
  Rectangle panel = {panelX, panelY, panelW, panelH};
  gui->aiRect = panel;

  DrawRectangleRounded(panel, 0.06f, 10,
                       ColorAlpha(t.surface, 0.98f));
  DrawRectangleRoundedLinesEx(panel, 0.06f, 10,
                              1.0f, t.border);

  float x = panel.x + 18.0f;
  float y = panel.y + 16.0f;
  DrawTextEx(gui->uiFont, "AI Analysis",
             (Vector2){x, y}, 18, 1.0f, t.text);
  if (gui->aiBusy) {
    DrawTextEx(gui->uiFont, "Waiting for response...",
               (Vector2){x, y + 18.0f}, 12, 1.0f, t.textDim);
  }

  Rectangle closeBtn = {panel.x + panel.width - 70,
                        panel.y + 12, 52, 24};
  if (PanelButton(gui, closeBtn, "Close", t))
    gui->showAiPanel = false;

  Rectangle runBtn = {panel.x + panel.width - 140,
                      panel.y + 12, 60, 24};
  if (PanelButton(gui, runBtn,
                  gui->aiBusy ? "..." : "Run", t))
    AiPanelRequest(gui, canvas, true);

  y += 32.0f;
  DrawLineEx((Vector2){panel.x + 12.0f, y},
             (Vector2){panel.x + panel.width - 12.0f,
                       y},
             1.0f, t.border);
  y += 16.0f;

  Color text = gui->darkMode ? t.text : t.textDim;
  DrawTextEx(gui->uiFont, gui->aiText,
             (Vector2){x, y}, 13, 1.0f, text);
}

#include "gui_internal.h"
#include <math.h>
#include <stdio.h>

static float NiceStep(float raw) {
  if (raw <= 0.0f || !isfinite(raw))
    return 1.0f;
  float expv = floorf(log10f(raw));
  float base = powf(10.0f, expv);
  float frac = raw / base;
  float niceFrac = 1.0f;
  if (frac < 1.5f)
    niceFrac = 1.0f;
  else if (frac < 3.0f)
    niceFrac = 2.0f;
  else if (frac < 7.0f)
    niceFrac = 5.0f;
  else
    niceFrac = 10.0f;
  return niceFrac * base;
}

static int SubdivForMajor(float majorStep) {
  if (majorStep <= 0.0f || !isfinite(majorStep))
    return 5;
  float expv = floorf(log10f(majorStep));
  float base = powf(10.0f, expv);
  float frac = majorStep / base;
  if (frac < 1.5f)
    return 5; // 1 -> 0.2
  if (frac < 3.5f)
    return 4; // 2 -> 0.5
  return 5;   // 5 -> 1
}

static int DecimalsForStep(float step) {
  if (step <= 0.0f || !isfinite(step))
    return 0;
  if (step >= 1.0f)
    return 0;
  int d = (int)ceilf(-log10f(step));
  if (d < 0)
    d = 0;
  if (d > 6)
    d = 6;
  return d;
}

static void FormatLabel(char *buf, size_t bufSize, float v, float step) {
  int decimals = DecimalsForStep(step);
  if (fabsf(v) < step * 0.001f)
    v = 0.0f;
  if (decimals == 0) {
    long long iv = llroundf(v);
    snprintf(buf, bufSize, "%lld", iv);
  } else {
    snprintf(buf, bufSize, "%.*f", decimals, v);
  }
}

void GuiDrawRulerLeft(GuiState *gui, const Canvas *canvas, Theme t, int sw,
                      int sh) {
  (void)sw;
  if (!gui->showRulers)
    return;

  const float topH = 88.0f;
  const float footerH = 24.0f;
  const float rulerSize = 24.0f;

  float h = (float)sh - footerH - (topH + rulerSize);
  if (h <= 0.0f)
    return;

  Rectangle r = {0, topH + rulerSize, rulerSize, h};
  gui->rulerLeftRect = r;

  DrawRectangleRec(r, t.surface);
  DrawLineEx((Vector2){rulerSize, r.y}, (Vector2){rulerSize, r.y + r.height}, 1,
             t.border);

  float zoom = canvas->camera.zoom;
  if (zoom <= 0.00001f)
    zoom = 0.00001f;

  float topWorldY =
      (r.y - canvas->camera.offset.y) / zoom + canvas->camera.target.y;
  float bottomWorldY =
      ((r.y + r.height) - canvas->camera.offset.y) / zoom + canvas->camera.target.y;

  if (topWorldY > bottomWorldY) {
    float tmp = topWorldY;
    topWorldY = bottomWorldY;
    bottomWorldY = tmp;
  }

  float desiredPx = 96.0f;
  float majorStep = NiceStep(desiredPx / zoom);
  int subdiv = SubdivForMajor(majorStep);
  float minorStep = majorStep / (float)subdiv;
  if (minorStep <= 0.0f)
    return;

  int startIndex = (int)floorf(topWorldY / minorStep) - 1;
  int endIndex = (int)ceilf(bottomWorldY / minorStep) + 1;

  float xr = r.x + r.width;
  Font font = GetFontDefault();
  float fontSize = 10.0f;

  for (int idx = startIndex; idx <= endIndex; idx++) {
    float v = (float)idx * minorStep;
    float sy = (v - canvas->camera.target.y) * zoom + canvas->camera.offset.y;
    if (sy < r.y - 2.0f || sy > r.y + r.height + 2.0f)
      continue;

    int mod = idx % subdiv;
    if (mod < 0)
      mod += subdiv;
    bool isMajor = (mod == 0);
    bool isMid = (!isMajor && (subdiv % 2 == 0) && (mod == subdiv / 2));

    float len = isMajor ? (r.width * 0.80f)
                        : (isMid ? (r.width * 0.55f) : (r.width * 0.35f));
    Color c = isMajor ? t.textDim : ColorAlpha(t.textDim, 0.65f);

    DrawLineEx((Vector2){xr, sy}, (Vector2){xr - len, sy}, 1, c);

    if (isMajor) {
      char label[32];
      FormatLabel(label, sizeof(label), v, majorStep);
      Vector2 size = MeasureTextEx(font, label, fontSize, 1.0f);
      Vector2 pos = {r.x + r.width * 0.5f, sy};
      Vector2 origin = {size.x * 0.5f, size.y * 0.5f};
      DrawTextPro(font, label, pos, origin, -90.0f, fontSize, 1.0f, t.textDim);
    }
  }

  Vector2 m = GetMousePosition();
  float my = m.y;
  if (my < r.y)
    my = r.y;
  if (my > r.y + r.height)
    my = r.y + r.height;

  float triW = 10.0f;
  float triH = 6.0f;
  DrawTriangle((Vector2){xr, my},
               (Vector2){xr - triH, my - triW * 0.5f},
               (Vector2){xr - triH, my + triW * 0.5f}, t.primary);
  DrawLineEx((Vector2){r.x, my}, (Vector2){xr, my}, 1, ColorAlpha(t.primary, 0.25f));
}


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

void GuiDrawRulerTop(GuiState *gui, const Canvas *canvas, Theme t, int sw,
                     int sh) {
  (void)sh;
  if (!gui->showRulers)
    return;

  const float topH = 88.0f;
  const float rulerSize = 24.0f;

  float w = (float)sw - rulerSize;
  if (w <= 0.0f)
    return;

  Rectangle corner = {0, topH, rulerSize, rulerSize};
  Rectangle r = {rulerSize, topH, w, rulerSize};
  gui->rulerTopRect = r;

  DrawRectangleRec(corner, t.surface);
  DrawRectangleRec(r, t.surface);
  DrawLineEx((Vector2){0, topH + rulerSize}, (Vector2){(float)sw, topH + rulerSize},
             1, t.border);
  DrawLineEx((Vector2){rulerSize, topH}, (Vector2){rulerSize, topH + rulerSize},
             1, t.border);

  float zoom = canvas->camera.zoom;
  if (zoom <= 0.00001f)
    zoom = 0.00001f;

  float leftWorldX =
      (r.x - canvas->camera.offset.x) / zoom + canvas->camera.target.x;
  float rightWorldX =
      ((r.x + r.width) - canvas->camera.offset.x) / zoom + canvas->camera.target.x;

  if (leftWorldX > rightWorldX) {
    float tmp = leftWorldX;
    leftWorldX = rightWorldX;
    rightWorldX = tmp;
  }

  float desiredPx = 96.0f;
  float majorStep = NiceStep(desiredPx / zoom);
  int subdiv = SubdivForMajor(majorStep);
  float minorStep = majorStep / (float)subdiv;
  if (minorStep <= 0.0f)
    return;

  int startIndex = (int)floorf(leftWorldX / minorStep) - 1;
  int endIndex = (int)ceilf(rightWorldX / minorStep) + 1;

  for (int idx = startIndex; idx <= endIndex; idx++) {
    float v = (float)idx * minorStep;
    float sx = (v - canvas->camera.target.x) * zoom + canvas->camera.offset.x;
    if (sx < r.x - 2.0f || sx > r.x + r.width + 2.0f)
      continue;

    int mod = idx % subdiv;
    if (mod < 0)
      mod += subdiv;
    bool isMajor = (mod == 0);
    bool isMid = (!isMajor && (subdiv % 2 == 0) && (mod == subdiv / 2));

    float len = isMajor ? (r.height * 0.80f)
                        : (isMid ? (r.height * 0.55f) : (r.height * 0.35f));
    Color c = isMajor ? t.textDim : ColorAlpha(t.textDim, 0.65f);

    float yb = r.y + r.height;
    DrawLineEx((Vector2){sx, yb}, (Vector2){sx, yb - len}, 1, c);

    if (isMajor) {
      char label[32];
      FormatLabel(label, sizeof(label), v, majorStep);

      int fontSize = 10;
      float tw = MeasureTextEx(gui->uiFont, label, (float)fontSize, 1.0f).x;
      float tx = sx - tw / 2.0f;
      float pad = 2.0f;
      if (tx < r.x + pad)
        tx = r.x + pad;
      if (tx + tw > r.x + r.width - pad)
        continue;
      DrawTextEx(gui->uiFont, label, (Vector2){tx, r.y + 3.0f}, (float)fontSize,
                 1.0f, t.textDim);
    }
  }

  Vector2 m = GetMousePosition();
  float mx = m.x;
  if (mx < r.x)
    mx = r.x;
  if (mx > r.x + r.width)
    mx = r.x + r.width;

  float yb = r.y + r.height;
  float triW = 10.0f;
  float triH = 6.0f;
  DrawTriangle((Vector2){mx, yb},
               (Vector2){mx + triW * 0.5f, yb - triH},
               (Vector2){mx - triW * 0.5f, yb - triH}, t.primary);
  DrawLineEx((Vector2){mx, r.y}, (Vector2){mx, yb}, 1, ColorAlpha(t.primary, 0.25f));
}

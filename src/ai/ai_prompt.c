#include "ai_prompt.h"
#include <stdarg.h>
#include <stdio.h>

static void Append(char *out, size_t out_sz,
                   size_t *off, const char *fmt, ...) {
  if (!out || out_sz == 0 || !off)
    return;
  if (*off >= out_sz)
    return;
  va_list args;
  va_start(args, fmt);
  int n = vsnprintf(out + *off,
                    out_sz - *off,
                    fmt, args);
  va_end(args);
  if (n > 0)
    *off += (size_t)n;
  if (*off >= out_sz)
    out[out_sz - 1] = '\0';
}

static void StrokeBounds(const Stroke *s,
                         float *min_x, float *min_y,
                         float *max_x, float *max_y) {
  *min_x = 0.0f;
  *min_y = 0.0f;
  *max_x = 0.0f;
  *max_y = 0.0f;
  if (!s || s->pointCount <= 0)
    return;
  *min_x = s->points[0].x;
  *min_y = s->points[0].y;
  *max_x = s->points[0].x;
  *max_y = s->points[0].y;
  for (int i = 1; i < s->pointCount; i++) {
    float x = s->points[i].x;
    float y = s->points[i].y;
    if (x < *min_x)
      *min_x = x;
    if (y < *min_y)
      *min_y = y;
    if (x > *max_x)
      *max_x = x;
    if (y > *max_y)
      *max_y = y;
  }
}

void AiBuildPrompt(const Canvas *c, char *out,
                   size_t out_sz) {
  size_t off = 0;
  int total = c ? c->strokeCount : 0;
  Append(out, out_sz, &off,
         "Analyze this drawing.\n");
  Append(out, out_sz, &off,
         "Return concise bullets.\n");
  Append(out, out_sz, &off,
         "Summarize shapes, text, and intent.\n\n");
  Append(out, out_sz, &off,
         "Strokes: %d\n", total);
  if (!c)
    return;
  int limit = total > 40 ? 40 : total;
  for (int i = 0; i < limit; i++) {
    const Stroke *s = &c->strokes[i];
    float min_x, min_y, max_x, max_y;
    StrokeBounds(s, &min_x, &min_y, &max_x, &max_y);
    Append(out, out_sz, &off,
           "[%d] pts=%d thick=%.1f ",
           i + 1, s->pointCount, s->thickness);
    Append(out, out_sz, &off,
           "color=#%02X%02X%02X ",
           s->color.r, s->color.g, s->color.b);
    Append(out, out_sz, &off,
           "bbox=%.1f,%.1f %.1f,%.1f\n",
           min_x, min_y,
           max_x - min_x, max_y - min_y);
  }
  if (total > limit)
    Append(out, out_sz, &off, "(truncated)\n");
}

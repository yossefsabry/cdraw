#include "canvas.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool SaveCanvasToFile(const Canvas *canvas, const char *path) {
  FILE *f = fopen(path, "wb");
  if (!f)
    return false;
  fprintf(f, "CDRAW1\n");
  fprintf(f, "strokes %d\n", canvas->strokeCount);
  for (int i = 0; i < canvas->strokeCount; i++) {
    const Stroke *s = &canvas->strokes[i];
    fprintf(f, "stroke %u %u %u %u %.3f %d\n", s->color.r, s->color.g, s->color.b,
            s->color.a, s->thickness, s->pointCount);
    for (int p = 0; p < s->pointCount; p++)
      fprintf(f, "%.6f %.6f\n", s->points[p].x, s->points[p].y);
  }
  fclose(f);
  return true;
}

bool LoadCanvasFromFile(Canvas *canvas, const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return false;

  char header[32] = {0};
  if (!fgets(header, (int)sizeof(header), f) || strncmp(header, "CDRAW1", 5) != 0) {
    fclose(f);
    return false;
  }

  ClearCanvas(canvas);

  char tok[32] = {0};
  int strokeCount = 0;
  if (fscanf(f, "%31s %d", tok, &strokeCount) != 2 || strcmp(tok, "strokes") != 0 ||
      strokeCount < 0) {
    fclose(f);
    return false;
  }

  for (int i = 0; i < strokeCount; i++) {
    unsigned int r, g, b, a;
    float thickness;
    int pointCount;
    if (fscanf(f, "%31s %u %u %u %u %f %d", tok, &r, &g, &b, &a, &thickness,
               &pointCount) != 7 ||
        strcmp(tok, "stroke") != 0 || pointCount < 0) {
      fclose(f);
      ClearCanvas(canvas);
      return false;
    }

    Stroke s = {0};
    s.color = (Color){(unsigned char)r, (unsigned char)g, (unsigned char)b,
                      (unsigned char)a};
    s.thickness = thickness;
    s.pointCount = pointCount;
    s.capacity = pointCount;
    if (pointCount > 0) {
      s.points = (Point *)malloc(sizeof(Point) * (size_t)pointCount);
      if (!s.points) {
        fclose(f);
        ClearCanvas(canvas);
        return false;
      }
      for (int p = 0; p < pointCount; p++) {
        float x, y;
        if (fscanf(f, "%f %f", &x, &y) != 2) {
          free(s.points);
          fclose(f);
          ClearCanvas(canvas);
          return false;
        }
        s.points[p] = (Point){x, y};
      }
    }
    AddStroke(canvas, s);
  }

  fclose(f);
  return true;
}


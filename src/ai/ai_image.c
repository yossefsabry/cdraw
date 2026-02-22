#include "ai_image.h"
#include <stdio.h>
#include <stdlib.h>
#if !defined(_WIN32)
#include <unistd.h>
#endif

static void TempPath(char *out, size_t out_sz) {
#if defined(_WIN32)
  snprintf(out, out_sz, "cdraw_ai.png");
#else
  long pid = (long)getpid();
  snprintf(out, out_sz, "/tmp/cdraw_ai_%ld.png", pid);
#endif
}

static bool ReadFile(const char *path,
                     unsigned char **out,
                     size_t *out_sz,
                     char *err,
                     size_t err_sz) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return false;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (len <= 0) {
    fclose(f);
    return false;
  }
  unsigned char *buf =
      (unsigned char *)malloc((size_t)len);
  if (!buf) {
    fclose(f);
    return false;
  }
  size_t got = fread(buf, 1, (size_t)len, f);
  fclose(f);
  if (got != (size_t)len) {
    free(buf);
    if (err && err_sz > 0)
      snprintf(err, err_sz, "read failed");
    return false;
  }
  *out = buf;
  *out_sz = (size_t)len;
  return true;
}

bool AiCapturePng(const Canvas *c,
                  unsigned char **out,
                  size_t *out_sz,
                  char *err,
                  size_t err_sz) {
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();
  if (sw <= 0 || sh <= 0)
    return false;
  RenderTexture2D target =
      LoadRenderTexture(sw, sh);
  if (target.texture.id == 0)
    return false;
  Canvas temp = *c;
  temp.selectedStrokeIndex = -1;
  temp.isDrawing = false;
  BeginTextureMode(target);
  DrawCanvas(&temp);
  EndTextureMode();
  Image img = LoadImageFromTexture(target.texture);
  ImageFlipVertical(&img);
  UnloadRenderTexture(target);
  char path[256];
  TempPath(path, sizeof(path));
  bool ok = ExportImage(img, path);
  UnloadImage(img);
  if (!ok) {
    if (err && err_sz > 0)
      snprintf(err, err_sz, "png export failed");
    return false;
  }
  ok = ReadFile(path, out, out_sz, err, err_sz);
  remove(path);
  return ok;
}

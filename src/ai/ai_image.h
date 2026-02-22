#ifndef AI_IMAGE_H
#define AI_IMAGE_H

#include "../canvas.h"
#include <stdbool.h>
#include <stddef.h>

bool AiCapturePng(const Canvas *canvas,
                  unsigned char **out,
                  size_t *out_sz,
                  char *err,
                  size_t err_sz);

#endif

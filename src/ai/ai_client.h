#ifndef AI_CLIENT_H
#define AI_CLIENT_H

#include "ai_settings.h"
#include "../canvas.h"
#include <stdbool.h>
#include <stddef.h>

bool AiAnalyzeCanvas(const Canvas *canvas,
                     const AiSettings *settings,
                     char *out,
                     size_t out_sz,
                     char *err,
                     size_t err_sz);

#endif

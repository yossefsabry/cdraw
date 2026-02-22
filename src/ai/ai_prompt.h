#ifndef AI_PROMPT_H
#define AI_PROMPT_H

#include "../canvas.h"
#include <stddef.h>

void AiBuildPrompt(const Canvas *canvas,
                   char *out,
                   size_t out_sz);

#endif

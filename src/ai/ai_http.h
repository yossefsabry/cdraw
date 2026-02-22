#ifndef AI_HTTP_H
#define AI_HTTP_H

#include <stdbool.h>
#include <stddef.h>

bool AiHttpPostJson(const char *url,
                    const char *body,
                    const char *auth,
                    char *out,
                    size_t out_sz,
                    char *err,
                    size_t err_sz);

#endif

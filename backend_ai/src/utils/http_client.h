#ifndef BACKEND_HTTP_CLIENT_H
#define BACKEND_HTTP_CLIENT_H

#include <stdbool.h>
#include <stddef.h>

bool HttpPostJson(const char *url,
                  const char *body,
                  const char *auth,
                  char **out,
                  char *err,
                  size_t err_sz);

#endif

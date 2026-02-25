#ifndef APP_PATHS_H
#define APP_PATHS_H

#include <stdbool.h>
#include <stddef.h>

bool CdrawResolveRoot(char *out, size_t size);
const char *CdrawAssetPath(char *out, size_t size, const char *relative);
bool CdrawBackendPath(char *out, size_t size);

#endif

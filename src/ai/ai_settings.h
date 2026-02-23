#ifndef AI_SETTINGS_H
#define AI_SETTINGS_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
  AI_PROVIDER_GEMINI = 0,
  AI_PROVIDER_LOCAL,
  AI_PROVIDER_OPENAI
} AiProvider;

typedef struct {
  AiProvider provider;
  char model[64];
  char api_key[128];
  char base_url[128];
} AiSettings;

bool AiSettingsLoad(AiSettings *out,
                    char *err,
                    size_t err_sz);
bool AiSettingsSave(const AiSettings *settings,
                    char *err,
                    size_t err_sz);
bool AiSettingsClear(char *err, size_t err_sz);
bool AiSettingsReady(const AiSettings *settings,
                     char *err,
                     size_t err_sz);
bool AiSettingsPath(char *out, size_t out_sz);
const char *AiProviderLabel(AiProvider provider);

#endif

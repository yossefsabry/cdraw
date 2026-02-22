#ifndef AI_SETTINGS_UI_H
#define AI_SETTINGS_UI_H

#include "../gui_internal.h"

void AiSettingsUiOpen(GuiState *gui);
void AiSettingsUiDraw(GuiState *gui,
                      Theme t,
                      int sw,
                      int sh);
void AiSettingsUiUpdate(GuiState *gui,
                        int sw,
                        int sh);
void AiSettingsUiClear(GuiState *gui);

#endif

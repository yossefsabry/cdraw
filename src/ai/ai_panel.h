#ifndef AI_PANEL_H
#define AI_PANEL_H

#include "../gui_internal.h"

void AiPanelInit(GuiState *gui);
void AiPanelDraw(GuiState *gui,
                 const Canvas *canvas,
                 Theme t,
                 int sw,
                 int sh);
void AiPanelRequest(GuiState *gui,
                    const Canvas *canvas);

#endif

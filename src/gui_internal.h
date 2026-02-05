#ifndef GUI_INTERNAL_H
#define GUI_INTERNAL_H

#include "gui.h"

typedef struct {
  Color background;
  Color surface;
  Color tab;
  Color border;
  Color text;
  Color textDim;
  Color primary;
  Color hover;
  Color canvas;
  Color grid;
  Color groupBg;
} Theme;

Theme GuiThemeGet(bool darkMode);

void GuiToastSet(GuiState *gui, const char *msg);

void GuiTooltipReset(GuiState *gui);
void GuiTooltipSet(GuiState *gui, const char *text, Rectangle anchor);
void GuiDrawTooltip(GuiState *gui, Theme t);

void GuiFontLoad(GuiState *gui);
void GuiFontUnload(GuiState *gui);

void GuiIconsLoad(GuiIcons *icons);
void GuiIconsUnload(GuiIcons *icons);

void GuiDrawIconTexture(const GuiIcons *icons, GuiIcon icon, Rectangle bounds,
                        Color tint);
bool GuiIconButton(GuiState *gui, const GuiIcons *icons, Rectangle bounds,
                   GuiIcon icon, bool active, Color bgActive, Color bgHover,
                   Color iconActive, Color iconIdle, Color iconHover,
                   const char *tooltip);

void GuiDrawHeader(GuiState *gui, Canvas *canvas, Theme t, Color iconIdle,
                   Color iconHover, int sw);
void GuiDrawToolbar(GuiState *gui, Canvas *canvas, Theme t, Color iconIdle,
                    Color iconHover, int sw, int sh, float paletteY);
void GuiDrawRulerTop(GuiState *gui, const Canvas *canvas, Theme t, int sw, int sh);
void GuiDrawRulerLeft(GuiState *gui, const Canvas *canvas, Theme t, int sw,
                      int sh);
void GuiDrawWelcome(GuiState *gui, Canvas *canvas, Theme t, int sw, int sh);
void GuiDrawMenu(GuiState *gui, Canvas *canvas, Theme t);
void GuiDrawPalette(GuiState *gui, Canvas *canvas, Theme t, Color iconIdle,
                    Color iconHover, int sw, int sh, float paletteX,
                    float paletteY, float paletteW, float paletteH);
void GuiDrawColorPicker(GuiState *gui, Theme t);
void GuiDrawFooter(GuiState *gui, Canvas *canvas, Theme t, int sw, int sh);

void GuiMarkNewDocument(GuiState *gui);
void GuiRequestOpen(GuiState *gui);
void GuiRequestSave(GuiState *gui, Document *doc);

void GuiDocumentsInit(GuiState *gui, int screenWidth, int screenHeight,
                      bool showGrid);
void GuiDocumentsFree(GuiState *gui);
Document *GuiGetActiveDocument(GuiState *gui);
Canvas *GuiGetActiveCanvas(GuiState *gui);
Document *GuiAddDocument(GuiState *gui, int screenWidth, int screenHeight,
                         bool showGrid, bool makeActive);
void GuiCloseDocument(GuiState *gui, int index, int screenWidth, int screenHeight,
                      bool showGrid);

#endif

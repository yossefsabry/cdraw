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

void GuiIconsLoad(GuiIcons *icons);
void GuiIconsUnload(GuiIcons *icons);

void GuiDrawIconTexture(Texture2D tex, Rectangle bounds, Color tint);
bool GuiIconButton(Rectangle bounds, Texture2D icon, bool active, Color bgActive,
                   Color bgHover, Color iconActive, Color iconIdle,
                   Color iconHover);

void GuiDrawHeader(GuiState *gui, Canvas *canvas, Theme t, Color iconIdle,
                   Color iconHover, int sw);
void GuiDrawToolbar(GuiState *gui, Canvas *canvas, Theme t, Color iconIdle,
                    Color iconHover, int sw, int sh, float paletteY);
void GuiDrawMenu(GuiState *gui, Canvas *canvas, Theme t);
void GuiDrawPalette(GuiState *gui, Canvas *canvas, Theme t, Color iconIdle,
                    Color iconHover, int sw, int sh, float paletteX,
                    float paletteY, float paletteW, float paletteH);
void GuiDrawColorPicker(GuiState *gui, Theme t);
void GuiDrawFooter(GuiState *gui, Canvas *canvas, Theme t, int sw, int sh);

#endif

#include "gui_internal.h"

static Theme ThemeLight(void) {
  return (Theme){
      .background = (Color){243, 244, 246, 255},
      .surface = (Color){255, 255, 255, 255},
      .tab = (Color){243, 244, 246, 255},
      .border = (Color){229, 231, 235, 255},
      .text = (Color){31, 41, 55, 255},
      .textDim = (Color){107, 114, 128, 255},
      .primary = (Color){56, 189, 248, 255},
      .hover = (Color){0, 0, 0, 18},
      .canvas = (Color){255, 255, 255, 255},
      .grid = (Color){229, 231, 235, 255},
      .groupBg = (Color){243, 244, 246, 255},
  };
}

static Theme ThemeDark(void) {
  return (Theme){
      .background = (Color){24, 24, 27, 255},
      .surface = (Color){39, 39, 42, 255},
      .tab = (Color){0, 0, 0, 60},
      .border = (Color){55, 65, 81, 255},
      .text = (Color){243, 244, 246, 255},
      .textDim = (Color){156, 163, 175, 255},
      .primary = (Color){56, 189, 248, 255},
      .hover = (Color){75, 85, 99, 110},
      .canvas = (Color){18, 18, 18, 255},
      .grid = (Color){42, 42, 42, 255},
      .groupBg = (Color){0, 0, 0, 50},
  };
}

Theme GuiThemeGet(bool darkMode) { return darkMode ? ThemeDark() : ThemeLight(); }


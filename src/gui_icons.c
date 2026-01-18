#include "gui_internal.h"
#include <math.h>
#include <stdio.h>

static Texture2D LoadIconPng(const char *basename) {
  char path[256];
  snprintf(path, sizeof(path), "assets/ui_icons/%s.png", basename);
  if (!FileExists(path))
    return (Texture2D){0};
  return LoadTexture(path);
}

static void UnloadIcon(Texture2D *tex) {
  if (tex->id != 0)
    UnloadTexture(*tex);
  *tex = (Texture2D){0};
}

void GuiIconsLoad(GuiIcons *icons) {
  *icons = (GuiIcons){0};

#define LOAD(field, name) icons->field = LoadIconPng(name)
  LOAD(menu, "menu");
  LOAD(add, "add");
  LOAD(openFile, "load_file");
  LOAD(saveFile, "save");
  LOAD(undo, "chevron_left");
  LOAD(redo, "arrow_forward");
  LOAD(pen, "draw_pen");
  LOAD(rect, "check_box_outline");
  LOAD(circle, "circle");
  LOAD(line, "line");
  LOAD(eraser, "ink_eraser");
  LOAD(select, "arrow_selector");
  LOAD(pan, "move_");
  LOAD(grid, "grid");
  LOAD(fullscreen, "fullscreen");
  LOAD(colorPicker, "color_gradent_choose");
  LOAD(darkMode, "dark_mode");
  LOAD(lightMode, "light_mode");
  LOAD(windowMinimize, "minmaz_app");
  LOAD(windowToggleSize, "toggle_window_size");
  LOAD(windowClose, "close_app");
#undef LOAD
}

void GuiIconsUnload(GuiIcons *icons) {
#define UNLOAD(field) UnloadIcon(&icons->field)
  UNLOAD(menu);
  UNLOAD(add);
  UNLOAD(openFile);
  UNLOAD(saveFile);
  UNLOAD(undo);
  UNLOAD(redo);
  UNLOAD(pen);
  UNLOAD(rect);
  UNLOAD(circle);
  UNLOAD(line);
  UNLOAD(eraser);
  UNLOAD(select);
  UNLOAD(pan);
  UNLOAD(grid);
  UNLOAD(fullscreen);
  UNLOAD(colorPicker);
  UNLOAD(darkMode);
  UNLOAD(lightMode);
  UNLOAD(windowMinimize);
  UNLOAD(windowToggleSize);
  UNLOAD(windowClose);
#undef UNLOAD
  *icons = (GuiIcons){0};
}

void GuiDrawIconTexture(Texture2D tex, Rectangle bounds, Color tint) {
  if (tex.id == 0)
    return;
  float scale = fminf(bounds.width / (float)tex.width,
                      bounds.height / (float)tex.height);
  scale = fminf(scale, 1.0f);
  float w = (float)tex.width * scale;
  float h = (float)tex.height * scale;
  Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
  Rectangle dst = {bounds.x + (bounds.width - w) / 2.0f,
                   bounds.y + (bounds.height - h) / 2.0f, w, h};
  DrawTexturePro(tex, src, dst, (Vector2){0, 0}, 0.0f, tint);
}

bool GuiIconButton(Rectangle bounds, Texture2D icon, bool active, Color bgActive,
                   Color bgHover, Color iconActive, Color iconIdle,
                   Color iconHover) {
  Vector2 mouse = GetMousePosition();
  bool hover = CheckCollisionPointRec(mouse, bounds);

  if (active)
    DrawRectangleRounded(bounds, 0.25f, 6, bgActive);
  else if (hover)
    DrawRectangleRounded(bounds, 0.25f, 6, bgHover);

  Color tint = active ? iconActive : (hover ? iconHover : iconIdle);
  if (icon.id != 0)
    GuiDrawIconTexture(icon, bounds, tint);
  else
    DrawRectangleLinesEx(bounds, 1.0f, tint);

  return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

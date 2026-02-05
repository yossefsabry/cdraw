#include "gui_internal.h"
#include <math.h>
#include <stdio.h>

typedef struct {
  const char *name;
  GuiIcon *icon;
} IconEntry;

static const int ICON_SIZE = 24;
static const int ICON_PADDING = 2;
static const int ICON_COLS = 8;

static int IconCellSize(void) { return ICON_SIZE + ICON_PADDING * 2; }

static void SetIconSrc(GuiIcon *icon, int index) {
  int cell = IconCellSize();
  int cx = (index % ICON_COLS) * cell;
  int cy = (index / ICON_COLS) * cell;
  icon->src = (Rectangle){(float)(cx + ICON_PADDING), (float)(cy + ICON_PADDING),
                          (float)ICON_SIZE, (float)ICON_SIZE};
}

static void IconPath(char *buf, size_t bufSize, const char *basename) {
  snprintf(buf, bufSize, "assets/ui_icons/%s.png", basename);
}

static bool AtlasIsStale(const char *atlasPath, const IconEntry entries[],
                         int count) {
  if (!FileExists(atlasPath))
    return true;

  long atlasTime = GetFileModTime(atlasPath);
  if (atlasTime <= 0)
    return true;

  char path[256];
  for (int i = 0; i < count; i++) {
    IconPath(path, sizeof(path), entries[i].name);
    if (!FileExists(path))
      continue;
    long iconTime = GetFileModTime(path);
    if (iconTime > atlasTime)
      return true;
  }
  return false;
}

static Texture2D BuildAtlas(const char *atlasPath, const IconEntry entries[],
                            int count) {
  int cell = IconCellSize();
  int rows = (count + ICON_COLS - 1) / ICON_COLS;
  Image atlas =
      GenImageColor(ICON_COLS * cell, rows * cell, (Color){0, 0, 0, 0});

  for (int i = 0; i < count; i++) {
    char path[256];
    IconPath(path, sizeof(path), entries[i].name);
    if (!FileExists(path))
      continue;

    Image icon = LoadImage(path);
    if (icon.data == NULL)
      continue;
    if (icon.width != ICON_SIZE || icon.height != ICON_SIZE)
      ImageResize(&icon, ICON_SIZE, ICON_SIZE);

    Rectangle src = {0, 0, (float)icon.width, (float)icon.height};
    int cx = (i % ICON_COLS) * cell;
    int cy = (i / ICON_COLS) * cell;
    Rectangle dst = {(float)(cx + ICON_PADDING), (float)(cy + ICON_PADDING),
                     (float)ICON_SIZE, (float)ICON_SIZE};
    ImageDraw(&atlas, icon, src, dst, WHITE);
    UnloadImage(icon);
  }

  // Best-effort on-disk cache for faster startup on subsequent runs.
  (void)ExportImage(atlas, atlasPath);

  Texture2D tex = LoadTextureFromImage(atlas);
  UnloadImage(atlas);
  return tex;
}

void GuiIconsLoad(GuiIcons *icons) {
  *icons = (GuiIcons){0};

  IconEntry entries[] = {
      {"menu", &icons->menu},
      {"add", &icons->add},
      {"load_file", &icons->openFile},
      {"save", &icons->saveFile},
      {"chevron_left", &icons->undo},
      {"arrow_forward", &icons->redo},

      {"draw_pen", &icons->pen},
      {"check_box_outline", &icons->rect},
      {"circle", &icons->circle},
      {"arrow_drawing", &icons->line},
      {"ink_eraser", &icons->eraser},
      {"arrow_selector", &icons->select},
      {"move_", &icons->pan},

      {"grid", &icons->grid},
      {"fullscreen", &icons->fullscreen},
      {"color_gradent_choose", &icons->colorPicker},

      {"dark_mode", &icons->darkMode},
      {"light_mode", &icons->lightMode},

      {"minmaz_app", &icons->windowMinimize},
      {"toggle_window_size", &icons->windowToggleSize},
      {"close_app", &icons->windowClose},

      {"reset_view", &icons->resetView},
  };

  int count = (int)(sizeof(entries) / sizeof(entries[0]));
  for (int i = 0; i < count; i++)
    SetIconSrc(entries[i].icon, i);

  const char *atlasPath = "assets/ui_icons/atlas.png";
  if (!AtlasIsStale(atlasPath, entries, count)) {
    icons->atlas = LoadTexture(atlasPath);
  }

  if (icons->atlas.id == 0) {
    icons->atlas = BuildAtlas(atlasPath, entries, count);
  }
}

void GuiIconsUnload(GuiIcons *icons) {
  if (icons->atlas.id != 0)
    UnloadTexture(icons->atlas);
  *icons = (GuiIcons){0};
}

void GuiDrawIconTexture(const GuiIcons *icons, GuiIcon icon, Rectangle bounds,
                        Color tint) {
  if (icons->atlas.id == 0 || icon.src.width <= 0.0f || icon.src.height <= 0.0f)
    return;
  float scale =
      fminf(bounds.width / icon.src.width, bounds.height / icon.src.height);
  scale = fminf(scale, 1.0f);
  float w = icon.src.width * scale;
  float h = icon.src.height * scale;
  Rectangle dst = {bounds.x + (bounds.width - w) / 2.0f,
                   bounds.y + (bounds.height - h) / 2.0f, w, h};
  DrawTexturePro(icons->atlas, icon.src, dst, (Vector2){0, 0}, 0.0f, tint);
}

bool GuiIconButton(GuiState *gui, const GuiIcons *icons, Rectangle bounds,
                   GuiIcon icon, bool active, Color bgActive, Color bgHover,
                   Color iconActive, Color iconIdle, Color iconHover,
                   const char *tooltip) {
  Vector2 mouse = GetMousePosition();
  bool hover = CheckCollisionPointRec(mouse, bounds);

  if (hover)
    GuiTooltipSet(gui, tooltip, bounds);

  if (active)
    DrawRectangleRounded(bounds, 0.25f, 6, bgActive);
  else if (hover)
    DrawRectangleRounded(bounds, 0.25f, 6, bgHover);

  Color tint = active ? iconActive : (hover ? iconHover : iconIdle);
  if (icons->atlas.id != 0)
    GuiDrawIconTexture(icons, icon, bounds, tint);
  else
    DrawRectangleLinesEx(bounds, 1.0f, tint);

  return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

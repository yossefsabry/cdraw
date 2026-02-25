#include "app_paths.h"
#include "gui_internal.h"
#include <limits.h>
#include <stdlib.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

static Font TryLoadFont(const char *path, int baseSize, bool *outOwned) {
  if (outOwned)
    *outOwned = false;
  if (!path || path[0] == '\0')
    return (Font){0};
  if (!FileExists(path))
    return (Font){0};

  Font f = LoadFontEx(path, baseSize, NULL, 0);
  if (f.texture.id == 0)
    return (Font){0};

  SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
  if (outOwned)
    *outOwned = true;
  return f;
}

void GuiFontLoad(GuiState *gui) {
  if (!gui)
    return;

  gui->uiFont = GetFontDefault();
  gui->ownsUiFont = false;

  const int baseSize = 24;

  const char *env = getenv("CDRAW_FONT");
  if (env && env[0] != '\0') {
    bool owned = false;
    Font f = TryLoadFont(env, baseSize, &owned);
    if (f.texture.id != 0) {
      gui->uiFont = f;
      gui->ownsUiFont = owned;
      return;
    }
  }

  const char *assetCandidates[] = {
      "assets/fonts/Inter-Regular.ttf",
      "assets/fonts/Roboto-Regular.ttf",
      "assets/fonts/DejaVuSans.ttf",
  };

  const int assetCount = (int)(sizeof(assetCandidates) / sizeof(assetCandidates[0]));
  for (int i = 0; i < assetCount; i++) {
    char path[PATH_MAX];
    CdrawAssetPath(path, sizeof(path), assetCandidates[i]);
    bool owned = false;
    Font f = TryLoadFont(path, baseSize, &owned);
    if (f.texture.id != 0) {
      gui->uiFont = f;
      gui->ownsUiFont = owned;
      return;
    }
  }

  const char *systemCandidates[] = {
      "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
      "/usr/share/fonts/truetype/dejavu/DejaVuSansCondensed.ttf",
      "/usr/share/fonts/TTF/DejaVuSans.ttf",
      "/usr/share/fonts/TTF/DejaVuSansCondensed.ttf",
      "/usr/share/fonts/dejavu/DejaVuSans.ttf",
      "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
      "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
      "/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf",
      "/usr/share/fonts/gnu-free/FreeSans.otf",
      "/usr/share/fonts/noto/NotoSans-Regular.ttf",
      "/System/Library/Fonts/Supplemental/Arial.ttf",
      "/System/Library/Fonts/Supplemental/Helvetica.ttf",
      "/Library/Fonts/Arial.ttf",
      "C:/Windows/Fonts/segoeui.ttf",
      "C:/Windows/Fonts/arial.ttf",
      "C:\\Windows\\Fonts\\segoeui.ttf",
      "C:\\Windows\\Fonts\\arial.ttf",
  };

  const int systemCount =
      (int)(sizeof(systemCandidates) / sizeof(systemCandidates[0]));
  for (int i = 0; i < systemCount; i++) {
    bool owned = false;
    Font f = TryLoadFont(systemCandidates[i], baseSize, &owned);
    if (f.texture.id != 0) {
      gui->uiFont = f;
      gui->ownsUiFont = owned;
      return;
    }
  }
}

void GuiFontUnload(GuiState *gui) {
  if (!gui)
    return;
  if (gui->ownsUiFont && gui->uiFont.texture.id != 0)
    UnloadFont(gui->uiFont);
  gui->uiFont = GetFontDefault();
  gui->ownsUiFont = false;
}

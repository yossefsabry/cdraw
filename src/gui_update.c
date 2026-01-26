#include "gui_internal.h"
#include "prefs.h"
#include "raymath.h"
#include <string.h>

static bool IsPanningNow(int activeTool, bool inputCaptured) {
  bool isPanning = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
  if (!inputCaptured && IsKeyDown(KEY_SPACE) &&
      IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    isPanning = true;
  if (activeTool == TOOL_PAN && IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
      !inputCaptured)
    isPanning = true;
  return isPanning;
}

static void HandleFileDialogInput(GuiState *gui, Canvas *canvas) {
  gui->isTyping = gui->fileDialogIsSave;

  if (gui->fileDialogIsSave) {
    int key = GetCharPressed();
    while (key > 0) {
      if (key >= 32 && key <= 126) {
        size_t len = strlen(gui->fileDialogName);
        if (len + 1 < sizeof(gui->fileDialogName)) {
          gui->fileDialogName[len] = (char)key;
          gui->fileDialogName[len + 1] = '\0';
        }
      }
      key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
      size_t len = strlen(gui->fileDialogName);
      if (len > 0)
        gui->fileDialogName[len - 1] = '\0';
    }
  }

  if (IsKeyPressed(KEY_ENTER))
    GuiFileDialogConfirm(gui, canvas);
  if (IsKeyPressed(KEY_ESCAPE))
    GuiFileDialogCancel(gui);
}

void UpdateCursor(GuiState *gui, const Canvas *canvas, bool mouseOverGui) {
  int cursor = MOUSE_CURSOR_DEFAULT;

  bool isPanning = IsPanningNow(gui->activeTool, mouseOverGui);
  bool useCustomEraserCursor = !mouseOverGui && !gui->isTyping &&
                               gui->activeTool == TOOL_ERASER && !isPanning;

  static bool cursorHidden = false;
  if (useCustomEraserCursor) {
    if (!cursorHidden) {
      HideCursor();
      cursorHidden = true;
    }
  } else {
    if (cursorHidden) {
      ShowCursor();
      cursorHidden = false;
    }
  }

  if (mouseOverGui) {
    cursor = MOUSE_CURSOR_DEFAULT;
  } else if (gui->isTyping) {
    cursor = MOUSE_CURSOR_IBEAM;
  } else {
    if (isPanning) {
      cursor = MOUSE_CURSOR_RESIZE_ALL;
    } else {
      switch (gui->activeTool) {
      case TOOL_PEN:
      case TOOL_LINE:
      case TOOL_RECT:
      case TOOL_CIRCLE:
        cursor = MOUSE_CURSOR_CROSSHAIR;
        break;
      case TOOL_ERASER:
        cursor = useCustomEraserCursor ? MOUSE_CURSOR_DEFAULT
                                       : MOUSE_CURSOR_CROSSHAIR;
        break;
      case TOOL_SELECT:
        cursor = canvas->isDraggingSelection ? MOUSE_CURSOR_RESIZE_ALL
                                             : MOUSE_CURSOR_POINTING_HAND;
        break;
      case TOOL_PAN:
        cursor = MOUSE_CURSOR_RESIZE_ALL;
        break;
      default:
        cursor = MOUSE_CURSOR_DEFAULT;
        break;
      }
    }
  }

  static int lastCursor = -1;
  if (cursor != lastCursor) {
    SetMouseCursor(cursor);
    lastCursor = cursor;
  }
}

void DrawCursorOverlay(GuiState *gui, const Canvas *canvas, bool mouseOverGui) {
  (void)canvas;

  bool isPanning = IsPanningNow(gui->activeTool, mouseOverGui);
  bool useCustomEraserCursor = !mouseOverGui && !gui->isTyping &&
                               gui->activeTool == TOOL_ERASER && !isPanning;
  if (!useCustomEraserCursor)
    return;

  Theme t = GuiThemeGet(gui->darkMode);
  Vector2 m = GetMousePosition();

  // Eraser uses a world radius computed as (8 + thickness)/zoom, so on-screen the
  // radius is effectively constant in pixels.
  float radius = 8.0f + gui->currentThickness;

  Color halo = ColorAlpha(gui->darkMode ? BLACK : WHITE, 0.35f);
  DrawCircleLines((int)m.x, (int)m.y, radius + 1.0f, halo);
  DrawCircleLines((int)m.x, (int)m.y, radius, t.primary);

  Rectangle iconBounds = {m.x - 12, m.y - 12, 24, 24};
  GuiDrawIconTexture(&gui->icons, gui->icons.eraser, iconBounds, t.text);
}

void UpdateGui(GuiState *gui, Canvas *canvas) {
  Theme t = GuiThemeGet(gui->darkMode);

  canvas->backgroundColor = t.canvas;
  canvas->gridColor = t.grid;
  canvas->selectionColor = t.primary;

  bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
  bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
  bool alt = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);

  if (gui->showWelcome) {
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
      gui->showWelcome = false;
      gui->hasSeenWelcome = true;
      GuiToastSet(gui, "Welcome!");
    }
  } else if (gui->showFileDialog) {
    HandleFileDialogInput(gui, canvas);
  } else {
    float wheel = GetMouseWheelMove();
    if (ctrl && shift && wheel != 0.0f) {
      gui->currentThickness = Clamp(gui->currentThickness + wheel, 1.0f, 21.0f);
    }

    canvas->currentStroke.color = gui->currentColor;
    canvas->currentStroke.thickness = gui->currentThickness;

    if (ctrl && IsKeyPressed(KEY_Z))
      Undo(canvas);
    if (ctrl && IsKeyPressed(KEY_Y))
      Redo(canvas);
    if (ctrl && IsKeyPressed(KEY_S))
      GuiRequestSave(gui, canvas);
    if (ctrl && IsKeyPressed(KEY_O))
      GuiRequestOpen(gui, canvas);
    if (ctrl && IsKeyPressed(KEY_N)) {
      ClearCanvas(canvas);
      GuiMarkNewDocument(gui);
      GuiToastSet(gui, "New canvas.");
    }

    if (!gui->isTyping && !ctrl && !alt) {
      typedef struct {
        KeyboardKey key;
        int tool;
      } ToolShortcut;

      static const ToolShortcut toolShortcuts[] = {
          {KEY_P, TOOL_PEN},      {KEY_B, TOOL_RECT},   {KEY_C, TOOL_CIRCLE},
          {KEY_A, TOOL_LINE},     {KEY_E, TOOL_ERASER}, {KEY_M, TOOL_SELECT},
      };

      const int count =
          (int)(sizeof(toolShortcuts) / sizeof(toolShortcuts[0]));
      for (int i = 0; i < count; i++) {
        if (IsKeyPressed(toolShortcuts[i].key)) {
          gui->activeTool = toolShortcuts[i].tool;
          break;
        }
      }
    }

    if (IsKeyPressed(KEY_G))
      canvas->showGrid = !canvas->showGrid;
    if (IsKeyPressed(KEY_F))
      ToggleFullscreen();
    if (IsKeyPressed(KEY_ESCAPE)) {
      gui->showMenu = false;
      gui->showColorPicker = false;
    }
  }

  static bool hasLastPrefs = false;
  static AppPrefs lastPrefs;
  AppPrefs now = PrefsDefaults();
  now.darkMode = gui->darkMode;
  now.showGrid = canvas->showGrid;
  now.hasSeenWelcome = gui->hasSeenWelcome;
  if (!hasLastPrefs) {
    lastPrefs = now;
    hasLastPrefs = true;
  } else if (now.darkMode != lastPrefs.darkMode || now.showGrid != lastPrefs.showGrid ||
             now.hasSeenWelcome != lastPrefs.hasSeenWelcome) {
    (void)PrefsSave(&now);
    lastPrefs = now;
  }
}

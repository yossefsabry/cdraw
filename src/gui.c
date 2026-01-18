#include "gui.h"
#include "raymath.h"
#include <stdio.h>
#include <string.h>
// --- Theme ------------------------------------------------------------------

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

static Theme ThemeLight(void) {
  return (Theme){
      .background = (Color){243, 244, 246, 255}, // #f3f4f6
      .surface = (Color){255, 255, 255, 255},
      .tab = (Color){243, 244, 246, 255}, // #f3f4f6
      .border = (Color){229, 231, 235, 255}, // #e5e7eb
      .text = (Color){31, 41, 55, 255},       // #1f2937
      .textDim = (Color){107, 114, 128, 255}, // #6b7280
      .primary = (Color){56, 189, 248, 255},  // #38bdf8
      .hover = (Color){0, 0, 0, 18},
      .canvas = (Color){255, 255, 255, 255},
      .grid = (Color){229, 231, 235, 255}, // #e5e7eb
      .groupBg = (Color){243, 244, 246, 255},
  };
}

static Theme ThemeDark(void) {
  return (Theme){
      .background = (Color){24, 24, 27, 255}, // #18181b
      .surface = (Color){39, 39, 42, 255},    // #27272a
      .tab = (Color){0, 0, 0, 60},
      .border = (Color){55, 65, 81, 255},      // #374151
      .text = (Color){243, 244, 246, 255},     // #f3f4f6
      .textDim = (Color){156, 163, 175, 255},  // #9ca3af
      .primary = (Color){56, 189, 248, 255},   // #38bdf8
      .hover = (Color){75, 85, 99, 110},       // #4b5563
      .canvas = (Color){18, 18, 18, 255},      // #121212
      .grid = (Color){42, 42, 42, 255},        // #2a2a2a
      .groupBg = (Color){0, 0, 0, 50},         // dark:bg-black/20-ish
  };
}

static Theme GetTheme(const GuiState *gui) {
  return gui->darkMode ? ThemeDark() : ThemeLight();
}

// --- Helpers ----------------------------------------------------------------

static void SetToast(GuiState *gui, const char *msg) {
  snprintf(gui->toast, sizeof(gui->toast), "%s", msg);
  gui->toastUntil = GetTime() + 2.5;
}

static Texture2D LoadIcon(const char *path) {
  Texture2D tex = {0};
  if (FileExists(path)) {
    tex = LoadTexture(path);
  }
  return tex;
}

static void UnloadIcon(Texture2D *tex) {
  if (tex->id != 0)
    UnloadTexture(*tex);
  *tex = (Texture2D){0};
}

static void DrawIconTexture(Texture2D tex, Rectangle bounds, Color tint) {
  if (tex.id == 0)
    return;

  float scale = fminf(bounds.width / (float)tex.width,
                      bounds.height / (float)tex.height);
  float w = (float)tex.width * scale;
  float h = (float)tex.height * scale;
  Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
  Rectangle dst = {bounds.x + (bounds.width - w) / 2.0f,
                   bounds.y + (bounds.height - h) / 2.0f, w, h};
  DrawTexturePro(tex, src, dst, (Vector2){0, 0}, 0.0f, tint);
}

static bool IconButton(Rectangle bounds, Texture2D icon, bool active,
                       Color bgActive, Color bgHover, Color iconActive,
                       Color iconIdle, Color iconHover) {
  Vector2 mouse = GetMousePosition();
  bool hover = CheckCollisionPointRec(mouse, bounds);

  if (active) {
    DrawRectangleRounded(bounds, 0.25f, 6, bgActive);
  } else if (hover) {
    DrawRectangleRounded(bounds, 0.25f, 6, bgHover);
  }

  Color tint = iconIdle;
  if (active)
    tint = iconActive;
  else if (hover)
    tint = iconHover;

  DrawIconTexture(icon, bounds, tint);
  return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static void DrawMoonIcon(Rectangle bounds, Color fg, Color bg) {
  Vector2 c = {bounds.x + bounds.width / 2.0f, bounds.y + bounds.height / 2.0f};
  float r = fminf(bounds.width, bounds.height) * 0.28f;
  DrawCircleV(c, r, fg);
  DrawCircleV((Vector2){c.x + r * 0.45f, c.y - r * 0.10f}, r * 0.92f, bg);
}

static void DrawSunIcon(Rectangle bounds, Color fg) {
  Vector2 c = {bounds.x + bounds.width / 2.0f, bounds.y + bounds.height / 2.0f};
  float r = fminf(bounds.width, bounds.height) * 0.22f;
  DrawCircleV(c, r, fg);
  for (int i = 0; i < 8; i++) {
    float a = (float)i / 8.0f * PI * 2.0f;
    Vector2 dir = {cosf(a), sinf(a)};
    Vector2 p1 = Vector2Add(c, Vector2Scale(dir, r * 1.6f));
    Vector2 p2 = Vector2Add(c, Vector2Scale(dir, r * 2.2f));
    DrawLineEx(p1, p2, 2.0f, fg);
  }
}

static bool DarkModeButton(Rectangle bounds, GuiState *gui, const Theme *t) {
  Vector2 mouse = GetMousePosition();
  bool hover = CheckCollisionPointRec(mouse, bounds);
  if (hover)
    DrawRectangleRounded(bounds, 0.25f, 6, t->hover);

  if (gui->darkMode)
    DrawSunIcon(bounds, t->textDim);
  else
    DrawMoonIcon(bounds, t->textDim, t->surface);

  return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static bool MinimizeButton(Rectangle bounds, const Theme *t) {
  Vector2 mouse = GetMousePosition();
  bool hover = CheckCollisionPointRec(mouse, bounds);
  if (hover)
    DrawRectangleRounded(bounds, 0.25f, 6, t->hover);
  float cx = bounds.x + bounds.width / 2.0f;
  float cy = bounds.y + bounds.height / 2.0f;
  DrawLineEx((Vector2){cx - 6, cy + 2}, (Vector2){cx + 6, cy + 2}, 2.0f,
             t->textDim);
  return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static bool MaximizeButton(Rectangle bounds, const Theme *t) {
  Vector2 mouse = GetMousePosition();
  bool hover = CheckCollisionPointRec(mouse, bounds);
  if (hover)
    DrawRectangleRounded(bounds, 0.25f, 6, t->hover);
  Rectangle r = {bounds.x + 9, bounds.y + 8, bounds.width - 18,
                 bounds.height - 16};
  DrawRectangleLinesEx(r, 2.0f, t->textDim);
  return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static bool MenuItem(Rectangle bounds, const char *label, const Theme *t) {
  Vector2 mouse = GetMousePosition();
  bool hover = CheckCollisionPointRec(mouse, bounds);
  if (hover)
    DrawRectangleRec(bounds, t->hover);
  DrawText(label, (int)bounds.x + 10, (int)bounds.y + 7, 12,
           hover ? t->text : t->textDim);
  return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// --- Public API -------------------------------------------------------------

void BaseLoadIcons(GuiState *gui) {
  gui->icons.menu = LoadIcon("assets/icons/menu.png");
  gui->icons.openFile = LoadIcon("assets/icons/open_file.png");
  gui->icons.saveFile = LoadIcon("assets/icons/save_file.png");
  gui->icons.undo = LoadIcon("assets/icons/undo.png");
  gui->icons.redo = LoadIcon("assets/icons/redo.png");
  gui->icons.brush = LoadIcon("assets/icons/brush_tool.png");
  gui->icons.rectangle = LoadIcon("assets/icons/rectangle_tool.png");
  gui->icons.circle = LoadIcon("assets/icons/circle_tool.png");
  gui->icons.line = LoadIcon("assets/icons/line_tool.png");
  gui->icons.eraser = LoadIcon("assets/icons/eraser_tool.png");
  gui->icons.select = LoadIcon("assets/icons/selection_tool.png");
  gui->icons.grid = LoadIcon("assets/icons/grid.png");
  gui->icons.fullscreen = LoadIcon("assets/icons/fullscreen.png");
  gui->icons.add = LoadIcon("assets/icons/add.png");
  gui->icons.close = LoadIcon("assets/icons/close.png");
  gui->icons.edit = LoadIcon("assets/icons/edit.png");
  gui->icons.colorPicker = LoadIcon("assets/icons/color_picker.png");
}

void UnloadGui(GuiState *gui) {
  UnloadIcon(&gui->icons.menu);
  UnloadIcon(&gui->icons.openFile);
  UnloadIcon(&gui->icons.saveFile);
  UnloadIcon(&gui->icons.undo);
  UnloadIcon(&gui->icons.redo);
  UnloadIcon(&gui->icons.brush);
  UnloadIcon(&gui->icons.rectangle);
  UnloadIcon(&gui->icons.circle);
  UnloadIcon(&gui->icons.line);
  UnloadIcon(&gui->icons.eraser);
  UnloadIcon(&gui->icons.select);
  UnloadIcon(&gui->icons.grid);
  UnloadIcon(&gui->icons.fullscreen);
  UnloadIcon(&gui->icons.add);
  UnloadIcon(&gui->icons.close);
  UnloadIcon(&gui->icons.edit);
  UnloadIcon(&gui->icons.colorPicker);
}

void InitGui(GuiState *gui) {
  memset(gui, 0, sizeof(*gui));

  gui->activeTool = TOOL_PEN;
  gui->currentColor = BLACK;
  gui->currentThickness = 3.0f;

  gui->toolbarRect = (Rectangle){0, 0, (float)GetScreenWidth(), 88};
  gui->isDraggingSize = false;

  gui->paletteRect = (Rectangle){0, 0, 0, 0};
  gui->showColorPicker = false;
  gui->colorPickerRect = (Rectangle){0, 0, 240, 220};

  Vector3 hsv = ColorToHSV(gui->currentColor);
  gui->hueValue = hsv.x;
  gui->satValue = hsv.y;
  gui->valValue = hsv.z;

  gui->darkMode = false;
  gui->showMenu = false;
  gui->menuRect = (Rectangle){0, 0, 200, 220};
  gui->requestExit = false;

  snprintf(gui->currentFile, sizeof(gui->currentFile), "%s", "drawing.cdraw");
  gui->toast[0] = '\0';
  gui->toastUntil = 0.0;

  BaseLoadIcons(gui);
}

bool IsMouseOverGui(GuiState *gui) {
  const float topH = 88.0f;
  bool overTop = CheckCollisionPointRec(
      GetMousePosition(), (Rectangle){0, 0, (float)GetScreenWidth(), topH});
  bool overPalette =
      CheckCollisionPointRec(GetMousePosition(), gui->paletteRect);
  bool overPicker =
      gui->showColorPicker &&
      CheckCollisionPointRec(GetMousePosition(), gui->colorPickerRect);
  bool overMenu =
      gui->showMenu && CheckCollisionPointRec(GetMousePosition(), gui->menuRect);
  return overTop || overPalette || overPicker || overMenu;
}

void UpdateGui(GuiState *gui, Canvas *canvas) {
  Theme t = GetTheme(gui);

  canvas->backgroundColor = t.canvas;
  canvas->gridColor = t.grid;
  canvas->selectionColor = t.primary;

  canvas->currentStroke.color = gui->currentColor;
  canvas->currentStroke.thickness = gui->currentThickness;

  bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
  if (ctrl && IsKeyPressed(KEY_Z))
    Undo(canvas);
  if (ctrl && IsKeyPressed(KEY_Y))
    Redo(canvas);
  if (ctrl && IsKeyPressed(KEY_S)) {
    if (SaveCanvasToFile(canvas, gui->currentFile))
      SetToast(gui, "Saved.");
    else
      SetToast(gui, "Save failed.");
  }
  if (ctrl && IsKeyPressed(KEY_O)) {
    if (LoadCanvasFromFile(canvas, gui->currentFile))
      SetToast(gui, "Loaded.");
    else
      SetToast(gui, "Load failed.");
  }
  if (ctrl && IsKeyPressed(KEY_N)) {
    ClearCanvas(canvas);
    SetToast(gui, "New canvas.");
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

void DrawGui(GuiState *gui, Canvas *canvas) {
  Theme t = GetTheme(gui);
  int sw = GetScreenWidth();
  int sh = GetScreenHeight();

  // --- Header ---------------------------------------------------------------
  Rectangle header = {0, 0, (float)sw, 40};
  DrawRectangleRec(header, t.surface);
  DrawLineEx((Vector2){0, 40}, (Vector2){(float)sw, 40}, 1, t.border);

  // window dots
  DrawCircle(20, 20, 6, (Color){239, 68, 68, 255});
  DrawCircle(40, 20, 6, (Color){234, 179, 8, 255});
  DrawCircle(60, 20, 6, (Color){34, 197, 94, 255});

  // tab
  Rectangle tab = {90, 8, 150, 32};
  DrawRectangleRounded(tab, 0.20f, 6, t.tab);
  DrawRectangleRoundedLinesEx(tab, 0.20f, 6, 1.0f, t.border);
  DrawIconTexture(gui->icons.edit, (Rectangle){tab.x + 8, tab.y + 6, 18, 18},
                  t.primary);
  DrawText("Untitled Sketch", (int)tab.x + 30, (int)tab.y + 9, 12, t.text);

  Rectangle tabClose = {tab.x + tab.width - 24, tab.y + 6, 18, 18};
  if (IconButton(tabClose, gui->icons.close, false, t.hover, t.hover, t.text,
                 t.textDim, t.text)) {
    ClearCanvas(canvas);
    SetToast(gui, "Cleared.");
  }

  Rectangle newBtn = {tab.x + tab.width + 8, 9, 26, 26};
  if (IconButton(newBtn, gui->icons.add, false, t.hover, t.hover, t.text,
                 t.textDim, t.text)) {
    ClearCanvas(canvas);
    SetToast(gui, "New canvas.");
  }

  // version centered
  const char *vText = "cdraw";
  int tw = MeasureText(vText, 12);
  DrawText(vText, sw / 2 - tw / 2, 14, 12, t.textDim);

  // right controls
  float rx = (float)sw - 120;
  Rectangle darkBtn = {rx, 7, 26, 26};
  if (DarkModeButton(darkBtn, gui, &t)) {
    gui->darkMode = !gui->darkMode;
    SetToast(gui, gui->darkMode ? "Dark mode." : "Light mode.");
  }

  Rectangle minBtn = {rx + 30, 7, 26, 26};
  if (MinimizeButton(minBtn, &t))
    MinimizeWindow();

  Rectangle maxBtn = {rx + 60, 7, 26, 26};
  if (MaximizeButton(maxBtn, &t)) {
    if (IsWindowMaximized())
      RestoreWindow();
    else
      MaximizeWindow();
  }

  Rectangle closeBtn = {rx + 90, 7, 26, 26};
  if (IconButton(closeBtn, gui->icons.close, false, t.hover, t.hover, t.text,
                 t.textDim, t.text)) {
    gui->requestExit = true;
  }

  // --- Toolbar --------------------------------------------------------------
  float ty = 40;
  Rectangle toolbar = {0, ty, (float)sw, 48};
  DrawRectangleRec(toolbar, t.surface);
  DrawLineEx((Vector2){0, ty + 48}, (Vector2){(float)sw, ty + 48}, 1, t.border);

  float tx = 10;
  float btnS = 36;
  float btnY = ty + (48 - btnS) / 2;

  // menu
  Rectangle menuBtn = {tx, btnY, btnS, btnS};
  if (IconButton(menuBtn, gui->icons.menu, false, t.hover, t.hover, t.text,
                 t.textDim, t.text)) {
    gui->showMenu = !gui->showMenu;
    gui->menuRect.x = menuBtn.x;
    gui->menuRect.y = menuBtn.y + menuBtn.height + 6;
  }
  tx += btnS + 10;
  DrawLineEx((Vector2){tx, ty + 10}, (Vector2){tx, ty + 38}, 1, t.border);
  tx += 10;

  // open/save
  Rectangle openBtn = {tx, btnY, btnS, btnS};
  if (IconButton(openBtn, gui->icons.openFile, false, t.hover, t.hover, t.text,
                 t.textDim, t.text)) {
    if (LoadCanvasFromFile(canvas, gui->currentFile))
      SetToast(gui, "Loaded.");
    else
      SetToast(gui, "Load failed.");
  }
  tx += btnS;

  Rectangle saveBtn = {tx, btnY, btnS, btnS};
  if (IconButton(saveBtn, gui->icons.saveFile, false, t.hover, t.hover, t.text,
                 t.textDim, t.text)) {
    if (SaveCanvasToFile(canvas, gui->currentFile))
      SetToast(gui, "Saved.");
    else
      SetToast(gui, "Save failed.");
  }
  tx += btnS + 10;
  DrawLineEx((Vector2){tx, ty + 10}, (Vector2){tx, ty + 38}, 1, t.border);
  tx += 10;

  // undo/redo
  Rectangle undoBtn = {tx, btnY, btnS, btnS};
  if (IconButton(undoBtn, gui->icons.undo, false, t.hover, t.hover, t.text,
                 t.textDim, t.text))
    Undo(canvas);
  tx += btnS;

  Rectangle redoBtn = {tx, btnY, btnS, btnS};
  if (IconButton(redoBtn, gui->icons.redo, false, t.hover, t.hover, t.text,
                 t.textDim, t.text))
    Redo(canvas);
  tx += btnS + 10;
  DrawLineEx((Vector2){tx, ty + 10}, (Vector2){tx, ty + 38}, 1, t.border);
  tx += 10;

  // tool group
  float toolCount = 6;
  float groupW = toolCount * btnS + 8;
  Rectangle toolGroup = {tx, btnY, groupW, btnS};
  DrawRectangleRounded(toolGroup, 0.25f, 6, t.groupBg);
  DrawRectangleRoundedLinesEx(toolGroup, 0.25f, 6, 1.0f, t.border);

  float gx = tx + 4;
  Color activeBg = t.surface;
  Color hoverBg = t.hover;
  Color idleIcon = t.textDim;
  Color hoverIcon = t.text;
  Color activeIcon = t.primary;

  if (IconButton((Rectangle){gx, btnY, btnS, btnS}, gui->icons.brush,
                 gui->activeTool == TOOL_PEN, activeBg, hoverBg, activeIcon,
                 idleIcon, hoverIcon))
    gui->activeTool = TOOL_PEN;
  gx += btnS;
  if (IconButton((Rectangle){gx, btnY, btnS, btnS}, gui->icons.rectangle,
                 gui->activeTool == TOOL_RECT, activeBg, hoverBg, activeIcon,
                 idleIcon, hoverIcon))
    gui->activeTool = TOOL_RECT;
  gx += btnS;
  if (IconButton((Rectangle){gx, btnY, btnS, btnS}, gui->icons.circle,
                 gui->activeTool == TOOL_CIRCLE, activeBg, hoverBg, activeIcon,
                 idleIcon, hoverIcon))
    gui->activeTool = TOOL_CIRCLE;
  gx += btnS;
  if (IconButton((Rectangle){gx, btnY, btnS, btnS}, gui->icons.line,
                 gui->activeTool == TOOL_LINE, activeBg, hoverBg, activeIcon,
                 idleIcon, hoverIcon))
    gui->activeTool = TOOL_LINE;
  gx += btnS;
  if (IconButton((Rectangle){gx, btnY, btnS, btnS}, gui->icons.eraser,
                 gui->activeTool == TOOL_ERASER, activeBg, hoverBg, activeIcon,
                 idleIcon, hoverIcon))
    gui->activeTool = TOOL_ERASER;
  gx += btnS;
  if (IconButton((Rectangle){gx, btnY, btnS, btnS}, gui->icons.select,
                 gui->activeTool == TOOL_SELECT, activeBg, hoverBg, activeIcon,
                 idleIcon, hoverIcon))
    gui->activeTool = TOOL_SELECT;

  tx += groupW + 10;
  DrawLineEx((Vector2){tx, ty + 10}, (Vector2){tx, ty + 38}, 1, t.border);
  tx += 10;

  // color + hex
  Rectangle colBtn = {tx, btnY + 6, 26, 26};
  DrawRectangleRounded(colBtn, 0.25f, 6, gui->currentColor);
  DrawRectangleRoundedLinesEx(colBtn, 0.25f, 6, 1.0f, t.border);

  char hex[10];
  snprintf(hex, sizeof(hex), "#%02X%02X%02X", gui->currentColor.r,
           gui->currentColor.g, gui->currentColor.b);
  DrawText(hex, (int)tx + 32, (int)btnY + 12, 12, t.textDim);

  Rectangle colHit = {tx, btnY, 96, btnS};
  if (CheckCollisionPointRec(GetMousePosition(), colHit) &&
      IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    gui->showColorPicker = !gui->showColorPicker;
    gui->colorPickerRect.x = sw / 2.0f - gui->colorPickerRect.width / 2.0f;
    gui->colorPickerRect.y = (float)sh - 60 - gui->colorPickerRect.height - 10;
  }
  tx += 120;

  // thickness slider
  DrawCircle((int)tx, (int)btnY + 18, 3, t.textDim);
  DrawRectangle((int)tx + 10, (int)btnY + 17, 80, 2, t.border);
  float knobX = (float)tx + 10 + ((gui->currentThickness - 1.0f) / 20.0f) * 80;
  DrawCircle((int)knobX, (int)btnY + 18, 6, t.primary);

  Rectangle sliderHit = {(float)tx + 10, btnY + 4, 80, 28};
  Vector2 mouse = GetMousePosition();
  if (CheckCollisionPointRec(mouse, sliderHit)) {
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
      gui->isDraggingSize = true;
  }
  if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
    gui->isDraggingSize = false;
  if (gui->isDraggingSize) {
    float r = (mouse.x - ((float)tx + 10)) / 80.0f;
    r = Clamp(r, 0, 1);
    gui->currentThickness = 1.0f + r * 20.0f;
  }

  DrawText(TextFormat("%.0f", gui->currentThickness), (int)tx + 100,
           (int)btnY + 12, 12, t.textDim);

  // right side
  float rtx = (float)sw - 90;
  Rectangle gridBtn = {rtx, btnY, btnS, btnS};
  if (IconButton(gridBtn, gui->icons.grid, canvas->showGrid, t.hover, t.hover,
                 t.primary, t.textDim, t.text))
    canvas->showGrid = !canvas->showGrid;

  Rectangle fsBtn = {rtx + 40, btnY, btnS, btnS};
  if (IconButton(fsBtn, gui->icons.fullscreen, false, t.hover, t.hover, t.text,
                 t.textDim, t.text))
    ToggleFullscreen();

  // --- Menu Dropdown --------------------------------------------------------
  if (gui->showMenu) {
    DrawRectangleRec(gui->menuRect, t.surface);
    DrawRectangleLinesEx(gui->menuRect, 1, t.border);

    float mx = gui->menuRect.x;
    float my = gui->menuRect.y;
    float mw = gui->menuRect.width;
    float itemH = 26;

    if (MenuItem((Rectangle){mx, my, mw, itemH}, "New (Ctrl+N)", &t)) {
      ClearCanvas(canvas);
      gui->showMenu = false;
      SetToast(gui, "New canvas.");
    }
    if (MenuItem((Rectangle){mx, my + itemH, mw, itemH}, "Open (Ctrl+O)", &t)) {
      if (LoadCanvasFromFile(canvas, gui->currentFile))
        SetToast(gui, "Loaded.");
      else
        SetToast(gui, "Load failed.");
      gui->showMenu = false;
    }
    if (MenuItem((Rectangle){mx, my + itemH * 2, mw, itemH}, "Save (Ctrl+S)",
                 &t)) {
      if (SaveCanvasToFile(canvas, gui->currentFile))
        SetToast(gui, "Saved.");
      else
        SetToast(gui, "Save failed.");
      gui->showMenu = false;
    }
    if (MenuItem((Rectangle){mx, my + itemH * 3, mw, itemH},
                 "Toggle Grid (G)", &t)) {
      canvas->showGrid = !canvas->showGrid;
      gui->showMenu = false;
    }
    if (MenuItem((Rectangle){mx, my + itemH * 4, mw, itemH},
                 gui->darkMode ? "Light Mode" : "Dark Mode", &t)) {
      gui->darkMode = !gui->darkMode;
      gui->showMenu = false;
      SetToast(gui, gui->darkMode ? "Dark mode." : "Light mode.");
    }
    if (MenuItem((Rectangle){mx, my + itemH * 5, mw, itemH}, "Exit", &t)) {
      gui->requestExit = true;
      gui->showMenu = false;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        !CheckCollisionPointRec(mouse, gui->menuRect) &&
        !CheckCollisionPointRec(mouse, menuBtn)) {
      gui->showMenu = false;
    }
  }

  // --- Bottom Palette -------------------------------------------------------
  float pw = 280;
  float ph = 48;
  float px = (float)sw / 2 - pw / 2;
  float py = (float)sh - 60;
  gui->paletteRect = (Rectangle){px, py, pw, ph};

  DrawRectangleRounded(gui->paletteRect, 1.0f, 10, t.surface);
  DrawRectangleRoundedLinesEx(gui->paletteRect, 1.0f, 10, 1.0f, t.border);

  Color pal[] = {BLACK, WHITE, (Color){239, 68, 68, 255},
                 (Color){59, 130, 246, 255}, (Color){234, 179, 8, 255}};

  float r = 12;
  float startX = px + 24;
  for (int i = 0; i < 5; i++) {
    Vector2 pos = {startX + (float)i * 40, py + ph / 2};
    DrawCircleV(pos, r, pal[i]);
    if (ColorToInt(pal[i]) == ColorToInt(WHITE))
      DrawCircleLines((int)pos.x, (int)pos.y, r, t.border);

    bool selected = ColorToInt(gui->currentColor) == ColorToInt(pal[i]);
    if (selected)
      DrawCircleLines((int)pos.x, (int)pos.y, r + 3, t.primary);

    if (CheckCollisionPointCircle(mouse, pos, r) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      gui->currentColor = pal[i];
      Vector3 hsv2 = ColorToHSV(gui->currentColor);
      gui->hueValue = hsv2.x;
      gui->satValue = hsv2.y;
      gui->valValue = hsv2.z;
    }
  }

  DrawLine((int)(startX + 4 * 40 + 20), (int)(py + 10),
           (int)(startX + 4 * 40 + 20), (int)(py + ph - 10), t.border);

  Rectangle palBtn = {px + pw - 42, py + 8, 32, 32};
  if (IconButton(palBtn, gui->icons.colorPicker, false, t.hover, t.hover, t.text,
                 t.textDim, t.text)) {
    gui->showColorPicker = !gui->showColorPicker;
    gui->colorPickerRect.x = px + pw / 2 - gui->colorPickerRect.width / 2;
    gui->colorPickerRect.y = py - gui->colorPickerRect.height - 10;
  }

  // --- Color Picker ---------------------------------------------------------
  if (gui->showColorPicker) {
    DrawRectangleRounded(gui->colorPickerRect, 0.06f, 6, t.surface);
    DrawRectangleRoundedLinesEx(gui->colorPickerRect, 0.06f, 6, 1.0f, t.border);

    Rectangle sv = {gui->colorPickerRect.x + 12, gui->colorPickerRect.y + 12,
                    160, 160};
    Rectangle hue =
        (Rectangle){sv.x + sv.width + 12, sv.y, 20, sv.height};

    Color hueColor = ColorFromHSV(gui->hueValue, 1.0f, 1.0f);
    DrawRectangleGradientH((int)sv.x, (int)sv.y, (int)sv.width, (int)sv.height,
                           WHITE, hueColor);
    DrawRectangleGradientV((int)sv.x, (int)sv.y, (int)sv.width, (int)sv.height,
                           (Color){0, 0, 0, 0}, BLACK);

    // Hue bar
    for (int i = 0; i < 6; i++) {
      float y0 = hue.y + (hue.height / 6.0f) * (float)i;
      float y1 = hue.y + (hue.height / 6.0f) * (float)(i + 1);
      Color c0 = ColorFromHSV((float)i / 6.0f * 360.0f, 1, 1);
      Color c1 = ColorFromHSV((float)(i + 1) / 6.0f * 360.0f, 1, 1);
      DrawRectangleGradientV((int)hue.x, (int)y0, (int)hue.width,
                             (int)(y1 - y0), c0, c1);
    }
    DrawRectangleLinesEx(hue, 1, t.border);

    Vector2 m = GetMousePosition();
    bool down = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    if (down && CheckCollisionPointRec(m, sv)) {
      gui->satValue = Clamp((m.x - sv.x) / sv.width, 0, 1);
      gui->valValue = Clamp(1.0f - (m.y - sv.y) / sv.height, 0, 1);
      gui->currentColor =
          ColorFromHSV(gui->hueValue, gui->satValue, gui->valValue);
    }
    if (down && CheckCollisionPointRec(m, hue)) {
      float r2 = Clamp((m.y - hue.y) / hue.height, 0, 1);
      gui->hueValue = r2 * 360.0f;
      gui->currentColor =
          ColorFromHSV(gui->hueValue, gui->satValue, gui->valValue);
    }

    // Indicators
    Vector2 svPos = {sv.x + gui->satValue * sv.width,
                     sv.y + (1.0f - gui->valValue) * sv.height};
    DrawCircleLines((int)svPos.x, (int)svPos.y, 5, t.text);
    float hueY = hue.y + (gui->hueValue / 360.0f) * hue.height;
    DrawRectangleLinesEx((Rectangle){hue.x - 2, hueY - 2, hue.width + 4, 4}, 1,
                         t.text);

    // close when clicking outside
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        !CheckCollisionPointRec(m, gui->colorPickerRect) &&
        !CheckCollisionPointRec(m, palBtn) &&
        !CheckCollisionPointRec(m, colHit)) {
      gui->showColorPicker = false;
    }
  }

  // --- Footer ---------------------------------------------------------------
  Rectangle footer = {0, (float)sh - 24, (float)sw, 24};
  DrawRectangleRec(footer, ColorAlpha(t.surface, gui->darkMode ? 0.75f : 0.85f));
  DrawLineEx((Vector2){0, footer.y}, (Vector2){(float)sw, footer.y}, 1, t.border);

  Vector2 mouseWorld = GetScreenToWorld2D(GetMousePosition(), canvas->camera);
  int totalPoints = GetTotalPoints(canvas);
  DrawText(TextFormat("Pos: %.0f, %.0f", mouseWorld.x, mouseWorld.y), 10,
           sh - 18, 10, t.textDim);
  DrawText(TextFormat("Zoom: %.2f", canvas->camera.zoom), 130, sh - 18, 10,
           t.textDim);
  DrawText(TextFormat("FPS: %d", GetFPS()), 240, sh - 18, 10, t.textDim);
  DrawText(TextFormat("Strokes: %d", canvas->strokeCount), sw - 200, sh - 18,
           10, t.textDim);
  DrawText(TextFormat("Points: %d", totalPoints), sw - 100, sh - 18, 10,
           t.textDim);

  // toast
  if (gui->toastUntil > GetTime() && gui->toast[0] != '\0') {
    int mt = MeasureText(gui->toast, 12);
    Rectangle toast = {sw - (float)mt - 30, 92, (float)mt + 20, 26};
    DrawRectangleRounded(toast, 0.25f, 6, ColorAlpha(t.surface, 0.95f));
    DrawRectangleRoundedLinesEx(toast, 0.25f, 6, 1.0f, t.border);
    DrawText(gui->toast, (int)toast.x + 10, (int)toast.y + 7, 12, t.text);
  }
}


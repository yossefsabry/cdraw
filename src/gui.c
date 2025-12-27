#include "gui.h"
#include "raymath.h"
#include <stdio.h>

// --- Colors from code.html (Tailwind config) ---
#define COL_CANVAS_DARK (Color){18, 18, 18, 255}  // #121212
#define COL_SURFACE_DARK (Color){39, 39, 42, 255} // #27272a
#define COL_BORDER (Color){55, 65, 81, 255}       // Gray 700 #374151
#define COL_TEXT (Color){243, 244, 246, 255}      // Gray 100 #f3f4f6
#define COL_TEXT_DIM (Color){156, 163, 175, 255}  // Gray 400 #9ca3af
#define COL_PRIMARY (Color){56, 189, 248, 255}    // Sky Blue #38bdf8
#define COL_HOVER (Color){75, 85, 99, 100}        // Gray 600 transparent-ish

void BaseLoadIcons(GuiState *gui) {}
void UnloadGui(GuiState *gui) {}

void InitGui(GuiState *gui) {
  gui->activeTool = TOOL_PEN;
  gui->currentColor = WHITE;
  gui->currentThickness = 2.0f;
  gui->toolbarRect = (Rectangle){0, 0, GetScreenWidth(), 90};

  gui->isDraggingSize = false;
  gui->paletteRect = (Rectangle){0, 0, 0, 0};

  gui->showColorPicker = false;
  gui->colorPickerRect = (Rectangle){0, 0, 220, 260};
  Vector3 hsv = ColorToHSV(gui->currentColor);
  gui->hueValue = hsv.x;
  gui->satValue = hsv.y;
  gui->valValue = hsv.z;
}

bool IsMouseOverGui(GuiState *gui) {
  bool overTop = CheckCollisionPointRec(
      GetMousePosition(), (Rectangle){0, 0, (float)GetScreenWidth(), 88});
  bool overPalette =
      CheckCollisionPointRec(GetMousePosition(), gui->paletteRect);
  bool overPicker =
      gui->showColorPicker &&
      CheckCollisionPointRec(GetMousePosition(), gui->colorPickerRect);
  return overTop || overPalette || overPicker;
}

// --- Icon Types ---
typedef enum {
  ICON_MENU,
  ICON_FOLDER,
  ICON_SAVE,
  ICON_UNDO,
  ICON_REDO,
  ICON_PEN,
  ICON_ERASER,
  ICON_SELECT,
  ICON_RECT,
  ICON_CIRCLE,
  ICON_LINE,
  ICON_GRID,
  ICON_MAXIMIZE,
  ICON_MINIMIZE,
  ICON_CLOSE,
  ICON_PLUS,
  ICON_CROSS,
  ICON_PAN,
  ICON_OPEN
} IconType;

// --- Icon Drawing Helpers ---
static void DrawIconPath(IconType icon, Rectangle rect, Color color, float th) {
  float cx = rect.x + rect.width / 2;
  float cy = rect.y + rect.height / 2;

  switch (icon) {
  case ICON_MENU:
    DrawLineEx((Vector2){cx - 9, cy - 6}, (Vector2){cx + 9, cy - 6}, th, color);
    DrawLineEx((Vector2){cx - 9, cy}, (Vector2){cx + 9, cy}, th, color);
    DrawLineEx((Vector2){cx - 9, cy + 6}, (Vector2){cx + 9, cy + 6}, th, color);
    break;
  case ICON_FOLDER:
    DrawRectangleLinesEx((Rectangle){cx - 9, cy - 7, 18, 14}, th, color);
    DrawLineEx((Vector2){cx - 9, cy - 7}, (Vector2){cx - 5, cy - 9}, th,
               color); // Tab
    DrawLineEx((Vector2){cx - 5, cy - 9}, (Vector2){cx + 2, cy - 9}, th, color);
    DrawLineEx((Vector2){cx + 2, cy - 9}, (Vector2){cx + 2, cy - 7}, th, color);
    break;
  case ICON_SAVE:
    DrawRectangleLinesEx((Rectangle){cx - 8, cy - 8, 16, 16}, th, color);
    DrawRectangle((int)(cx - 6), (int)(cy - 8), 12, 5, color);
    break;
  case ICON_UNDO:
    // Arc arrow
    DrawRing((Vector2){cx, cy + 2}, 6, 6 + th, 180, 270, 0, color);
    DrawLineEx((Vector2){cx, cy + 2 - 6 - th / 2},
               (Vector2){cx - 7, cy + 2 - 6 - th / 2}, th, color);
    DrawLineEx((Vector2){cx - 7, cy + 2 - 6 - th / 2},
               (Vector2){cx - 7 - 3, cy + 2 - 6 - th / 2 - 3}, th, color);
    DrawLineEx((Vector2){cx - 7, cy + 2 - 6 - th / 2},
               (Vector2){cx - 7 - 3, cy + 2 - 6 - th / 2 + 3}, th, color);
    break;
  case ICON_REDO:
    DrawRing((Vector2){cx, cy + 2}, 6, 6 + th, 90, 180, 0, color);
    DrawLineEx((Vector2){cx, cy + 2 - 6 - th / 2},
               (Vector2){cx + 7, cy + 2 - 6 - th / 2}, th, color);
    DrawLineEx((Vector2){cx + 7, cy + 2 - 6 - th / 2},
               (Vector2){cx + 7 + 3, cy + 2 - 6 - th / 2 - 3}, th, color);
    DrawLineEx((Vector2){cx + 7, cy + 2 - 6 - th / 2},
               (Vector2){cx + 7 + 3, cy + 2 - 6 - th / 2 + 3}, th, color);
    break;
  case ICON_PEN: // Brush icon
    DrawLineEx((Vector2){cx - 4, cy + 4}, (Vector2){cx + 4, cy - 4}, th * 1.5,
               color);
    DrawTriangleLines((Vector2){cx - 4, cy + 4}, (Vector2){cx - 8, cy + 8},
                      (Vector2){cx - 4 + 4, cy + 4 + 4}, color);
    break;
  case ICON_RECT:
    DrawRectangleLinesEx((Rectangle){cx - 8, cy - 7, 16, 14}, th, color);
    break;
  case ICON_CIRCLE:
    DrawRing((Vector2){cx, cy}, 7, 7 + th, 0, 360, 0, color);
    break;
  case ICON_LINE:
    DrawLineEx((Vector2){cx - 8, cy}, (Vector2){cx + 8, cy}, th, color);
    break;
  case ICON_ERASER:
    DrawRectangleLinesEx((Rectangle){cx - 6, cy - 6, 12, 12}, th, color);
    DrawLineEx((Vector2){cx - 6, cy + 6}, (Vector2){cx + 6, cy - 6}, th,
               color); // diagonal
    break;
  case ICON_PAN: // Select
    DrawRectangleLinesEx((Rectangle){cx - 8, cy - 8, 16, 16}, 1, color);
    DrawLineEx((Vector2){cx - 2, cy - 2}, (Vector2){cx + 8, cy + 8}, 1, color);
    break;
  case ICON_GRID:
    DrawLineEx((Vector2){cx - 8, cy - 3}, (Vector2){cx + 8, cy - 3}, th, color);
    DrawLineEx((Vector2){cx - 8, cy + 3}, (Vector2){cx + 8, cy + 3}, th, color);
    DrawLineEx((Vector2){cx - 3, cy - 8}, (Vector2){cx - 3, cy + 8}, th, color);
    DrawLineEx((Vector2){cx + 3, cy - 8}, (Vector2){cx + 3, cy + 8}, th, color);
    break;
  case ICON_MAXIMIZE:
    DrawRectangleLinesEx((Rectangle){cx - 6, cy - 6, 12, 12}, th, color);
    break;
  case ICON_MINIMIZE:
    DrawLineEx((Vector2){cx - 6, cy}, (Vector2){cx + 6, cy}, th, color);
    break;
  case ICON_CLOSE:
    DrawLineEx((Vector2){cx - 5, cy - 5}, (Vector2){cx + 5, cy + 5}, th, color);
    DrawLineEx((Vector2){cx + 5, cy - 5}, (Vector2){cx - 5, cy + 5}, th, color);
    break;
  case ICON_PLUS:
    DrawLineEx((Vector2){cx - 5, cy}, (Vector2){cx + 5, cy}, th, color);
    DrawLineEx((Vector2){cx, cy - 5}, (Vector2){cx, cy + 5}, th, color);
    break;
  default:
    break;
  }
}

static bool GuiIconButton(Rectangle bounds, IconType icon, bool active,
                          Color tint, bool explicitColor) {
  Vector2 mouse = GetMousePosition();
  bool hover = CheckCollisionPointRec(mouse, bounds);

  if (active)
    DrawRectangleRec(bounds,
                     (Color){255, 255, 255, 20}); // Subtle lighter highlight
  else if (hover)
    DrawRectangleRec(bounds, COL_HOVER);

  Color iconC = explicitColor ? tint
                              : (active ? COL_PRIMARY
                                        : (hover ? COL_TEXT : COL_TEXT_DIM));
  if (active && !explicitColor)
    iconC = COL_PRIMARY;

  DrawIconPath(icon, bounds, iconC, 2.0f);

  return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

// --- Components ---

void DrawVerticalDivider(float x, float y, float h) {
  DrawLineEx((Vector2){x, y}, (Vector2){x, y + h}, 1, COL_BORDER);
}

void DrawColorBox(Rectangle rect, Color c) {
  DrawRectangleRounded(rect, 0.4f, 4, c);
  DrawRectangleRoundedLines(rect, 0.4f, 4, COL_BORDER);
}

// --- Main Draw Loop ---

void UpdateGui(GuiState *gui, Canvas *canvas) {
  if (gui->activeTool == TOOL_PEN || gui->activeTool == TOOL_LINE ||
      gui->activeTool == TOOL_RECT || gui->activeTool == TOOL_CIRCLE) {
    canvas->currentStroke.color = gui->currentColor;
    canvas->currentStroke.thickness = gui->currentThickness;
  }
}

void DrawGui(GuiState *gui, Canvas *canvas) {
  int sw = GetScreenWidth();
  int sh = GetScreenHeight(); // Used for Footer

  // 1. Header (40px)
  Rectangle header = {0, 0, (float)sw, 40};
  DrawRectangleRec(header, COL_SURFACE_DARK);
  DrawLineEx((Vector2){0, 40}, (Vector2){(float)sw, 40}, 1, COL_BORDER);

  // Window Controls (Left side per code.html, but actually user wants standard
  // right side? code.html has them deep in structure. Let's put colored dots on
  // left like Mac/Lorien) Actually code.html has <div class="flex items-center
  // space-x-2"> ... dots ... </div> on LEFT
  DrawCircle(20, 20, 6, (Color){239, 68, 68, 255}); // Red
  DrawCircle(40, 20, 6, (Color){234, 179, 8, 255}); // Yellow
  DrawCircle(60, 20, 6, (Color){34, 197, 94, 255}); // Green

  // "Tab" -> Untitled Sketch
  // x pos approx 90
  // Draw tab shape (rounded top)
  DrawRectangleRounded((Rectangle){90, 8, 140, 40}, 0.3f, 4,
                       (Color){0, 0, 0, 30}); // dark background for tab?
  DrawLineEx((Vector2){90, 8}, (Vector2){90, 40}, 1, COL_BORDER); // Border Left
  DrawLineEx((Vector2){90, 8}, (Vector2){230, 8}, 1, COL_BORDER); // Border Top
  DrawLineEx((Vector2){230, 8}, (Vector2){230, 40}, 1,
             COL_BORDER); // Border Right

  DrawIconPath(ICON_PEN, (Rectangle){95, 8, 24, 24}, COL_PRIMARY,
               2); // Edit icon substitute
  DrawText("Untitled Sketch", 125, 14, 12, COL_TEXT);
  DrawIconPath(ICON_CLOSE, (Rectangle){205, 12, 16, 16}, COL_TEXT_DIM, 2);

  // Version Text Centered
  const char *vText = "Lorien v0.6.0";
  int tw = MeasureText(vText, 12);
  DrawText(vText, sw / 2 - tw / 2, 14, 12, COL_TEXT_DIM);

  // Right Controls
  float rx = sw - 100;
  GuiIconButton((Rectangle){rx, 5, 30, 30}, ICON_MAXIMIZE, false, WHITE,
                false); // Dark Mode toggle fake
  GuiIconButton((Rectangle){rx + 30, 5, 30, 30}, ICON_MINIMIZE, false, WHITE,
                false);
  GuiIconButton((Rectangle){rx + 60, 5, 30, 30}, ICON_CLOSE, false, WHITE,
                false);

  // 2. Toolbar (48px) -> y=40, h=48. Total bottom y=88
  float ty = 40;
  Rectangle toolbar = {0, ty, (float)sw, 48};
  DrawRectangleRec(toolbar, COL_SURFACE_DARK);
  DrawLineEx((Vector2){0, ty + 48}, (Vector2){(float)sw, ty + 48}, 1,
             COL_BORDER);

  float tx = 10;
  float tbh = 48;  // bar height
  float btnS = 36; // button size
  float btnY = ty + (tbh - btnS) / 2;

  // Hamburger
  GuiIconButton((Rectangle){tx, btnY, btnS, btnS}, ICON_MENU, false, WHITE,
                false);
  tx += btnS + 8;
  DrawVerticalDivider(tx, ty + 10, 28);
  tx += 8;

  // File
  if (GuiIconButton((Rectangle){tx, btnY, btnS, btnS}, ICON_FOLDER, false,
                    WHITE, false)) {
  }
  tx += btnS;
  if (GuiIconButton((Rectangle){tx, btnY, btnS, btnS}, ICON_SAVE, false, WHITE,
                    false)) {
  }
  tx += btnS + 8;
  DrawVerticalDivider(tx, ty + 10, 28);
  tx += 8;

  // Undo/Redo
  if (GuiIconButton((Rectangle){tx, btnY, btnS, btnS}, ICON_UNDO, false, WHITE,
                    false))
    Undo(canvas);
  tx += btnS;
  if (GuiIconButton((Rectangle){tx, btnY, btnS, btnS}, ICON_REDO, false, WHITE,
                    false))
    Redo(canvas);
  tx += btnS + 8;
  DrawVerticalDivider(tx, ty + 10, 28);
  tx += 8;

  // Tools
  if (GuiIconButton((Rectangle){tx, btnY, btnS, btnS}, ICON_PEN,
                    gui->activeTool == TOOL_PEN, WHITE, false))
    gui->activeTool = TOOL_PEN;
  tx += btnS;
  if (GuiIconButton((Rectangle){tx, btnY, btnS, btnS}, ICON_RECT,
                    gui->activeTool == TOOL_RECT, WHITE, false))
    gui->activeTool = TOOL_RECT;
  tx += btnS;
  if (GuiIconButton((Rectangle){tx, btnY, btnS, btnS}, ICON_CIRCLE,
                    gui->activeTool == TOOL_CIRCLE, WHITE, false))
    gui->activeTool = TOOL_CIRCLE;
  tx += btnS;
  if (GuiIconButton((Rectangle){tx, btnY, btnS, btnS}, ICON_LINE,
                    gui->activeTool == TOOL_LINE, WHITE, false))
    gui->activeTool = TOOL_LINE;
  tx += btnS;
  if (GuiIconButton((Rectangle){tx, btnY, btnS, btnS}, ICON_ERASER,
                    gui->activeTool == TOOL_ERASER, WHITE, false))
    gui->activeTool = TOOL_ERASER;
  tx += btnS;
  if (GuiIconButton((Rectangle){tx, btnY, btnS, btnS}, ICON_PAN,
                    gui->activeTool == TOOL_PAN, WHITE, false))
    gui->activeTool = TOOL_PAN;
  tx += btnS + 8;
  DrawVerticalDivider(tx, ty + 10, 28);
  tx += 8;

  // Properties (Color + Size)
  Rectangle colRect = {tx, btnY + 4, 28, 28};
  DrawColorBox(colRect, gui->currentColor);
  // Draw Text Hex
  char hex[10];
  snprintf(hex, 10, "#%02X%02X%02X", gui->currentColor.r, gui->currentColor.g,
           gui->currentColor.b);
  DrawText(hex, tx + 35, btnY + 10, 10, COL_TEXT);
  tx += 100;

  // Slider
  DrawCircle(tx, btnY + 14, 3, COL_TEXT_DIM);
  DrawRectangle(tx + 10, btnY + 13, 80, 2, COL_BORDER); // Track
  float knobX = tx + 10 + ((gui->currentThickness - 1) / 20.0f) * 80;
  DrawCircle(knobX, btnY + 14, 6, COL_PRIMARY); // Knob

  // Slider Logic
  Rectangle sHit = {tx + 10, btnY, 80, 28};
  Vector2 mouse = GetMousePosition();
  if (CheckCollisionPointRec(mouse, sHit)) {
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
      gui->isDraggingSize = true;
  }
  if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
    gui->isDraggingSize = false;
  if (gui->isDraggingSize) {
    float r = (mouse.x - (tx + 10)) / 80.0f;
    r = Clamp(r, 0, 1);
    gui->currentThickness = 1.0f + r * 20.0f;
  }

  char sizeT[8];
  sprintf(sizeT, "%.0f", gui->currentThickness);
  DrawText(sizeT, tx + 100, btnY + 10, 10, COL_TEXT);

  // Right Side Grid
  float rtx = sw - 100;
  if (GuiIconButton((Rectangle){rtx, btnY, btnS, btnS}, ICON_GRID,
                    canvas->showGrid, WHITE, false))
    canvas->showGrid = !canvas->showGrid;
  if (GuiIconButton((Rectangle){rtx + 36, btnY, btnS, btnS}, ICON_MAXIMIZE,
                    false, WHITE, false)) {
  }

  // 3. Bottom Palette Pill
  // "absolute bottom-10 left-1/2"
  float pw = 280;
  float ph = 48;
  float px = sw / 2 - pw / 2;
  float py = sh - 60;
  gui->paletteRect = (Rectangle){px, py, pw, ph};

  DrawRectangleRounded(gui->paletteRect, 1.0f, 10, COL_SURFACE_DARK);
  DrawRectangleRoundedLines(gui->paletteRect, 1.0f, 10, COL_BORDER);
  // Shadow fake
  // DrawRectangleRoundedLinesEx... just simple border

  Color pal[] = {BLACK, WHITE, (Color){239, 68, 68, 255},
                 (Color){59, 130, 246, 255}, (Color){234, 179, 8, 255}};
  float swR = 12;
  float startX = px + 24;
  for (int i = 0; i < 5; i++) {
    Vector2 pos = {startX + i * 40, py + ph / 2};
    DrawCircleV(pos, swR, pal[i]);
    if (ColorToInt(pal[i]) == ColorToInt(WHITE))
      DrawCircleLines(pos.x, pos.y, swR, COL_BORDER); // Light border for white

    bool isSel = ColorToInt(gui->currentColor) == ColorToInt(pal[i]);
    if (isSel)
      DrawCircleLines(pos.x, pos.y, swR + 3, COL_PRIMARY);

    if (CheckCollisionPointCircle(mouse, pos, swR) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      gui->currentColor = pal[i];
    }
  }
  // Divider
  DrawLine((int)(startX + 4 * 40 + 20), (int)(py + 10),
           (int)(startX + 4 * 40 + 20), (int)(py + ph - 10), COL_BORDER);

  // Palette Button
  Vector2 palPos = {px + pw - 30, py + ph / 2};
  DrawIconPath(ICON_OPEN, (Rectangle){palPos.x - 10, palPos.y - 10, 20, 20},
               COL_TEXT_DIM, 2); // Icon Generic
  if (CheckCollisionPointCircle(mouse, palPos, 20) &&
      IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    gui->showColorPicker = !gui->showColorPicker;
    gui->colorPickerRect.x = px + pw / 2 - 110;
    gui->colorPickerRect.y = py - 270;
  }

  // 4. Footer
  DrawRectangle(0, sh - 24, sw, 24,
                (Color){39, 39, 42, 200}); // Semi transparent
  DrawLine(0, sh - 24, sw, sh - 24, COL_BORDER);
  DrawText(TextFormat("FPS: %d", GetFPS()), 10, sh - 18, 10, COL_TEXT_DIM);
  DrawText(TextFormat("Strokes: %d", canvas->strokeCount), sw - 100, sh - 18,
           10, COL_TEXT_DIM);

  // Picker
  if (gui->showColorPicker) {
    DrawRectangleRec(gui->colorPickerRect, COL_SURFACE_DARK);
    DrawRectangleLinesEx(gui->colorPickerRect, 1, COL_BORDER);
    // ... reuse picker logic or simplify ...
    // Drawing simple gradients
    DrawRectangleGradientEx((Rectangle){gui->colorPickerRect.x + 10,
                                        gui->colorPickerRect.y + 10, 200, 150},
                            WHITE, ColorFromHSV(gui->hueValue, 1, 1), BLACK,
                            BLACK);
    // Logic for picker needs to be updated if needed, reusing previous logic is
    // fine Re-implementing interaction for brevity
    Rectangle sv = {gui->colorPickerRect.x + 10, gui->colorPickerRect.y + 10,
                    200, 150};
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) &&
        CheckCollisionPointRec(mouse, sv)) {
      gui->satValue = (mouse.x - sv.x) / sv.width;
      gui->valValue = 1.0f - (mouse.y - sv.y) / sv.height;
      gui->currentColor =
          ColorFromHSV(gui->hueValue, gui->satValue, gui->valValue);
    }
  }
}

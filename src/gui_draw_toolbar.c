#include "gui_internal.h"
#include "raymath.h"
#include "view_reset.h"
#include <stdio.h>

static void Divider(float x, float y, float h, Color c) {
    DrawLineEx((Vector2){x, y}, (Vector2){x, y + h}, 1, c);
}

static void ColorBox(Rectangle r, Color c, Color border) {
    DrawRectangleRounded(r, 0.25f, 6, c);
    DrawRectangleRoundedLinesEx(r, 0.25f, 6, 1.0f, border);
}

void GuiDrawToolbar(GuiState *gui, Canvas *canvas, Theme t, Color iconIdle,
                    Color iconHover, int sw, int sh, float paletteY) {
    float ty = 40;
    Rectangle toolbar = {0, ty, (float)sw, 48};
    DrawRectangleRec(toolbar, t.surface);
    DrawLineEx((Vector2){0, ty + 48}, (Vector2){(float)sw, ty + 48}, 1, t.border);

    float tx = 10;
    float btnS = 36;
    float btnY = ty + (48 - btnS) / 2;

    gui->menuButtonRect = (Rectangle){tx, btnY, btnS, btnS};
    if (GuiIconButton(&gui->icons, gui->menuButtonRect, gui->icons.menu, false,
                      t.hover, t.hover, t.text, iconIdle, iconHover)) {
        gui->showMenu = !gui->showMenu;
        gui->menuRect.x = gui->menuButtonRect.x;
        gui->menuRect.y = gui->menuButtonRect.y + gui->menuButtonRect.height + 6;
    }
    tx += btnS + 10;
    Divider(tx, ty + 10, 28, t.border);
    tx += 10;

    Rectangle openBtn = {tx, btnY, btnS, btnS};
    if (GuiIconButton(&gui->icons, openBtn, gui->icons.openFile, false, t.hover,
                      t.hover, t.text, iconIdle, iconHover)) {
        GuiToastSet(gui,
                    LoadCanvasFromFile(canvas, gui->currentFile) ? "Loaded."
                    : "Load failed.");
    }
    tx += btnS;

    Rectangle saveBtn = {tx, btnY, btnS, btnS};
    if (GuiIconButton(&gui->icons, saveBtn, gui->icons.saveFile, false, t.hover,
                      t.hover, t.text, iconIdle, iconHover)) {
        GuiToastSet(gui, SaveCanvasToFile(canvas, gui->currentFile) ? "Saved."
                    : "Save failed.");
    }
    tx += btnS + 10;
    Divider(tx, ty + 10, 28, t.border);
    tx += 10;

    Rectangle undoBtn = {tx, btnY, btnS, btnS};
    if (GuiIconButton(&gui->icons, undoBtn, gui->icons.undo, false, t.hover,
                      t.hover, t.text, iconIdle, iconHover))
        Undo(canvas);
    tx += btnS;

    Rectangle redoBtn = {tx, btnY, btnS, btnS};
    if (GuiIconButton(&gui->icons, redoBtn, gui->icons.redo, false, t.hover,
                      t.hover, t.text, iconIdle, iconHover))
        Redo(canvas);
    tx += btnS + 10;
    Divider(tx, ty + 10, 28, t.border);
    tx += 10;

    Rectangle group = {tx, btnY, 6 * btnS + 8, btnS};
    DrawRectangleRounded(group, 0.25f, 6, t.groupBg);
    DrawRectangleRoundedLinesEx(group, 0.25f, 6, 1.0f, t.border);

    float gx = tx + 4;
    Color activeBg = t.surface, hoverBg = t.hover, activeIcon = t.primary;

#define TOOLBTN(icon, tool)                                                    \
    do {                                                                         \
        Rectangle b = {gx, btnY, btnS, btnS};                                      \
        if (GuiIconButton(&gui->icons, b, icon, gui->activeTool == (tool),         \
                          activeBg, hoverBg, activeIcon, iconIdle, iconHover))     \
            gui->activeTool = (tool);                                                \
        gx += btnS;                                                                \
    } while (0)

    TOOLBTN(gui->icons.pen, TOOL_PEN);
    TOOLBTN(gui->icons.rect, TOOL_RECT);
    TOOLBTN(gui->icons.circle, TOOL_CIRCLE);
    TOOLBTN(gui->icons.line, TOOL_LINE);
    TOOLBTN(gui->icons.eraser, TOOL_ERASER);
    TOOLBTN(gui->icons.select, TOOL_SELECT);
    #undef TOOLBTN

    tx += group.width + 10;
    Divider(tx, ty + 10, 28, t.border);
    tx += 10;

    Rectangle swatch = {tx, btnY + 6, 26, 26};
    ColorBox(swatch, gui->currentColor, t.border);
    char hex[10];
    snprintf(hex, sizeof(hex), "#%02X%02X%02X", gui->currentColor.r,
             gui->currentColor.g, gui->currentColor.b);
    DrawTextEx(gui->uiFont, hex, (Vector2){tx + 32, btnY + 12}, 12, 1.0f,
               t.textDim);

    gui->colorButtonRect = (Rectangle){tx, btnY, 96, btnS};
    if (CheckCollisionPointRec(GetMousePosition(), gui->colorButtonRect) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        gui->showColorPicker = !gui->showColorPicker;
        gui->colorPickerRect.x = sw / 2.0f - gui->colorPickerRect.width / 2.0f;
        gui->colorPickerRect.y = paletteY - gui->colorPickerRect.height - 10;
    }
    tx += 120;

    DrawCircle((int)tx, (int)btnY + 18, 3, t.textDim);
    DrawRectangle((int)tx + 10, (int)btnY + 17, 80, 2, t.border);
    float knobX = (float)tx + 10 + ((gui->currentThickness - 1.0f) / 20.0f) * 80;
    DrawCircle((int)knobX, (int)btnY + 18, 6, t.primary);

    Rectangle sliderHit = {(float)tx + 10, btnY + 4, 80, 28};
    Vector2 mouse = GetMousePosition();
    if (CheckCollisionPointRec(mouse, sliderHit) &&
        IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        gui->isDraggingSize = true;
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
        gui->isDraggingSize = false;
    if (gui->isDraggingSize) {
        float r = (mouse.x - ((float)tx + 10)) / 80.0f;
        gui->currentThickness = 1.0f + Clamp(r, 0, 1) * 20.0f;
    }
    DrawTextEx(gui->uiFont, TextFormat("%.0f", gui->currentThickness),
               (Vector2){tx + 100, btnY + 12}, 12, 1.0f, t.textDim);

    float rtx = (float)sw - 130;
    Rectangle resetBtn = {rtx, btnY, btnS, btnS};
    if (GuiIconButton(&gui->icons, resetBtn, gui->icons.resetView, false, t.hover,
                      t.hover, t.text, iconIdle, iconHover)) {
        ResetCanvasView(canvas);
        GuiToastSet(gui, "View reset.");
    }

    Rectangle gridBtn = {rtx + 40, btnY, btnS, btnS};
    if (GuiIconButton(&gui->icons, gridBtn, gui->icons.grid, canvas->showGrid,
                      t.hover, t.hover, t.primary, iconIdle, iconHover))
        canvas->showGrid = !canvas->showGrid;

    Rectangle fsBtn = {rtx + 80, btnY, btnS, btnS};
    if (GuiIconButton(&gui->icons, fsBtn, gui->icons.fullscreen, false, t.hover,
                      t.hover, t.text, iconIdle, iconHover))
        ToggleFullscreen();
}

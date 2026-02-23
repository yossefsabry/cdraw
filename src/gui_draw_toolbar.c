#include "gui_internal.h"
#include "raymath.h"
#include "view_reset.h"
#include <math.h>
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
    float ty = 40.0f;
    float toolbarH = 48.0f;
    Rectangle toolbar = {0, ty, (float)sw, toolbarH};
    DrawRectangleRec(toolbar, t.surface);
    DrawLineEx((Vector2){0, ty + toolbarH}, (Vector2){(float)sw, ty + toolbarH}, 1,
               t.border);

    bool compact = sw < 980;
    bool dense = sw < 820;
    bool ultra = sw < 700;

    float btnS = 36.0f;
    if (compact)
        btnS = 32.0f;
    if (dense)
        btnS = 30.0f;
    if (sw < 640)
        btnS = 28.0f;

    float gap = dense ? 6.0f : 10.0f;
    float dividerPad = dense ? 6.0f : 10.0f;
    float groupPad = dense ? 3.0f : 4.0f;
    float rightMargin = dense ? 8.0f : 12.0f;
    float rightGap = dense ? 6.0f : 8.0f;

    float btnY = ty + (toolbarH - btnS) / 2.0f;
    float dividerH = toolbarH - 20.0f;
    float dividerY = ty + (toolbarH - dividerH) / 2.0f;

    float tx = 10.0f;
    gui->menuButtonRect = (Rectangle){tx, btnY, btnS, btnS};
    if (GuiIconButton(gui, &gui->icons, gui->menuButtonRect, gui->icons.menu,
                      false, t.hover, t.hover, t.text, iconIdle, iconHover,
                      "Menu")) {
        gui->showMenu = !gui->showMenu;
        gui->menuRect.x = gui->menuButtonRect.x;
        gui->menuRect.y = gui->menuButtonRect.y + gui->menuButtonRect.height + 6;
    }
    tx += btnS + gap;
    Divider(tx, dividerY, dividerH, t.border);
    tx += dividerPad;

    Rectangle openBtn = {tx, btnY, btnS, btnS};
    if (GuiIconButton(gui, &gui->icons, openBtn, gui->icons.openFile, false,
                      t.hover, t.hover, t.text, iconIdle, iconHover, "Open")) {
        GuiRequestOpen(gui);
    }
    tx += btnS;

    Rectangle saveBtn = {tx, btnY, btnS, btnS};
    if (GuiIconButton(gui, &gui->icons, saveBtn, gui->icons.saveFile, false,
                      t.hover, t.hover, t.text, iconIdle, iconHover, "Save")) {
        GuiRequestSave(gui, GuiGetActiveDocument(gui));
    }
    tx += btnS + gap;
    Divider(tx, dividerY, dividerH, t.border);
    tx += dividerPad;

    Rectangle undoBtn = {tx, btnY, btnS, btnS};
    if (GuiIconButton(gui, &gui->icons, undoBtn, gui->icons.undo, false, t.hover,
                      t.hover, t.text, iconIdle, iconHover, "Undo"))
        Undo(canvas);
    tx += btnS;

    Rectangle redoBtn = {tx, btnY, btnS, btnS};
    if (GuiIconButton(gui, &gui->icons, redoBtn, gui->icons.redo, false, t.hover,
                      t.hover, t.text, iconIdle, iconHover, "Redo"))
        Redo(canvas);
    tx += btnS + gap;
    Divider(tx, dividerY, dividerH, t.border);
    tx += dividerPad;

    Rectangle group = {tx, btnY, 6 * btnS + groupPad * 2.0f, btnS};
    DrawRectangleRounded(group, 0.25f, 6, t.groupBg);
    DrawRectangleRoundedLinesEx(group, 0.25f, 6, 1.0f, t.border);

    float gx = tx + groupPad;
    Color activeBg = t.surface, hoverBg = t.hover, activeIcon = t.primary;

#define TOOLBTN(icon, tool, label)                                              \
    do {                                                                         \
        Rectangle b = {gx, btnY, btnS, btnS};                                     \
        if (GuiIconButton(gui, &gui->icons, b, icon, gui->activeTool == (tool),   \
                          activeBg, hoverBg, activeIcon, iconIdle, iconHover,    \
                          (label)))                                              \
            gui->activeTool = (tool);                                             \
        gx += btnS;                                                               \
    } while (0)

    TOOLBTN(gui->icons.pen, TOOL_PEN, "Pen");
    TOOLBTN(gui->icons.rect, TOOL_RECT, "Rectangle");
    TOOLBTN(gui->icons.circle, TOOL_CIRCLE, "Circle");
    TOOLBTN(gui->icons.line, TOOL_LINE, "Arrow/Line");
    TOOLBTN(gui->icons.eraser, TOOL_ERASER, "Eraser");
    TOOLBTN(gui->icons.select, TOOL_SELECT, "Select");
    #undef TOOLBTN

    tx += group.width + gap;
    Divider(tx, dividerY, dividerH, t.border);
    tx += dividerPad;

    float swatchSize = fmaxf(18.0f, btnS - 10.0f);
    Rectangle swatch = {tx, btnY + (btnS - swatchSize) / 2.0f, swatchSize,
                        swatchSize};
    ColorBox(swatch, gui->currentColor, t.border);
    char hex[10];
    snprintf(hex, sizeof(hex), "#%02X%02X%02X", gui->currentColor.r,
             gui->currentColor.g, gui->currentColor.b);

    bool showHex = !ultra;
    float colorBtnW = showHex ? (dense ? 86.0f : 96.0f) : (swatchSize + 12.0f);
    gui->colorButtonRect = (Rectangle){tx, btnY, colorBtnW, btnS};
    if (showHex)
        DrawTextEx(gui->uiFont, hex,
                   (Vector2){tx + swatchSize + 6.0f, btnY + 12}, 12, 1.0f,
                   t.textDim);

    if (CheckCollisionPointRec(GetMousePosition(), gui->colorButtonRect)) {
        GuiTooltipSet(gui, "Color picker", gui->colorButtonRect);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            gui->showColorPicker = !gui->showColorPicker;
            gui->colorPickerRect.x = sw / 2.0f - gui->colorPickerRect.width / 2.0f;
            gui->colorPickerRect.y = paletteY - gui->colorPickerRect.height - 10;
            float minX = 8.0f;
            float maxX = (float)sw - gui->colorPickerRect.width - 8.0f;
            if (maxX < minX)
                maxX = minX;
            gui->colorPickerRect.x = Clamp(gui->colorPickerRect.x, minX, maxX);
            float minY = 88.0f;
            float maxY = (float)sh - gui->colorPickerRect.height - 8.0f;
            if (maxY < minY)
                maxY = minY;
            gui->colorPickerRect.y = Clamp(gui->colorPickerRect.y, minY, maxY);
        }
    }
    tx += colorBtnW + gap;

    bool showReset = true;
    bool showGrid = true;
    bool showFull = true;
    float minGap = 12.0f;
    for (int i = 0; i < 3; i++) {
        int count = (showReset ? 1 : 0) + (showGrid ? 1 : 0) + (showFull ? 1 : 0);
        float rightW = count > 0 ? (btnS * count + rightGap * (count - 1)) : 0.0f;
        float rightStart = (float)sw - rightMargin - rightW;
        if (rightStart >= tx + minGap)
            break;
        if (showFull)
            showFull = false;
        else if (showGrid)
            showGrid = false;
        else if (showReset)
            showReset = false;
    }

    int rightCount = (showReset ? 1 : 0) + (showGrid ? 1 : 0) + (showFull ? 1 : 0);
    float rightW = rightCount > 0 ? (btnS * rightCount + rightGap * (rightCount - 1))
                                  : 0.0f;
    float rightStart = (float)sw - rightMargin - rightW;

    float sliderX = tx;
    float available = rightStart - sliderX - minGap;
    if (available < 0.0f)
        available = 0.0f;

    char thicknessText[8];
    snprintf(thicknessText, sizeof(thicknessText), "%.0f", gui->currentThickness);
    Vector2 thicknessSize = MeasureTextEx(gui->uiFont, thicknessText, 12, 1.0f);
    float valueGap = dense ? 8.0f : 10.0f;
    bool showThicknessValue = available > thicknessSize.x + 6.0f;

    float sliderW = dense ? 64.0f : 80.0f;
    float minSliderW = dense ? 40.0f : 52.0f;
    float sliderAvail = available;
    if (showThicknessValue)
        sliderAvail -= thicknessSize.x + valueGap;

    bool showSlider = sliderAvail >= minSliderW;
    if (showSlider)
        sliderW = fminf(sliderW, sliderAvail);

    if (!showSlider)
        gui->isDraggingSize = false;

    if (showSlider) {
        float trackY = btnY + btnS / 2.0f;
        DrawRectangle((int)sliderX, (int)trackY - 1, (int)sliderW, 2, t.border);
        float knobX = sliderX + ((gui->currentThickness - 1.0f) / 20.0f) * sliderW;
        DrawCircle((int)knobX, (int)trackY, 6, t.primary);

        Rectangle sliderHit = {sliderX, btnY + 4.0f, sliderW, btnS - 8.0f};
        Vector2 mouse = GetMousePosition();
        if (CheckCollisionPointRec(mouse, sliderHit) &&
            IsMouseButtonDown(MOUSE_BUTTON_LEFT))
            gui->isDraggingSize = true;
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            gui->isDraggingSize = false;
        if (gui->isDraggingSize) {
            float r = (mouse.x - sliderX) / sliderW;
            gui->currentThickness = 1.0f + Clamp(r, 0, 1) * 20.0f;
            snprintf(thicknessText, sizeof(thicknessText), "%.0f",
                     gui->currentThickness);
            thicknessSize = MeasureTextEx(gui->uiFont, thicknessText, 12, 1.0f);
        }
    }

    if (showThicknessValue) {
        float valueX = sliderX + (showSlider ? sliderW + valueGap : 0.0f);
        DrawTextEx(gui->uiFont, thicknessText, (Vector2){valueX, btnY + 12}, 12,
                   1.0f, t.textDim);
    }

    float rtx = rightStart;
    if (showReset) {
        Rectangle resetBtn = {rtx, btnY, btnS, btnS};
        if (GuiIconButton(gui, &gui->icons, resetBtn, gui->icons.resetView, false,
                          t.hover, t.hover, t.text, iconIdle, iconHover,
                          "Reset view")) {
            ResetCanvasView(canvas);
            GuiToastSet(gui, "View reset.");
        }
        rtx += btnS + rightGap;
    }

    if (showGrid) {
        Rectangle gridBtn = {rtx, btnY, btnS, btnS};
        const char *gridTip = canvas->showGrid ? "Hide grid" : "Show grid";
        if (GuiIconButton(gui, &gui->icons, gridBtn, gui->icons.grid,
                          canvas->showGrid, t.hover, t.hover, t.primary, iconIdle,
                          iconHover, gridTip))
            canvas->showGrid = !canvas->showGrid;
        rtx += btnS + rightGap;
    }

    if (showFull) {
        Rectangle fsBtn = {rtx, btnY, btnS, btnS};
        const char *fsTip = "Toggle fullscreen";
        if (GuiIconButton(gui, &gui->icons, fsBtn, gui->icons.fullscreen, false,
                          t.hover, t.hover, t.text, iconIdle, iconHover, fsTip))
            ToggleFullscreen();
    }
}

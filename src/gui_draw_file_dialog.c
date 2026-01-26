#include "gui_internal.h"
#include <stdio.h>
#include <string.h>

static bool TextButton(Font font, Rectangle r, const char *label, Theme t,
                       bool primary) {
  Vector2 mouse = GetMousePosition();
  bool hover = CheckCollisionPointRec(mouse, r);

  Color bg = primary ? t.primary : t.tab;
  if (!primary && hover)
    bg = t.hover;

  DrawRectangleRounded(r, 0.25f, 8, bg);
  DrawRectangleRoundedLinesEx(r, 0.25f, 8, 1.0f, t.border);

  int fontSize = 12;
  float tw = MeasureTextEx(font, label, (float)fontSize, 1.0f).x;
  Color textColor = primary ? BLACK : t.text;
  DrawTextEx(font, label,
             (Vector2){r.x + (r.width - tw) / 2.0f,
                       r.y + (r.height - (float)fontSize) / 2.0f},
             (float)fontSize, 1.0f, textColor);

  return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static void DrawTruncatedText(Font font, const char *text, Vector2 pos, float maxW,
                              float size, Color color) {
  if (!text || text[0] == '\0')
    return;
  Vector2 full = MeasureTextEx(font, text, size, 1.0f);
  if (full.x <= maxW) {
    DrawTextEx(font, text, pos, size, 1.0f, color);
    return;
  }

  char buf[256];
  size_t len = strlen(text);
  for (size_t i = 0; i < len; i++) {
    const char *tail = text + i;
    snprintf(buf, sizeof(buf), "...%s", tail);
    Vector2 tailSize = MeasureTextEx(font, buf, size, 1.0f);
    if (tailSize.x <= maxW) {
      DrawTextEx(font, buf, pos, size, 1.0f, color);
      return;
    }
  }
  DrawTextEx(font, "...", pos, size, 1.0f, color);
}

static void DrawInputField(GuiState *gui, Rectangle r, Theme t) {
  DrawRectangleRounded(r, 0.18f, 6, t.tab);
  DrawRectangleRoundedLinesEx(r, 0.18f, 6, 1.0f, t.border);

  const char *text = gui->fileDialogName;
  Color textColor = t.text;
  char placeholder[256];
  if (text[0] == '\0') {
    snprintf(placeholder, sizeof(placeholder), "%s", "drawing.cdraw");
    text = placeholder;
    textColor = t.textDim;
  }

  DrawTruncatedText(gui->uiFont, text,
                    (Vector2){r.x + 8.0f, r.y + (r.height - 12.0f) / 2.0f},
                    r.width - 16.0f, 12.0f, textColor);

  if (gui->isTyping) {
    float cx = r.x + r.width - 10.0f;
    float cy = r.y + 6.0f;
    DrawLineEx((Vector2){cx, cy}, (Vector2){cx, cy + r.height - 12.0f}, 1.0f,
               t.primary);
  }
}

void GuiDrawFileDialog(GuiState *gui, Canvas *canvas, Theme t, int sw, int sh) {
  if (!gui->showFileDialog)
    return;

  Color overlay = ColorAlpha(BLACK, gui->darkMode ? 0.55f : 0.35f);
  DrawRectangle(0, 0, sw, sh, overlay);

  float cardW = 640.0f;
  float cardH = 420.0f;
  if (cardW > sw - 48.0f)
    cardW = (float)sw - 48.0f;
  if (cardH > sh - 48.0f)
    cardH = (float)sh - 48.0f;
  Rectangle card = {(float)sw / 2.0f - cardW / 2.0f,
                    (float)sh / 2.0f - cardH / 2.0f, cardW, cardH};

  DrawRectangleRounded(card, 0.04f, 8, t.surface);
  DrawRectangleRoundedLinesEx(card, 0.04f, 8, 1.0f, t.border);

  float pad = 18.0f;
  float x = card.x + pad;
  float y = card.y + pad;

  const char *title = gui->fileDialogIsSave ? "Save drawing" : "Open drawing";
  DrawTextEx(gui->uiFont, title, (Vector2){x, y}, 18, 1.0f, t.text);
  y += 28.0f;

  DrawTextEx(gui->uiFont, "Folder", (Vector2){x, y}, 12, 1.0f, t.textDim);
  y += 16.0f;
  DrawRectangleRounded((Rectangle){x, y, card.width - pad * 2.0f, 26.0f}, 0.16f, 6,
                       t.tab);
  DrawRectangleRoundedLinesEx((Rectangle){x, y, card.width - pad * 2.0f, 26.0f},
                              0.16f, 6, 1.0f, t.border);
  DrawTruncatedText(gui->uiFont, gui->fileDialogDir, (Vector2){x + 8.0f, y + 6.0f},
                    card.width - pad * 2.0f - 16.0f, 12.0f, t.text);
  y += 36.0f;

  float listH = card.height - pad * 3.0f - 120.0f;
  if (listH < 120.0f)
    listH = 120.0f;
  Rectangle listRect = {x, y, card.width - pad * 2.0f, listH};
  DrawRectangleRounded(listRect, 0.08f, 6, t.tab);
  DrawRectangleRoundedLinesEx(listRect, 0.08f, 6, 1.0f, t.border);

  float rowH = 24.0f;
  int visible = (int)(listRect.height / rowH);
  if (visible < 1)
    visible = 1;
  int maxScroll = gui->fileDialogEntryCount - visible;
  if (maxScroll < 0)
    maxScroll = 0;
  if (gui->fileDialogScroll > maxScroll)
    gui->fileDialogScroll = maxScroll;
  if (gui->fileDialogScroll < 0)
    gui->fileDialogScroll = 0;

  Vector2 mouse = GetMousePosition();
  if (CheckCollisionPointRec(mouse, listRect)) {
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
      gui->fileDialogScroll -= (int)wheel;
      if (gui->fileDialogScroll < 0)
        gui->fileDialogScroll = 0;
      if (gui->fileDialogScroll > maxScroll)
        gui->fileDialogScroll = maxScroll;
    }
  }

  for (int i = 0; i < visible; i++) {
    int index = gui->fileDialogScroll + i;
    if (index >= gui->fileDialogEntryCount)
      break;
    FileDialogEntry *entry = &gui->fileDialogEntries[index];
    Rectangle row = {listRect.x + 6.0f, listRect.y + 6.0f + i * rowH,
                     listRect.width - 12.0f, rowH};
    bool hover = CheckCollisionPointRec(mouse, row);
    bool selected = index == gui->fileDialogSelected;
    if (selected)
      DrawRectangleRounded(row, 0.12f, 4, t.hover);
    else if (hover)
      DrawRectangleRounded(row, 0.12f, 4, ColorAlpha(t.hover, 0.6f));

    Color labelColor = entry->isDir ? t.textDim : t.text;
    char label[160];
    if (entry->isUp)
      snprintf(label, sizeof(label), "..");
    else if (entry->isDir)
      snprintf(label, sizeof(label), "[%s] %s", "DIR", entry->name);
    else
      snprintf(label, sizeof(label), "%s", entry->name);

    DrawTruncatedText(gui->uiFont, label,
                      (Vector2){row.x + 8.0f, row.y + 6.0f}, row.width - 16.0f,
                      12.0f, labelColor);

    if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      gui->fileDialogSelected = index;
      double now = GetTime();
      bool doubleClick = gui->fileDialogLastClickIndex == index &&
                         (now - gui->fileDialogLastClickTime) < 0.35;
      gui->fileDialogLastClickIndex = index;
      gui->fileDialogLastClickTime = now;

      if (entry->isDir || entry->isUp) {
        if (doubleClick)
          GuiFileDialogEnterDirectory(gui, entry->path);
      } else if (gui->fileDialogIsSave) {
        snprintf(gui->fileDialogName, sizeof(gui->fileDialogName), "%s",
                 entry->name);
        if (doubleClick)
          GuiFileDialogConfirm(gui, canvas);
      } else if (doubleClick) {
        GuiFileDialogConfirm(gui, canvas);
      }
    }
  }

  if (gui->fileDialogEntryCount == 0) {
    DrawTextEx(gui->uiFont, "No .cdraw files here", (Vector2){listRect.x + 12.0f,
                                                               listRect.y + 12.0f},
               12.0f, 1.0f, t.textDim);
  }

  y += listRect.height + 12.0f;

  if (gui->fileDialogIsSave) {
    DrawTextEx(gui->uiFont, "File name", (Vector2){x, y}, 12, 1.0f, t.textDim);
    y += 16.0f;
    DrawInputField(gui,
                   (Rectangle){x, y, card.width - pad * 2.0f, 34.0f}, t);
    y += 44.0f;
  }

  DrawTextEx(gui->uiFont, "Enter to confirm, Esc to cancel", (Vector2){x, y}, 11,
             1.0f, t.textDim);

  float btnW = 110.0f;
  float btnH = 32.0f;
  Rectangle cancelBtn = {card.x + card.width - pad - btnW,
                         card.y + card.height - pad - btnH, btnW, btnH};
  Rectangle actionBtn = {cancelBtn.x - btnW - 10.0f, cancelBtn.y, btnW, btnH};

  if (TextButton(gui->uiFont, actionBtn,
                 gui->fileDialogIsSave ? "Save" : "Open", t, true))
    GuiFileDialogConfirm(gui, canvas);
  if (TextButton(gui->uiFont, cancelBtn, "Cancel", t, false))
    GuiFileDialogCancel(gui);
}

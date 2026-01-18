CC = gcc
CFLAGS = -g -O0 -Wall -std=c99 -D_DEFAULT_SOURCE -Wno-missing-braces
LDFLAGS = -lraylib -lm -lpthread -ldl -lrt -lX11

SRC_DIR = src
OBJ_DIR = obj
ICON_SRC_DIR = public
ICON_OUT_DIR = assets/ui_icons

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

TARGET = cdraw

.PHONY: all clean ui-icons

SVG_ICONS = $(wildcard $(ICON_SRC_DIR)/*.svg)
PNG_ICONS = $(patsubst $(ICON_SRC_DIR)/%.svg,$(ICON_OUT_DIR)/%.png,$(SVG_ICONS))

all: ui-icons $(TARGET)

ui-icons: $(PNG_ICONS)

$(ICON_OUT_DIR)/%.png: $(ICON_SRC_DIR)/%.svg | $(ICON_OUT_DIR)
	@echo "GEN  $@"
	@sed 's/fill=\"#e3e3e3\"/fill=\"#ffffff\"/g' $< | rsvg-convert -w 24 -h 24 -o $@ -

$(ICON_OUT_DIR):
	mkdir -p $(ICON_OUT_DIR)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

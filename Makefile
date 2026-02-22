CC ?= gcc

DEBUG ?= 0
CFLAGS ?= -Wall -std=c99 -D_DEFAULT_SOURCE -Wno-missing-braces
GTK_CFLAGS := $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS := $(shell pkg-config --libs gtk+-3.0)
ifeq ($(DEBUG),1)
  CFLAGS += -g -O0
else
  CFLAGS += -O2 -DNDEBUG
endif
LDFLAGS = -lraylib -lm -lpthread -ldl -lrt -lX11 -lcurl \
  $(GTK_LIBS)
CFLAGS += $(GTK_CFLAGS)

SRC_DIR = src
AI_DIR = $(SRC_DIR)/ai
OBJ_DIR = obj
ICON_SRC_DIR = public
ICON_OUT_DIR = assets/ui_icons

SRCS = $(wildcard $(SRC_DIR)/*.c) \
  $(wildcard $(AI_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

TARGET = cdraw
BACKEND_DIR = backend_ai
BACKEND_TARGET = $(BACKEND_DIR)/backend_ai

.PHONY: all clean ui-icons backend-ai

SVG_ICONS = $(wildcard $(ICON_SRC_DIR)/*.svg)
PNG_ICONS = $(patsubst $(ICON_SRC_DIR)/%.svg,$(ICON_OUT_DIR)/%.png,$(SVG_ICONS))

all: ui-icons backend-ai $(TARGET)

ui-icons: $(PNG_ICONS)

$(ICON_OUT_DIR)/%.png: $(ICON_SRC_DIR)/%.svg | $(ICON_OUT_DIR)
	@echo "GEN  $@"
	@sed 's/fill=\"#e3e3e3\"/fill=\"#ffffff\"/g' $< | rsvg-convert -w 24 -h 24 -o $@ -

$(ICON_OUT_DIR):
	mkdir -p $(ICON_OUT_DIR)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	$(MAKE) -C $(BACKEND_DIR) clean
	rm -rf $(OBJ_DIR) $(TARGET)
backend-ai:
	$(MAKE) -C $(BACKEND_DIR)

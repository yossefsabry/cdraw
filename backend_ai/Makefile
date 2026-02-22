CC ?= gcc

CFLAGS ?= -Wall -std=c99 -D_DEFAULT_SOURCE -O2
LDFLAGS = -lcurl

SRC_DIR = src
HANDLERS_DIR = $(SRC_DIR)/handlers
UTILS_DIR = $(SRC_DIR)/utils

SRCS = $(SRC_DIR)/main.c \
  $(SRC_DIR)/server.c \
  $(SRC_DIR)/router.c \
  $(HANDLERS_DIR)/health.c \
  $(HANDLERS_DIR)/ai.c \
  $(HANDLERS_DIR)/calculate.c \
  $(UTILS_DIR)/http.c \
  $(UTILS_DIR)/json.c \
  $(UTILS_DIR)/http_client.c

OBJS = $(SRCS:.c=.o)

TARGET = backend_ai

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

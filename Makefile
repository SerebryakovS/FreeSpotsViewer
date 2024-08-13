CC = gcc
CFLAGS = -I/usr/include/modbus -I./modbus -I./config -lwiringPi -pthread -lmodbus -Wall -Wextra -O2

SRC_DIR = .
MODBUS_DIR = modbus

SRC_FILES = $(SRC_DIR)/main.c $(SRC_DIR)/sensor.c $(SRC_DIR)/concentrator.c
MODBUS_SRC_FILES = $(MODBUS_DIR)/modbus_master.c $(MODBUS_DIR)/modbus_slave.c $(MODBUS_DIR)/modbus_helpers.c

OBJ_FILES = $(SRC_FILES:.c=.o) $(MODBUS_SRC_FILES:.c=.o)

TARGET = FreeSpotsViewer

all: $(TARGET)

$(TARGET): $(OBJ_FILES)
	$(CC) $(OBJ_FILES) -o $@ $(CFLAGS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -f $(OBJ_FILES) $(TARGET)

# PHONY targets
.PHONY: all clean

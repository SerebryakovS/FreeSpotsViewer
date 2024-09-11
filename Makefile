CC = gcc
INCLUDES = -I/usr/include/modbus -I./modbus -I./config -I./sensor -I./memcached -I./web
LIBRARIES = -lwiringPi -pthread -lmodbus -lmemcached -lmicrohttpd 
CFLAGS = $(INCLUDES) $(LIBRARIES) -Wall -Wextra -O2

SRC_DIR = .
SRC_FILES = $(SRC_DIR)/main.c $(SRC_DIR)/sensor.c $(SRC_DIR)/concentrator.c
MODBUS_SRC_FILES = modbus/modbus_master.c modbus/modbus_slave.c modbus/modbus_helpers.c config/config.c
SENSOR_SRC_FILES = sensor/sensor_buffer.c sensor/sensor_proto.c sensor/sensor_laddr.c
MEMCACHED_SRC_FILES = memcached/memcached.c
WEB_SRC_FILES = web/web_server.c web/web_handlers.c 

OBJ_FILES = $(SRC_FILES:.c=.o) \
			$(MODBUS_SRC_FILES:.c=.o) \
			$(SENSOR_SRC_FILES:.c=.o) \
			$(MEMCACHED_SRC_FILES:.c=.o) \
			$(WEB_SRC_FILES:.c=.o)

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

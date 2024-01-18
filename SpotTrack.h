
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define REST_PORT         16333
#define PRINT_TAG         "SpotTrack"
#define UNIX_SOCKET_PATH  "/tmp/SpotTrackSocket"
#define RS485_ENABLE_PIN  12
#define RS485_UART_DEVICE "/dev/ttyAMA0"
#define RS485_BUFFER_LEN  1024
#define RS485_TX_DELAY    5000
#define WEB_RESPONSE_SIZE 1024

extern int Rs485ReadFd, Rs485WriteFd, UartFd;

int RunRs485Controller( void );
void RunWebServer( void );

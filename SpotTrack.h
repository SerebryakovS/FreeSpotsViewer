
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
#define RS485_TX_DELAY    10000
#define WEB_RESPONSE_SIZE 1024
#define SPOTS_MAX_COUNT   256

extern int WebToRs485ReadFd, WebToRs485WriteFd, UartFd;

int RunRs485Controller( void );
int RunWebServer( void );

int PrettyPrintJSON(const char*, char*, size_t);
int ExtractJson(const char *, int, char *, int);

bool WriteToRs485(const char *command, char *ResponseBuffer, size_t ResponseBufferSize);
bool ReadFromRs485(char *ResponseBuffer, size_t ResponseBufferSize);
bool Rs485MakeIO(char *Rs485Cmd, char *ResponseBuffer, size_t ResponseBufferSize);

typedef struct{
    char DeviceUid[25];
    bool State;
}Spot;

Spot WorkingSpots[SPOTS_MAX_COUNT];

bool InitCacheWorkingSpots();
void SetCacheSpotState(char *DeviceUid, bool State);
int GetCacheFreeSpotsCount(void);
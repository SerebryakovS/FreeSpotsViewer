
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
#define BACKOFF_TIME      30000 
#define WEB_RESPONSE_SIZE 1024
#define SPOTS_MAX_COUNT   256
#define DEBUG_MODE        1
#define WEB_MODE          2

extern int WebToRs485RecvPipe[2], WebToRs485SendPipe[2], UartFd;

int RunRs485Controller( void );
int RunWebServer( void );

bool InitPipe(int *OutPipeFds, int BindOut, int BindIn);

int PrettyPrintJSON(const char*, char*, size_t);

bool ExtractJsonBlockRead(const char *InputBuffer, int InputSize, char *OutputBuffer, int OutputBufferSize);
bool ExtractJsonNonBlockRead(const char *InputBuffer, int InputSize, char *OutputBuffer, int OutputBufferSize, bool *JsonStart, int *RX_Index);

bool WriteToRs485(const char *Command, char *ResponseBuffer, size_t ResponseBufferSize);
bool ReadFromRs485(char *ResponseBuffer, size_t ResponseBufferSize);
bool Rs485MakeIO(char *Rs485Cmd, char *ResponseBuffer, size_t ResponseBufferSize);

bool HandleClientStatusBeacon(char *ClientRequest, int InputFd);

typedef struct{
    char DeviceUid[25];
    bool State;
}Spot;

extern Spot WorkingSpots[SPOTS_MAX_COUNT];

bool InitCacheWorkingSpots();
void SetCacheSpotState(char *DeviceUid, bool IsParkingClear);
int GetCacheFreeSpotsCount(void);


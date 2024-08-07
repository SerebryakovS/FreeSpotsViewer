
#include "SpotTrack.h"
#include <termios.h>
#include <gpiod.h>

volatile uint32_t LastRxTime = 0;
pthread_mutex_t UartMutex;

int SetupUart() {
    int _UartFd = open(RS485_UART_DEVICE, O_RDWR | O_NOCTTY);
    //int _UartFd = open(RS485_UART_DEVICE, O_RDWR | O_NOCTTY | O_NDELAY); // non-blocking read
    if (_UartFd == -1) {
        perror("Unable to open UART");
        return -EXIT_FAILURE;
    };
    struct termios Options;
    tcgetattr(_UartFd, &Options);
    Options.c_cflag = B115200 | CS8 | CLOCAL | CREAD;
    Options.c_iflag = IGNPAR; // Ignore parity errors
    Options.c_oflag = 0;
    Options.c_lflag = 0;
    tcflush(_UartFd, TCIOFLUSH);
    tcsetattr(_UartFd, TCSANOW, &Options);
    return _UartFd;
};
//
bool InitPipe(int *OutPipeFds){
    int PipeFds[2];
    if (pipe(PipeFds) == -1) {
        return false;
    };
    OutPipeFds[0]  = PipeFds[0];
    OutPipeFds[1]  = PipeFds[1];
    return true;
};
//
void SetTimeValue(uint32_t *VariableToSet){
    struct timeval Now; gettimeofday(&Now, NULL);
    *VariableToSet = Now.tv_sec * 1000000 + Now.tv_usec;
};
bool Rs485ChannelCheck() {
    uint32_t CurrentTime;
    SetTimeValue(&CurrentTime);
    return (CurrentTime - LastRxTime) >= 50;
};
//
bool WriteToRs485(const char *Command, char *ResponseBuffer, size_t ResponseBufferSize, int OutFd) {
    int BackOffTime = BACKOFF_TIME;
    pthread_mutex_lock(&UartMutex);
    bool ReturnValue = true;
    while (true){
        while (Rs485ChannelCheck() && BackOffTime > 0) {
            usleep(1);
            BackOffTime--;
        };
        if (BackOffTime > 0) {
            continue;
        };
        int BytesWrite = write(OutFd, Command, strlen(Command));
        tcflush(OutFd, TCOFLUSH);
        if (BytesWrite <= 0) {
            if (ResponseBuffer != NULL){
                snprintf(ResponseBuffer, ResponseBufferSize, "{\"error\":\"WRITE_ERR\"}\n");
            };
            ReturnValue = false;
        };
        break;
    };
    usleep(200);
    pthread_mutex_unlock(&UartMutex);
    return ReturnValue;
};
//
bool ReadFromRs485(char *ResponseBuffer, size_t ResponseBufferSize, int InFd) {
    fd_set ReadFds; FD_ZERO(&ReadFds); FD_SET(InFd, &ReadFds);
    struct timeval Timeout;
    Timeout.tv_sec = 5; Timeout.tv_usec = 0;
    int SelectResult = select(InFd + 1, &ReadFds, NULL, NULL, &Timeout);
    if (SelectResult > 0) {
        int BytesRead = read(InFd, ResponseBuffer, ResponseBufferSize);
        if (BytesRead > 0){
            return true;
        } else {
            snprintf(ResponseBuffer, ResponseBufferSize, "{\"error\":\"READ_ERR\"}\n");
            return false;
        };
    } else {
        if (SelectResult == 0) {
            snprintf(ResponseBuffer, ResponseBufferSize, "{\"error\":\"TIMEOUT_ERR\"}\n");
        } else {
            snprintf(ResponseBuffer, ResponseBufferSize, "{\"error\":\"SELECT_ERR\"}\n");
        };
        return false;
    };
};
//
bool Rs485MakeIO(char *Rs485Cmd, char *ResponseBuffer, size_t ResponseBufferSize) {
    if (WriteToRs485(Rs485Cmd, ResponseBuffer, ResponseBufferSize, WebToRs485SendPipe[1])) {
        if (ReadFromRs485(ResponseBuffer, ResponseBufferSize, WebToRs485RecvPipe[0])) {
            char PrettyJson[WEB_RESPONSE_SIZE];
            if (PrettyPrintJSON(ResponseBuffer, PrettyJson, sizeof(PrettyJson)) == 0) {
                snprintf(ResponseBuffer, ResponseBufferSize, "%s", PrettyJson);
                return true;
            } else {
                snprintf(ResponseBuffer, ResponseBufferSize, "{\"error\":\"PRINT_ERR\"}\n");
                return false;
            };
        };
    };
    return false;
};
//
bool HandleClientStatusBeacon(char *ClientRequest, int InputFd) {
    const char *TypeField = "\"type\":\"set_status\"";
    const char *UidField = "\"uid\":\"";
    const char *IsFreeField = "\"is_free\":";
    if (!strstr(ClientRequest, TypeField)) {
        return false;
    };
    char Uid[25] = {0};
    char *UidStart = strstr(ClientRequest, UidField);
    if (UidStart) {
        UidStart += strlen(UidField);
        sscanf(UidStart, "%24[^\"]", Uid);
    } else {
        return false;
    };
    bool IsFree = false;
    char *IsFreeStart = strstr(ClientRequest, IsFreeField);
    if (IsFreeStart) {
        IsFreeStart += strlen(IsFreeField);
        if (strncmp(IsFreeStart, "true", 4) == 0) {
            IsFree = true;
        };
    } else {
        return false;
    };
    SetCacheSpotState(Uid, IsFree);
    char AckMessage[RS485_BUFFER_LEN];
    snprintf(AckMessage, sizeof(AckMessage), "{\"uid\":\"%s\",\"type\":\"ACK\"}\n", Uid);
    WriteToRs485(AckMessage, NULL, 0, InputFd);
    return true;
};
//
int RunRs485Controller( int WORKING_MODE ) {
    pthread_mutex_init(&UartMutex, NULL)
    fd_set UartReadFd;
    UartFd = SetupUart();
    if (UartFd < 0) {
        return -EXIT_FAILURE;
    };
    bool IsWaitingForResponse = false;
    struct gpiod_chip *GpioChip;
    struct gpiod_line *GpioLine;

    GpioChip = gpiod_chip_open_by_name("gpiochip0");
    if (!GpioChip) {
        perror("Open chip failed");
        return -EXIT_FAILURE;
    };
    GpioLine = gpiod_chip_get_line(GpioChip, RS485_ENABLE_PIN);
    if (!GpioLine) {
        perror("Get line failed");
        gpiod_chip_close(GpioChip);
        return -EXIT_FAILURE;
    };
    gpiod_line_request_output(GpioLine, "REDE", 0);
    gpiod_line_set_value(GpioLine, 0);


    if (WORKING_MODE == DEBUG_MODE){
        dup2(WebToRs485SendPipe[0], STDIN_FILENO); // stdin redirect
    };

    char RX_Buffer[RS485_BUFFER_LEN]; memset(RX_Buffer, 0, RS485_BUFFER_LEN);
    char TX_Buffer[RS485_BUFFER_LEN]; memset(TX_Buffer, 0, RS485_BUFFER_LEN);
    bool JsonStart = false; int RX_Index = 0;
    printf("[%s][OK]: Running main monitor loop..\n", PRINT_TAG);
    while (1) {
        FD_ZERO(&UartReadFd);
        FD_SET(UartFd, &UartReadFd);
        FD_SET(WebToRs485SendPipe[0], &UartReadFd);

        if (select(UartFd + 1, &UartReadFd, NULL, NULL, NULL) > 0) {
            if (FD_ISSET(UartFd, &UartReadFd)) {
                char TempBuffer[RS485_BUFFER_LEN]; memset(TempBuffer, 0, RS485_BUFFER_LEN);
                int BytesRead = read(UartFd, TempBuffer, sizeof(TempBuffer) - 1);
                if (BytesRead > 0) {
                    SetTimeValue(&LastRxTime);
                    TempBuffer[BytesRead] = '\0';
                    if (ExtractJsonNonBlockRead(TempBuffer, BytesRead, RX_Buffer, RS485_BUFFER_LEN, &JsonStart, &RX_Index)){
                        printf("[%s][RX]: %s\n", PRINT_TAG, RX_Buffer);
                        if (HandleClientStatusBeacon(RX_Buffer, WebToRs485SendPipe[1])){
                            continue;
                        };
                        if (WORKING_MODE == WEB_MODE) {
                            write(WebToRs485RecvPipe[1], RX_Buffer, strlen(RX_Buffer));
                        };
                    };
                };
            };
            if (FD_ISSET(WebToRs485SendPipe[0], &UartReadFd)) {
                int BytesRead = read(WebToRs485SendPipe[0], TX_Buffer, sizeof(TX_Buffer) - 1);
                if (BytesRead > 0) {
                    TX_Buffer[BytesRead] = '\0';
                    if (ExtractJsonBlockRead(TX_Buffer, BytesRead, NULL, 0)){
                        printf("[%s][TX]: %s", PRINT_TAG, TX_Buffer);
                        gpiod_line_set_value(GpioLine, 1);
                        for (size_t Idx = 0; Idx < strlen(TX_Buffer); ++Idx) {
                            write(UartFd, &TX_Buffer[Idx], 1);
                            usleep(10);
                        };
                        usleep(10);
                        gpiod_line_set_value(GpioLine, 0);
                    };
                };
            };
        };
    };
    gpiod_line_release(GpioLine);
    gpiod_chip_close(GpioChip);
    close(UartFd);
    return EXIT_SUCCESS;
};

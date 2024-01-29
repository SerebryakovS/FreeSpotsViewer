
#include "SpotTrack.h"
#include <termios.h>
#include <gpiod.h>

volatile uint32_t LastRxTime = 0;

int SetupUart() {
    int _UartFd = open(RS485_UART_DEVICE, O_RDWR | O_NOCTTY | O_NDELAY);
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
    tcflush(_UartFd, TCIFLUSH);
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
    return (CurrentTime - LastRxTime) >= 100;
};
//
bool WriteToRs485(const char *Command, char *ResponseBuffer, size_t ResponseBufferSize) {
    int BackOffTime = BACKOFF_TIME;
    while (true){
        while (Rs485ChannelCheck() && BackOffTime > 0) {
            usleep(1);
            BackOffTime--;
        };
        if (BackOffTime == 0) {
            continue;
        };
        int BytesWrite = write(WebToRs485SendPipe[1], Command, strlen(Command));
        if (BytesWrite <= 0) {
            snprintf(ResponseBuffer, ResponseBufferSize, "{\"error\":\"WRITE_ERR\"}\n");
            return false;
        };
        return true;
    };
};
//
bool ReadFromRs485(char *ResponseBuffer, size_t ResponseBufferSize) {
    fd_set ReadFds; FD_ZERO(&ReadFds); FD_SET(WebToRs485RecvPipe[0], &ReadFds);
    struct timeval Timeout;
    Timeout.tv_sec = 2; Timeout.tv_usec = 0;
    int SelectResult = select(WebToRs485RecvPipe[0] + 1, &ReadFds, NULL, NULL, &Timeout);
    if (SelectResult > 0) {
        int BytesRead = read(WebToRs485RecvPipe[0], ResponseBuffer, ResponseBufferSize);
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
    if (WriteToRs485(Rs485Cmd, ResponseBuffer, ResponseBufferSize)) {
        if (ReadFromRs485(ResponseBuffer, ResponseBufferSize)) {
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
    int TempFd = WebToRs485SendPipe[1];
    WebToRs485SendPipe[1] = InputFd;
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
    char AckMessage[256];
    snprintf(AckMessage, sizeof(AckMessage), "{\"uid\":\"%s\",\"type\":\"ACK\"}", Uid);
    WriteToRs485(AckMessage, NULL, 0);
    WebToRs485SendPipe[1] = TempFd;
    return true;
};
//
int RunRs485Controller( int WORKING_MODE ) {
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

    int InputFd  = WORKING_MODE == WEB_MODE ? WebToRs485SendPipe[0] : STDIN_FILENO;
    int OutputFd = WORKING_MODE == WEB_MODE ? WebToRs485RecvPipe[1] : STDOUT_FILENO;

    //int flags = fcntl(InputFd, F_GETFL, 0);
    //fcntl(InputFd, F_SETFL, flags | O_NONBLOCK);

    char RX_Buffer[RS485_BUFFER_LEN]; memset(RX_Buffer, 0, RS485_BUFFER_LEN);
    char TX_Buffer[RS485_BUFFER_LEN]; memset(TX_Buffer, 0, RS485_BUFFER_LEN);
    bool JsonStart = false; int RX_Index = 0;
    printf("[%s][OK]: Running main monitor loop..\n", PRINT_TAG);
    while (1) {
        FD_ZERO(&UartReadFd);
        FD_SET(UartFd, &UartReadFd);
        FD_SET(InputFd, &UartReadFd);

        if (select(UartFd + 1, &UartReadFd, NULL, NULL, NULL) > 0) {
            if (FD_ISSET(UartFd, &UartReadFd)) {
                char TempBuffer[256];
                int BytesRead = read(UartFd, TempBuffer, sizeof(TempBuffer) - 1);
                if (BytesRead > 0) {
                    SetTimeValue(&LastRxTime);
                    TempBuffer[BytesRead] = '\0';
                    if (ExtractJsonNonBlockRead(TempBuffer, BytesRead, RX_Buffer, RS485_BUFFER_LEN, &JsonStart, &RX_Index)){
                        printf("[%s][RX]: %s\n", PRINT_TAG, RX_Buffer);
                        if (HandleClientStatusBeacon(RX_Buffer ,InputFd)){
                            continue;
                        };
                        if (WORKING_MODE == WEB_MODE) {
                            write(OutputFd, RX_Buffer, strlen(RX_Buffer));
                        };
                    };
                };
            };
            if (FD_ISSET(InputFd, &UartReadFd)) {
                int BytesRead = read(InputFd, TX_Buffer, sizeof(TX_Buffer) - 1);
                if (BytesRead > 0) {
                    TX_Buffer[BytesRead] = '\0';
                    if (ExtractJsonBlockRead(TX_Buffer, BytesRead, NULL, 0)){
                        printf("[%s][TX]: %s", PRINT_TAG, TX_Buffer);
                        gpiod_line_set_value(GpioLine, 1);
                        for (size_t Idx = 0; Idx < strlen(TX_Buffer); ++Idx) {
                            write(UartFd, &TX_Buffer[Idx], 1);
                            usleep(10);
                        };
                        gpiod_line_set_value(GpioLine, 0);
                        if (WORKING_MODE == DEBUG_MODE) {
                            write(OutputFd, TX_Buffer, strlen(TX_Buffer));
                        };
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

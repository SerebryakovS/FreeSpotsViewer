
#include "SpotTrack.h"
#include <termios.h>
#include <gpiod.h>

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
bool WriteToRs485(const char *command, char *ResponseBuffer, size_t ResponseBufferSize) {
    int BytesWrite = write(WebToRs485WriteFd, command, strlen(command));
    if (BytesWrite <= 0) {
        snprintf(ResponseBuffer, ResponseBufferSize, "{\"error\":\"WRITE_ERR\"}\n");
        return false;
    };
    return true;
};
//
bool ReadFromRs485(char *ResponseBuffer, size_t ResponseBufferSize) {
    char TempBuffer[256];
    int BytesRead = read(UartFd, TempBuffer, sizeof(TempBuffer) - 1);
    if (BytesRead > 0) {
        TempBuffer[BytesRead] = '\0';
        return ExtractJsonBlockRead(TempBuffer, BytesRead, ResponseBuffer, ResponseBufferSize);
    } else if (BytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        snprintf(ResponseBuffer, ResponseBufferSize, "{\"error\":\"READ_ERR\"}\n");
    } else {
        snprintf(ResponseBuffer, ResponseBufferSize, "{\"error\":\"NO_DATA\"}\n");
    };
    return false;
};
//
bool Rs485MakeIO(char *Rs485Cmd, char *ResponseBuffer, size_t ResponseBufferSize){
    if (WriteToRs485(Rs485Cmd, ResponseBuffer, ResponseBufferSize)) {
        if (ReadFromRs485(ResponseBuffer, ResponseBufferSize)) {
            char PrettyJson[WEB_RESPONSE_SIZE];
            if (PrettyPrintJSON(ResponseBuffer, PrettyJson, sizeof(PrettyJson)) == 0) {
                sprintf(ResponseBuffer, "%s", PrettyJson);
                return true;
            } else {
                snprintf(ResponseBuffer, WEB_RESPONSE_SIZE, "{\"error\":\"PRINT_ERR\"}\n");
                return false;
            };
        };
    };
    return true;
};
//
int RunRs485Controller( void ) {
    fd_set UartReadFd;
    UartFd = SetupUart();
    if (UartFd < 0) {
        return -EXIT_FAILURE;
    };
    
    struct timeval Timeout;
    struct gpiod_chip *GpioChip;
    struct gpiod_line *GpioLine;
    struct gpiod_line_request_config GpioLineConfig;

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

    int InputFdUsed;
    if (WebToRs485ReadFd != -1){
        InputFdUsed = WebToRs485ReadFd;
    } else {
        InputFdUsed = STDIN_FILENO;
        int Flags = fcntl(InputFdUsed, F_GETFL, 0);
        fcntl(InputFdUsed, F_SETFL, Flags | O_NONBLOCK);
    };
    //int InputFdUsed = WebToRs485ReadFd != -1 ? WebToRs485ReadFd : STDIN_FILENO;

    char RX_Buffer[RS485_BUFFER_LEN]; memset(RX_Buffer, 0, RS485_BUFFER_LEN);
    char TX_Buffer[RS485_BUFFER_LEN]; memset(TX_Buffer, 0, RS485_BUFFER_LEN);
    char Cmd[32];
    bool JsonStart = false; int RX_Index = 0;
    printf("[%s][OK]: Running main monitor loop..\n", PRINT_TAG);
    while (1) {
        FD_ZERO(&UartReadFd);
        FD_SET(UartFd, &UartReadFd);
        FD_SET(InputFdUsed, &UartReadFd);
        //Timeout.tv_sec = 3;
        //Timeout.tv_usec = 0;
        if (select(UartFd + 1, &UartReadFd, NULL, NULL, NULL) > 0){ //&Timeout) > 0) {
            if (WebToRs485ReadFd == -1){
                if (FD_ISSET(UartFd, &UartReadFd)) {
                    char TempBuffer[256];
                    int BytesRead = read(UartFd, TempBuffer, sizeof(TempBuffer) - 1);
                    if (BytesRead > 0) {
                        TempBuffer[BytesRead] = '\0';
                        if (ExtractJsonNonBlockRead(TempBuffer, BytesRead, RX_Buffer, RS485_BUFFER_LEN, &JsonStart, &RX_Index)) {
                            printf("[%s][RX]: %s\n", PRINT_TAG, RX_Buffer);
                        };
                    };
                };
            };
            if (FD_ISSET(InputFdUsed, &UartReadFd)) {
                int BytesRead = read(InputFdUsed, TX_Buffer, sizeof(TX_Buffer) - 1);
                if (BytesRead > 0) {
                    TX_Buffer[BytesRead] = '\0';
                    tcflush(UartFd, TCIFLUSH);
                    printf("[%s][TX]: %s", PRINT_TAG, TX_Buffer, strlen(TX_Buffer));
                    gpiod_line_set_value(GpioLine, 1);
                    write(UartFd, TX_Buffer, strlen(TX_Buffer));
                    usleep(RS485_TX_DELAY);
                    gpiod_line_set_value(GpioLine, 0);
                };
            };
        };
    };
    gpiod_line_release(GpioLine);
    gpiod_chip_close(GpioChip);
    close(UartFd);
    return EXIT_SUCCESS;
};

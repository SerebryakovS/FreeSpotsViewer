
#include "SpotTrack.h"
#include <termios.h>
#include <gpiod.h>

int SetupUart() {
    int UartFileDescriptor = open(RS485_UART_DEVICE, O_RDWR | O_NOCTTY | O_NDELAY);
    if (UartFileDescriptor == -1) {
        perror("Unable to open UART");
        return -EXIT_FAILURE;
    };
    struct termios Options;
    tcgetattr(UartFileDescriptor, &Options);
    Options.c_cflag = B115200 | CS8 | CLOCAL | CREAD; 
    Options.c_iflag = IGNPAR; // Ignore parity errors
    Options.c_oflag = 0;
    Options.c_lflag = 0;
    tcflush(UartFileDescriptor, TCIFLUSH);
    tcsetattr(UartFileDescriptor, TCSANOW, &Options);
    return UartFileDescriptor;
};

int RunControllerOnly(int ReadFd, int WriteFd) {
    fd_set UartReadFd;
    struct timeval Timeout;

    int UartFileDescriptor = SetupUart();
    if (UartFileDescriptor < 0) {
        return -EXIT_FAILURE;
    };
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

    int ReadFdUsed = ReadFd != -1 ? ReadFd : STDIN_FILENO;


    int Flags = fcntl(ReadFdUsed, F_GETFL, 0);
    fcntl(ReadFdUsed, F_SETFL, Flags | O_NONBLOCK);

    char RX_Buffer[RS485_BUFFER_LEN]; memset(RX_Buffer, 0, RS485_BUFFER_LEN);
    char TX_Buffer[RS485_BUFFER_LEN]; memset(TX_Buffer, 0, RS485_BUFFER_LEN);
    int RX_Index = 0; bool JsonStart = false;
    char Cmd[32];
    printf("[%s][OK]: Running main monitor loop..\n", PRINT_TAG);
    while (1) {
        FD_ZERO(&UartReadFd);
        FD_SET(UartFd, &UartReadFd);
        FD_SET(ReadFdUsed, &UartReadFd);
        Timeout.tv_sec = 3;
        Timeout.tv_usec = 0;
        if (select(UartFd + 1, &UartReadFd, NULL, NULL, &Timeout) > 0) {
            if (ReadFdUsed == -1){
                if (FD_ISSET(UartFd, &UartReadFd)) {
                    char TempBuffer[256];
                    int BytesRead = read(UartFd, TempBuffer, sizeof(TempBuffer) - 1);
                    if (BytesRead > 0) {
                        TempBuffer[BytesRead] = '\0';
                        for (int Idx = 0; Idx < BytesRead; ++Idx) {
                            if (TempBuffer[Idx] == '{') {
                                JsonStart = true;
                                RX_Index = 0;
                            };
                            if (JsonStart) {
                                RX_Buffer[RX_Index++] = TempBuffer[Idx];
                                if (TempBuffer[Idx] == '}') {
                                    RX_Buffer[RX_Index] = '\0';
                                    printf("[%s][RX]: %s\n", PRINT_TAG, RX_Buffer);
                                    JsonStart = false;
                                    memset(RX_Buffer, 0, RS485_BUFFER_LEN);
                                    RX_Index = 0; 
                                };
                            };
                            if (RX_Index >= RS485_BUFFER_LEN) {
                                RX_Index = 0;
                                JsonStart = false;
                                memset(RX_Buffer, 0, RS485_BUFFER_LEN);
                            };
                        };
                    };
                };
            };
            if (FD_ISSET(ReadFdUsed, &UartReadFd)) {
                int BytesRead = read(ReadFdUsed, TX_Buffer, sizeof(TX_Buffer) - 1);
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


// compilation: gcc Controller.c -o out -lgpiod

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <termios.h>
#include <gpiod.h>

#define RS485_ENABLE 12
#define UART_DEVICE "/dev/ttyAMA0"
#define BUFFER_LEN  1024

char *TAG = "SpotTrack";

int SetupUart() {
    int UartFileDescriptor = open(UART_DEVICE, O_RDWR | O_NOCTTY | O_NDELAY);
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

int main() {
    fd_set UartReadFileDescriptor;
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
    GpioLine = gpiod_chip_get_line(GpioChip, RS485_ENABLE);
    if (!GpioLine) {
        perror("Get line failed");
        gpiod_chip_close(GpioChip);
        return -EXIT_FAILURE;
    };
    gpiod_line_request_output(GpioLine, "REDE", 0);
    gpiod_line_set_value(GpioLine, 0);

    char RX_Buffer[BUFFER_LEN]; memset(RX_Buffer, 0, BUFFER_LEN);
    char TX_Buffer[BUFFER_LEN]; memset(TX_Buffer, 0, BUFFER_LEN);
    int RX_Index = 0; bool JsonStart = false;
    char Cmd[32];
    printf("[%s][OK]: Running main monitor loop..\n",TAG);
    while (1) {
        scanf("[SpotTrack]: >>%s",&Cmd);
        if (strlen(Cmd) > 0){
            
        };

        FD_ZERO(&UartReadFileDescriptor);
        FD_SET(UartFileDescriptor, &UartReadFileDescriptor);
        FD_SET(STDIN_FILENO, &UartReadFileDescriptor);
        Timeout.tv_sec = 1;
        Timeout.tv_usec = 0;

        if (select(UartFileDescriptor + 1, &UartReadFileDescriptor, NULL, NULL, &Timeout) > 0) {
            if (FD_ISSET(UartFileDescriptor, &UartReadFileDescriptor)) {
                char TempBuffer[256];
                int BytesRead = read(UartFileDescriptor, TempBuffer, sizeof(TempBuffer) - 1);
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
                                printf("[%s][RX]: %s\n", TAG, RX_Buffer);
                                JsonStart = false;
                                memset(RX_Buffer, 0, BUFFER_LEN);
                                RX_Index = 0; 
                            };
                        };
                        if (RX_Index >= BUFFER_LEN) {
                            RX_Index = 0;
                            JsonStart = false;
                            memset(RX_Buffer, 0, BUFFER_LEN);
                        };
                    };
                };
                tcflush(UartFileDescriptor, TCIFLUSH);
            };
            if (FD_ISSET(STDIN_FILENO, &UartReadFileDescriptor)) {
                if (fgets(TX_Buffer, BUFFER_LEN, stdin) != NULL) {
                    gpiod_line_set_value(GpioLine, 1);
                    write(UartFileDescriptor, TX_Buffer, strlen(TX_Buffer));
                    usleep(1000);
                    gpiod_line_set_value(GpioLine, 0);
                };
            };
        };
    };
    gpiod_line_release(GpioLine);
    gpiod_chip_close(GpioChip);
    close(UartFileDescriptor);
    return EXIT_SUCCESS;
};

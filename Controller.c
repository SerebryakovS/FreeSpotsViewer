
// compilation: gcc FreeSpotsViewer/Controller.c -o out -lwiringPi

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#define RS485_ENABLE 12
#define UART_DEVICE "/dev/ttyAMA0"
#define BUFFER_LEN  1024

int SetupUart() {
    int UartFileDescriptor = open(UART_DEVICE, O_RDWR | O_NOCTTY | O_NDELAY);
    if (UartFileDescriptor == -1) {
        perror("Unable to open UART");
        return -EXIT_FAILURE;
    };
    struct termios Options;
    tcgetattr(UartFileDescriptor, &Options);
    Options.c_cflag = B115200 | CS8 | CLOCAL | CREAD; // Set baud rate, 8-bit data, local mode, enable receiver
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

    GpioChip = gpiod_chip_open_by_name(GPIO_CHIP);
    if (!GpioChip) {
        perror("Open chip failed");
        return -EXIT_FAILURE;
    };
    GpioLine = gpiod_chip_get_line(GpioChip, RS485_ENABLE);
    if (!GpioLine) {
        perror("Get line failed");
        gpiod_chip_close(chip);
        return -EXIT_FAILURE;
    };

    char RX_Buffer[BUFFER_LEN];
    char TX_Buffer[BUFFER_LEN];
    int PacketLength;

    while (1) {
        FD_ZERO(&UartReadFileDescriptor);
        FD_SET(UartFileDescriptor, &UartReadFileDescriptor);
        FD_SET(STDIN_FILENO, &UartReadFileDescriptor);
        Timeout.tv_sec = 1;
        Timeout.tv_usec = 0;

        if (select(UartFileDescriptor + 1, &UartReadFileDescriptor, NULL, NULL, &Timeout) > 0) {
            if (FD_ISSET(UartFileDescriptor, &UartReadFileDescriptor)) {
                PacketLength = read(UartFileDescriptor, (void*)RX_Buffer, BUFFER_LEN);
                if (PacketLength > 0) {
                    RX_Buffer[PacketLength] = '\0';
                    printf("[..]: %s\n", RX_Buffer);
                };
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

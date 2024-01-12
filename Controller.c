#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <wiringPi.h>

#define RS485_ENABLE 12
#define UART_DEVICE "/dev/serial0"
#define BUFFER_LEN  1024

int SetupUart() {
    int UartFileDescriptor = open(UART_DEVICE, O_RDWR | O_NOCTTY | O_NDELAY);
    if (UartFileDescriptor == -1) {
        perror("Unable to open UART");
        return -EXIT_FAILURE;
    };
    struct termios Options;
    tcgetattr(uart_fd, &Options);
    Options.c_cflag = B9600 | CS8 | CLOCAL | CREAD; // Set baud rate, 8-bit data, local mode, enable receiver
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
    wiringPiSetupGpio();
    pinMode(RS485_ENABLE, OUTPUT);
    digitalWrite(RS485_ENABLE, LOW);

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
                    rx_buffer[PacketLength] = '\0';
                    printf("[..]: %s\n", RX_Buffer);
                };
            };
            if (FD_ISSET(STDIN_FILENO, &UartReadFileDescriptor)) {
                if (fgets(TX_Buffer, BUFFER_LEN, stdin) != NULL) {
                    digitalWrite(RS485_ENABLE, HIGH);
                    write(UartFileDescriptor, TX_Buffer, strlen(TX_Buffer));
                    usleep(1000);
                    digitalWrite(RS485_ENABLE, LOW);
                };
            };
        };
    };
    close(UartFileDescriptor);
    return EXIT_SUCCESS;
}


// gcc -I/usr/include/modbus -I/usr/include/libmemcached SensorsProto.c ConcentratorsProto.c Main.c Spotrack.h -o out -lwiringPi -pthread -lmodbus -lmemcached

#include "Spotrack.h"

pthread_t ThreadA, ThreadB;

void HandleSigint(int Signal) {
    printf("Caught CTRL+C. Closing app..\n");
    close(_UartModuleA.UartPortFd);
    close(_UartModuleB.UartPortFd);
    pthread_cancel(ThreadA);
    pthread_cancel(ThreadB);
    digitalWrite(_UartModuleA.EnablePin, LOW);
    digitalWrite(_UartModuleB.EnablePin, LOW);
    exit(EXIT_SUCCESS);
};

uint16_t SetupUart(const char *UartPort) {
    uint16_t UartPortFd = open(UartPort, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (UartPortFd == -1) {
        perror("Unable to open UART port");
        return -EXIT_FAILURE;
    };
    struct termios UartOptions;
    tcgetattr(UartPortFd, &UartOptions);
    UartOptions.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
    UartOptions.c_iflag = IGNPAR;
    UartOptions.c_oflag = 0;
    UartOptions.c_lflag = 0;
    tcflush(UartPortFd, TCIOFLUSH);
    tcsetattr(UartPortFd, TCSANOW, &UartOptions);
    return UartPortFd;
};

uint8_t main(void) {
    wiringPiSetup();
    _UartModuleA.PortId = 'A';
    _UartModuleA.EnablePin = RS485_CTRL_PIN_A;
    pinMode(_UartModuleA.EnablePin, OUTPUT);
    digitalWrite(_UartModuleA.EnablePin, LOW);
    _UartModuleA.UartPortFd = SetupUart(RS485_UART_PORT_A);
    if (_UartModuleA.UartPortFd == -1) {
        return -EXIT_FAILURE;
    } else {
        _UartModuleB.PortId = 'B';
        _UartModuleB.EnablePin = RS485_CTRL_PIN_B;
        pinMode(_UartModuleB.EnablePin, OUTPUT);

        pinMode(RS485_ROLE_PIN_B, INPUT);
        pullUpDnControl(RS485_ROLE_PIN_B, PUD_UP);


        pinMode(RS485_ROLE_LED_B, OUTPUT);


        digitalWrite(_UartModuleB.EnablePin, LOW);
        _UartModuleB.UartPortFd = SetupUart(RS485_UART_PORT_B);
        if (_UartModuleA.UartPortFd == -1) {
            close(_UartModuleA.UartPortFd);
            return -EXIT_FAILURE;
        };
    };
    signal(SIGINT, HandleSigint);
    // pthread_create(&ThreadA, NULL, SyncClientsHandler, (void *)&_UartModuleA);
    // while (1) {
    //     GetSensorsStatus();
    // };
    // pthread_join(ThreadA, NULL);
    // close(_UartModuleA.UartPortFd);

    pthread_create(&ThreadB, NULL, SyncConcentratorsHandler, (void *)&_UartModuleA);
    pthread_join(ThreadB, NULL);

    return EXIT_SUCCESS;
};

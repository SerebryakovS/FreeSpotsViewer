
#include "spotrack.h"

pthread_t ThreadA, ThreadB;

void HandleSigint(int Signal) {
    printf("Caught CTRL+C. Closing app..\n");
    if (_UartModuleA.UartPortFd != -1) {
        close(_UartModuleA.UartPortFd);
    };
    if (_UartModuleB.UartPortFd != -1) {
        close(_UartModuleB.UartPortFd);
    };
    pthread_cancel(ThreadA);
    pthread_cancel(ThreadB);
    digitalWrite(_UartModuleA.EnablePin, LOW);
    digitalWrite(_UartModuleB.EnablePin, LOW);
    exit(EXIT_SUCCESS);
};

uint16_t SetupUart(const char *UartPort, speed_t BaudSpeed) {
    int16_t UartPortFd = open(UartPort, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (UartPortFd == -1) {
        perror("Unable to open UART port");
        return -EXIT_FAILURE;
    };
    struct termios UartOptions;
    tcgetattr(UartPortFd, &UartOptions);
    UartOptions.c_cflag = BaudSpeed | CS8 | CLOCAL | CREAD;
    UartOptions.c_iflag = IGNPAR;
    UartOptions.c_oflag = 0;
    UartOptions.c_lflag = 0;
    tcflush(UartPortFd, TCIOFLUSH);
    tcsetattr(UartPortFd, TCSANOW, &UartOptions);
    return UartPortFd;
};

#ifdef TEST_MODE
void SimulateSensorData() {
    for (uint8_t Idx = 0; Idx < SENSORS_COUNT; ++Idx) {
        uint8_t sensorValue = rand() % 2;
        UpdateSensor(Idx, sensorValue);
    };
    StoreSensorDataInMemcached(GetSlaveId(), SensorsHead);
};
#endif

int32_t main(void) {
    wiringPiSetup();
    _UartModuleA.PortId = 'A';
    _UartModuleA.EnablePin = RS485_CTRL_PIN_A;
    pinMode(_UartModuleA.EnablePin, OUTPUT);
    digitalWrite(_UartModuleA.EnablePin, LOW);
    _UartModuleA.UartPortFd = SetupUart(RS485_UART_PORT_A, B19200);
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
        _UartModuleB.UartPortFd = SetupUart(RS485_UART_PORT_B, B9600);
        if (_UartModuleA.UartPortFd == -1) {
            close(_UartModuleA.UartPortFd);
            return -EXIT_FAILURE;
        };
    };
    signal(SIGINT, HandleSigint);

#ifdef TEST_MODE
    SimulateSensorData();
#endif
    pthread_create(&ThreadA, NULL, SyncClientsHandler, (void *)&_UartModuleA);
    pthread_create(&ThreadB, NULL, SyncConcentratorsHandler, (void *)&_UartModuleB);
    pthread_join(ThreadA, NULL);
    pthread_join(ThreadB, NULL);
    close(_UartModuleA.UartPortFd);
    close(_UartModuleB.UartPortFd);
    return EXIT_SUCCESS;
};


#include "SpotTrack.h"

int PrettyPrintJSON(const char* InputJson, char* OutputBuffer, size_t BufferSize) {
    FILE *SubProcessPipe;
    char Command[WEB_RESPONSE_SIZE + 64]; 
    snprintf(Command, sizeof(Command), "echo '%s' | jq .", InputJson);
    SubProcessPipe = popen(Command, "r");
    if (!SubProcessPipe) {
        return -1; 
    };
    size_t bytesRead = fread(OutputBuffer, 1, BufferSize - 1, SubProcessPipe);
    OutputBuffer[bytesRead] = '\0';
    pclose(SubProcessPipe);
    return 0; 
};

bool ExtractJsonBlockRead(const char *InputBuffer, int InputSize, char *OutputBuffer, int OutputBufferSize) {
    bool JsonStart = false;
    int RX_Index = 0;
    memset(OutputBuffer, 0, OutputBufferSize);
    for (int Idx = 0; Idx < InputSize; ++Idx) {
        if (InputBuffer[Idx] == '{') {
            JsonStart = true;
            RX_Index = 0;
        };
        if (JsonStart) {
            OutputBuffer[RX_Index++] = InputBuffer[Idx];
            if (InputBuffer[Idx] == '}') {
                OutputBuffer[RX_Index] = '\0';
                printf("[%s][RX]: %s\n", PRINT_TAG, OutputBuffer);
                JsonStart = false;
                RX_Index = 0; 
                return true;
            };
        };
        if (RX_Index >= OutputBufferSize - 1 && OutputBuffer != NULL) {
            RX_Index = 0;
            JsonStart = false;
            memset(OutputBuffer, 0, OutputBufferSize);
        };
    };
    printf("[%s]: Json parsing failed\n", PRINT_TAG);
    return false;
};

bool ExtractJsonNonBlockRead(const char *InputBuffer, int InputSize, char *OutputBuffer, int OutputBufferSize, bool *JsonStart, int *RX_Index) {
    for (int Idx = 0; Idx < InputSize; ++Idx) {
        if (InputBuffer[Idx] == '{' && !*JsonStart) {
            *JsonStart = true;
            *RX_Index = 0;
        };
        if (*JsonStart) {
            OutputBuffer[*RX_Index] = InputBuffer[Idx];
            (*RX_Index)++;
            if (InputBuffer[Idx] == '}') {
                OutputBuffer[*RX_Index] = '\0';
                printf("[%s][RX]: %s\n", PRINT_TAG, OutputBuffer);
                *JsonStart = false;
                return true;
            };
        };
        if (*RX_Index >= OutputBufferSize - 1) {
            *RX_Index = 0;
            *JsonStart = false;
            memset(OutputBuffer, 0, OutputBufferSize);
        };
    };
    return false;
};
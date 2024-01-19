
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

int ExtractJson(const char *InputBuffer, int InputSize, char *OutputBuffer, int OutputBufferSize) {
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
            };
        };
        if (RX_Index >= OutputBufferSize - 1) {
            RX_Index = 0;
            JsonStart = false;
            memset(OutputBuffer, 0, OutputBufferSize);
        };
    };
};
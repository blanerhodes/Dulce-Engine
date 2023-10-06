#include "logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "defines.h"

//b8 InitializeLogging(u64* memoryRequirement, void* state){
//    *memoryRequirement = sizeof(LoggerSystemState);
//    if(state == 0){
//        return true;
//    }
//    logger_state_ptr = (LoggerSystemState*)state;
//
//
//    DFATAL("A test message: %f", 3.14f);
//    DERROR("A test message: %f", 3.14f);
//    DWARN("A test message: %f", 3.14f);
//    DINFO("A test message: %f", 3.14f);
//    DDEBUG("A test message: %f", 3.14f);
//    DTRACE("A test message: %f", 3.14f);
//
//    return true;
//}
//
//void ShutdownLogging(void* state) {
//    logger_state_ptr = 0;
//    //TODO: cleanup logging/write queued entries
//}

void Win32ConsoleWrite(char* message, u8 color) {
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    //fatal, error, info, debug, trace
    u8 levels[6] = {64, 4, 6, 2, 1, 8};
    SetConsoleTextAttribute(console_handle, levels[color]);

    OutputDebugStringA(message);
    u64 length = strlen(message);
    LPDWORD bytes_written = 0;
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), message, (DWORD)length, bytes_written, 0);
}

void Win32ConsoleWriteError(char* message, u8 color) {
    HANDLE console_handle = GetStdHandle(STD_ERROR_HANDLE);
    //fatal, error, info, debug, trace
    u8 levels[6] = {64, 4, 6, 2, 1, 8};
    SetConsoleTextAttribute(console_handle, levels[color]);

    OutputDebugStringA(message);
    u64 length = strlen(message);
    LPDWORD numberWritten = 0;
    WriteConsoleA(GetStdHandle(STD_ERROR_HANDLE), message, (DWORD)length, numberWritten, 0);
}

i32 StringFormatV(char* dest, char* format, va_list va_listp) {
    if (dest) {
        // Big, but can fit on the stack.
        char buffer[32000];
        i32 written = vsnprintf(buffer, 32000, format, va_listp);
        buffer[written] = 0;
        memcpy(dest, buffer, written + 1);

        return written;
    }
    return -1;
}

i32 StringFormat(char* dest, char* format, ...) {
    if (dest) {
        va_list arg_ptr;
        va_start(arg_ptr, format);
        i32 written = StringFormatV(dest, format, arg_ptr);
        va_end(arg_ptr);
        return written;
    }
    return -1;
}

void LogOutput(LogLevel level, char* message, ...) {
    //TODO: all this stuff has to go on another thread since it's so slow
    const char* level_strings[6] = {"[FATAL]: ", "[ERROR]: ", "[WARN]: ", "[INFO]: ", "[DEBUG]: ", "[TRACE]: "};
    b8 is_error = level < LOG_LEVEL_WARN;

    char out_message[32000] = {};

    //this va list type workaround is because MSFT headers override the Clang va_list type with char* sometimes
    va_list arg_ptr;
    va_start(arg_ptr, message);
    StringFormatV(out_message, message, arg_ptr);
    va_end(arg_ptr);

    StringFormat(out_message, "%s%s\n", level_strings[level], out_message);

    if (is_error) {
        Win32ConsoleWriteError(out_message, level);
    } else{
        Win32ConsoleWrite(out_message, level);
    }

}

void ReportAssertionFailure(char* expression, char* message, char* file, i32 line){
    LogOutput(LOG_LEVEL_FATAL, "Assertion Failure: %s, message: '%s', in file: %s, line: %d\n", expression, message, file, line);
}
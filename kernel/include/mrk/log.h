#pragma once

#include <stdarg.h>

#define KLOG_BUFF_LEN (UINT16_MAX + 1)

// Simple logging functions
void log(char* fmt, ...);

#define todo(x, ...) _todo(__FILE__, __LINE__, x, ##__VA_ARGS__)
void _todo(char* file, int line, char* fmt, ...);

// Flush the ringbuffer
void flush_buffer();

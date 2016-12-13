/*
 * Copyright 2015, Yukai Engineering
 * All Right Reserved.
 */
#ifndef __LOG_H__
#define __LOG_H__

#include "types.h"

#define DEBUG

#define LOG_I(...)  log(0, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_W(...)  log(1, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define LOG_E(...)  log(2, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

extern void log(int type, char *file_name, const char *func_name, int line_no, const char* text);

#endif


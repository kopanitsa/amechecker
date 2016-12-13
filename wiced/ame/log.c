/*
 * Copyright 2015, Yukai Engineering
 * All Right Reserved.
 */
#include "stdio.h"
#include "platform.h"
#include "bleapp.h"
#include "log.h"

#define DEBUG_LOG

void log(int type, char *file_name, const char *func_name, int line_no, const char *text);

void log(int type, char *file_name, const char *func_name, int line_no, const char *text) {
#ifdef DEBUG_LOG
    UINT8 str[256];
    sprintf(str,"\n<%d>[%d]%20s: %s\n",type, line_no, func_name, text);
    ble_trace0((char *)str);
#endif
}

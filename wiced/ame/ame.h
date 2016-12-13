/*
 * Copyright 2014, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
*
* BLE Vendor Specific Device
*
* This file provides definitions and function prototypes for Hello Sensor
* device
*
*/
#ifndef AME_H
#define AME_H

// following definitions are shared between client and sensor
// to avoid unnecessary GATT Discovery
//
#define HANDLE_AME_SERVICE_UUID               0x28
#define HANDLE_AME_CHARACTERISTIC_BUTTON      0x29
#define HANDLE_AME_VALUE_BUTTON               0x2a
#define HANDLE_AME_CHARACTERISTIC_WEATHER     0x2b
#define HANDLE_AME_VALUE_WEATHER              0x2c
#define HANDLE_AME_CONFIGURATION              0x2d

// Please note that all UUIDs need to be reversed when publishing in the database

// C334A1D5-2D05-4865-B121-6DBC8BD37F31
#define UUID_AME_SERVICE   0x31, 0x7F, 0xD3, 0x8B, 0xBC, 0x6D, 0x21, 0xB1, 0x65, 0x48, 0x05, 0x2D, 0xD5, 0xA1, 0x34, 0xC3

// FF7278B6-5892-4771-A63C-F061EC2A8AC3
#define UUID_AME_CHARACTERISTIC_BUTTON    0xC3, 0x8A, 0x2A, 0xEC, 0x61, 0xF0, 0x3C, 0xA6, 0x71, 0x47, 0x92, 0x58, 0xB6, 0x78, 0x72, 0xFF

// 2DCA3470-42F2-4FB0-AC9F-4203EB80B1EE
#define UUID_AME_CHARACTERISTIC_WEATHER   0xEE, 0xB1, 0x80, 0xEB, 0x03, 0x42, 0x9F, 0xAC, 0xB0, 0x4F, 0xF2, 0x42, 0x70, 0x34, 0xCA, 0x2D

#endif

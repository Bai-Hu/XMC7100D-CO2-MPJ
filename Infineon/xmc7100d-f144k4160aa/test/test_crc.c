/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-22     Developer    CRC-16 Modbus test
 */

#include <rtthread.h>
#include "modbus_rtu.h"

/**
 * Test CRC calculation with known examples from S8 manual
 */
static void test_crc_examples(void)
{
    rt_uint8_t test_data1[] = {0xFE, 0x04, 0x00, 0x03, 0x00, 0x01};
    rt_uint8_t test_data2[] = {0xFE, 0x03, 0x00, 0x00, 0x00, 0x02};
    rt_uint16_t calculated_crc;
    
    rt_kprintf("\n=== CRC-16 Modbus Test ===\n");
    
    /* Test case 1: Read Input Register (from manual) */
    calculated_crc = modbus_crc16(test_data1, sizeof(test_data1));
    rt_kprintf("[TEST1] Data: FE 04 00 03 00 01\n");
    rt_kprintf("[TEST1] Expected CRC: D5 C5 (from manual)\n");
    rt_kprintf("[TEST1] Calculated CRC: %04X\n", calculated_crc);
    rt_kprintf("[TEST1] Low byte: %02X, High byte: %02X\n", 
               calculated_crc & 0xFF, (calculated_crc >> 8) & 0xFF);
    
    if ((calculated_crc & 0xFF) == 0xC5 && ((calculated_crc >> 8) & 0xFF) == 0xD5) {
        rt_kprintf("[TEST1] PASS: CRC calculation correct!\n");
    } else {
        rt_kprintf("[TEST1] FAIL: CRC calculation wrong!\n");
    }
    
    /* Test case 2: Read Holding Register */
    calculated_crc = modbus_crc16(test_data2, sizeof(test_data2));
    rt_kprintf("\n[TEST2] Data: FE 03 00 00 00 02\n");
    rt_kprintf("[TEST2] Expected CRC: ? (manual reference needed)\n");
    rt_kprintf("[TEST2] Calculated CRC: %04X\n", calculated_crc);
    rt_kprintf("[TEST2] Low byte: %02X, High byte: %02X\n", 
               calculated_crc & 0xFF, (calculated_crc >> 8) & 0xFF);
    
    rt_kprintf("\n=== CRC Test Complete ===\n");
}

/* Export to msh */

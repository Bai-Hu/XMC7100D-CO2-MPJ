/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-22     Developer    Simple S8 communication test
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "modbus_rtu.h"

/* Simple test function to verify S8 communication */
static void test_s8_simple(int argc, char *argv[])
{
    modbus_rtu_device_t *mb_device;
    rt_uint16_t co2_value;
    int retry_count = 3;
    int retry;
    
    RT_UNUSED(argc);
    RT_UNUSED(argv);
    
    rt_kprintf("[S8_TEST] Simple S8 Communication Test\n");
    rt_kprintf("[S8_TEST] ===============================\n");
    
    /* Initialize Modbus RTU */
    mb_device = modbus_rtu_init("uart2");
    if (!mb_device) {
        rt_kprintf("[S8_TEST] FAILED: Could not initialize Modbus RTU\n");
        return;
    }
    
    rt_kprintf("[S8_TEST] Modbus RTU initialized successfully\n");
    
    /* Test communication with retries */
    for (retry = 0; retry < retry_count; retry++) {
        rt_kprintf("[S8_TEST] Attempt %d/%d:\n", retry + 1, retry_count);
        
        /* Test with only legal S8 register addresses from manual */
        rt_uint16_t test_addrs[] = {0x0003};  /* Only confirmed CO2 register */
        const char* addr_names[] = {"0x0003 (CO2 Concentration)"};
        int addr_idx;
        
        for (addr_idx = 0; addr_idx < 1; addr_idx++) {
            rt_uint16_t test_addr = test_addrs[addr_idx];
            
            rt_kprintf("[S8_TEST] Testing %s...\n", addr_names[addr_idx]);
            
            /* Try input register */
            if (modbus_read_input_registers(mb_device, 0xFE, test_addr, 1, &co2_value) == RT_EOK) {
                rt_kprintf("[S8_TEST] SUCCESS! %s = %d ppm\n", addr_names[addr_idx], co2_value);
                rt_kprintf("[S8_TEST] Communication with S8 sensor: WORKING!\n");
                goto test_complete;
            } else {
                rt_kprintf("[S8_TEST] FAILED: %s\n", addr_names[addr_idx]);
            }
            
            rt_thread_mdelay(200);  /* Wait between different address tests */
        }
        
        if (retry < retry_count - 1) {
            rt_kprintf("[S8_TEST] Waiting 2 seconds before retry...\n");
            rt_thread_mdelay(2000);
        }
    }
    
test_complete:
    modbus_rtu_deinit(mb_device);
    
    rt_kprintf("[S8_TEST] ===============================\n");
    rt_kprintf("[S8_TEST] Test completed\n");
}

/* Export to MSH commands */
MSH_CMD_EXPORT(test_s8_simple, Simple S8 communication test);

/* Auto-initialization */
static int s8_test_init(void)
{
    rt_kprintf("[S8_TEST] Simple S8 Test Tool Loaded\n");
    rt_kprintf("[S8_TEST] Use 'test_s8_simple' to test communication\n");
    return RT_EOK;
}


/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-25     Developer    Simplified S8 debug tool
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "s8_sensor.h"

static s8_sensor_device_t *g_s8_debug_sensor = RT_NULL;

/**
 * Simple S8 communication test
 */
static void s8_simple_test(void)
{
    rt_uint16_t co2_value = 0;
    rt_err_t result;
    
    rt_kprintf("=== S8 Simple Communication Test ===\n");
    
    /* Initialize sensor */
    g_s8_debug_sensor = s8_sensor_init("uart2");
    if (!g_s8_debug_sensor) {
        rt_kprintf("S8 Init: FAILED\n");
        return;
    }
    
    rt_kprintf("S8 Init: SUCCESS\n");
    
    /* Wait for sensor to stabilize */
    rt_thread_mdelay(2000);
    
    /* Test CO2 reading */
    rt_kprintf("Reading CO2...\n");
    result = s8_read_co2_data(g_s8_debug_sensor);
    
    if (result == RT_EOK) {
        co2_value = g_s8_debug_sensor->data.co2_ppm;
        rt_kprintf("CO2: %d ppm\n", co2_value);
        rt_kprintf("S8 Communication: SUCCESS\n");
    } else {
        rt_kprintf("CO2 Reading: FAILED (error: %d)\n", result);
        rt_kprintf("S8 Communication: FAILED\n");
    }
    
    /* Cleanup */
    if (g_s8_debug_sensor) {
        s8_sensor_deinit(g_s8_debug_sensor);
        g_s8_debug_sensor = RT_NULL;
    }
    
    rt_kprintf("=== Test Complete ===\n");
}

/**
 * Test raw Modbus communication
 */
static void s8_raw_modbus_test(void)
{
    modbus_rtu_device_t *mb_device;
    rt_uint16_t co2_value = 0;
    rt_err_t result;
    
    rt_kprintf("=== Raw Modbus Test ===\n");
    
    /* Initialize Modbus RTU */
    mb_device = modbus_rtu_init("uart2");
    if (!mb_device) {
        rt_kprintf("Modbus Init: FAILED\n");
        return;
    }
    
    rt_kprintf("Modbus Init: SUCCESS\n");
    
    /* Wait for sensor to stabilize */
    rt_thread_mdelay(2000);
    
    /* Read CO2 from register 0x0003 */
    rt_kprintf("Sending: FE 04 00 03 00 01 D5 C5\n");
    result = modbus_read_input_registers(mb_device, 0xFE, 0x0003, 1, &co2_value);
    
    if (result == RT_EOK) {
        rt_kprintf("CO2: %d ppm\n", co2_value);
        rt_kprintf("Raw Modbus: SUCCESS\n");
    } else {
        rt_kprintf("CO2 Reading: FAILED (error: %d)\n", result);
        rt_kprintf("Raw Modbus: FAILED\n");
    }
    
    /* Cleanup */
    if (mb_device) {
        modbus_rtu_deinit(mb_device);
        mb_device = RT_NULL;
    }
    
    rt_kprintf("=== Test Complete ===\n");
}

/**
 * Test different sensor addresses
 */
static void s8_address_test(void)
{
    modbus_rtu_device_t *mb_device;
    rt_uint16_t co2_value = 0;
    rt_uint8_t test_addresses[] = {0xFE, 0x01, 0x02, 0x03};
    rt_uint8_t i;
    
    rt_kprintf("=== Address Scan Test ===\n");
    
    /* Initialize Modbus RTU */
    mb_device = modbus_rtu_init("uart2");
    if (!mb_device) {
        rt_kprintf("Modbus Init: FAILED\n");
        return;
    }
    
    /* Test each address */
    for (i = 0; i < sizeof(test_addresses); i++) {
        rt_kprintf("Testing address: 0x%02X\n", test_addresses[i]);
        
        co2_value = 0;
        rt_err_t result = modbus_read_input_registers(mb_device, test_addresses[i], 0x0003, 1, &co2_value);
        
        if (result == RT_EOK) {
            rt_kprintf("  SUCCESS: CO2 = %d ppm\n", co2_value);
        } else {
            rt_kprintf("  FAILED: error %d\n", result);
        }
        
        rt_thread_mdelay(500);  /* Delay between tests */
    }
    
    /* Cleanup */
    modbus_rtu_deinit(mb_device);
    rt_kprintf("=== Test Complete ===\n");
}

/**
 * Check hardware connections
 */
static void s8_hardware_check(void)
{
    rt_kprintf("=== Hardware Connection Check ===\n");
    rt_kprintf("Required Connections:\n");
    rt_kprintf("  P19_0(RX) ← S8_TXD (CO2 sensor TX)\n");
    rt_kprintf("  P19_1(TX) → S8_RXD (CO2 sensor RX)\n");
    rt_kprintf("  P20_1(IO5) → S8_R/T (control line)\n");
    rt_kprintf("  G+ → 5V, G0 → GND\n");
    rt_kprintf("  UART2: 9600-8N1\n");
    rt_kprintf("=== Check Complete ===\n");
}

/* Export to msh */
MSH_CMD_EXPORT(s8_simple_test, Simple S8 communication test);
MSH_CMD_EXPORT(s8_raw_modbus_test, Raw Modbus test);
MSH_CMD_EXPORT(s8_address_test, Test different sensor addresses);
MSH_CMD_EXPORT(s8_hardware_check, Check hardware connections);

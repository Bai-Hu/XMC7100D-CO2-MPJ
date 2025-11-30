/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-22     Developer    S8 CO2 sensor debug tool
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "modbus_rtu.h"

/* S8 Debug tool for testing different register addresses and communication parameters */
static void s8_debug_scan(int argc, char *argv[])
{
    modbus_rtu_device_t *mb_device;
    rt_uint16_t value;
    rt_uint8_t slave_addr = 0xFE;  /* S8 broadcast address */
    int i;
    
    RT_UNUSED(argc);
    RT_UNUSED(argv);
    
    rt_kprintf("[S8_DEBUG] Starting register scan...\n");
    
    /* Initialize Modbus RTU */
    mb_device = modbus_rtu_init("uart2");
    if (!mb_device) {
        rt_kprintf("[S8_DEBUG] Failed to initialize Modbus RTU\n");
        return;
    }
    
    rt_kprintf("[S8_DEBUG] Scanning common S8 register addresses...\n");
    
    /* Test only legal S8 register addresses from manual */
    rt_uint16_t test_addresses[] = {0x0003, 0x0004, 0x0005};  /* Only confirmed legal registers */
    const char* register_names[] = {"CO2 Concentration", "Temperature", "Humidity"};
    
    for (i = 0; i < sizeof(test_addresses) / sizeof(test_addresses[0]); i++) {
        rt_uint16_t addr = test_addresses[i];
        
        rt_kprintf("[S8_DEBUG] Testing input register 0x%04X (%s)...\n", addr, register_names[i]);
        
        /* Try input register read */
        if (modbus_read_input_registers(mb_device, slave_addr, addr, 1, &value) == RT_EOK) {
            rt_kprintf("[S8_DEBUG] SUCCESS - Input Reg 0x%04X: %d (0x%04X)\n", addr, value, value);
        } else {
            rt_kprintf("[S8_DEBUG] FAILED - Input register 0x%04X\n", addr);
            
            /* Try holding register instead */
            if (modbus_read_holding_registers(mb_device, slave_addr, addr, 1, &value) == RT_EOK) {
                rt_kprintf("[S8_DEBUG] SUCCESS - Holding Reg 0x%04X: %d (0x%04X)\n", addr, value, value);
            } else {
                rt_kprintf("[S8_DEBUG] FAILED - Both input and holding register 0x%04X\n", addr);
            }
        }
        
        rt_thread_mdelay(100);  /* Small delay between tests */
    }
    
    /* Test reading multiple registers at once */
    rt_kprintf("[S8_DEBUG] Testing multiple register read (0x0003-0x0005)...\n");
    rt_uint16_t values[3];
    if (modbus_read_input_registers(mb_device, slave_addr, 0x0003, 3, values) == RT_EOK) {
        rt_kprintf("[S8_DEBUG] SUCCESS - Multi-read:\n");
        rt_kprintf("[S8_DEBUG]   Reg 0x0003: %d (CO2)\n", values[0]);
        rt_kprintf("[S8_DEBUG]   Reg 0x0004: %d (Temperature: %.2f°C)\n", values[1], values[1] / 100.0f);
        rt_kprintf("[S8_DEBUG]   Reg 0x0005: %d (Humidity: %.2f%%)\n", values[2], values[2] / 100.0f);
    } else {
        rt_kprintf("[S8_DEBUG] FAILED - Multi-register read\n");
    }
    
    modbus_rtu_deinit(mb_device);
    rt_kprintf("[S8_DEBUG] Register scan completed\n");
}

static void s8_debug_addr(int argc, char *argv[])
{
    modbus_rtu_device_t *mb_device;
    rt_uint16_t value;
    int start_addr, end_addr, i;
    
    if (argc < 3) {
        rt_kprintf("[S8_DEBUG] Usage: s8_debug_addr <start_addr> <end_addr>\n");
        rt_kprintf("[S8_DEBUG] Example: s8_debug_addr 0 255\n");
        return;
    }
    
    start_addr = atoi(argv[1]);
    end_addr = atoi(argv[2]);
    
    rt_kprintf("[S8_DEBUG] Scanning registers 0x%04X to 0x%04X...\n", start_addr, end_addr);
    
    /* Initialize Modbus RTU */
    mb_device = modbus_rtu_init("uart2");
    if (!mb_device) {
        rt_kprintf("[S8_DEBUG] Failed to initialize Modbus RTU\n");
        return;
    }
    
    for (i = start_addr; i <= end_addr; i++) {
        rt_kprintf("[S8_DEBUG] Testing register 0x%04X...\n", i);
        
        if (modbus_read_input_registers(mb_device, 0xFE, i, 1, &value) == RT_EOK) {
            rt_kprintf("[S8_DEBUG] SUCCESS - Input Reg 0x%04X: %d\n", i, value);
        } else if (modbus_read_holding_registers(mb_device, 0xFE, i, 1, &value) == RT_EOK) {
            rt_kprintf("[S8_DEBUG] SUCCESS - Holding Reg 0x%04X: %d\n", i, value);
        } else {
            rt_kprintf("[S8_DEBUG] FAILED - Register 0x%04X\n", i);
        }
        
        rt_thread_mdelay(50);  /* Small delay */
    }
    
    modbus_rtu_deinit(mb_device);
    rt_kprintf("[S8_DEBUG] Address scan completed\n");
}

static void s8_debug_slave(int argc, char *argv[])
{
    modbus_rtu_device_t *mb_device;
    rt_uint16_t value;
    int slave_addr;
    
    if (argc < 2) {
        rt_kprintf("[S8_DEBUG] Usage: s8_debug_slave <slave_addr>\n");
        rt_kprintf("[S8_DEBUG] Example: s8_debug_slave 254\n");
        return;
    }
    
    slave_addr = atoi(argv[1]);
    
    rt_kprintf("[S8_DEBUG] Testing with slave address 0x%02X...\n", slave_addr);
    
    /* Initialize Modbus RTU */
    mb_device = modbus_rtu_init("uart2");
    if (!mb_device) {
        rt_kprintf("[S8_DEBUG] Failed to initialize Modbus RTU\n");
        return;
    }
    
    /* Test with different slave address */
    if (modbus_read_input_registers(mb_device, slave_addr, 0x0003, 1, &value) == RT_EOK) {
        rt_kprintf("[S8_DEBUG] SUCCESS - Slave 0x%02X, Reg 0x0003: %d ppm\n", slave_addr, value);
    } else {
        rt_kprintf("[S8_DEBUG] FAILED - Slave 0x%02X, Reg 0x0003\n", slave_addr);
    }
    
    modbus_rtu_deinit(mb_device);
    rt_kprintf("[S8_DEBUG] Slave address test completed\n");
}

static void s8_debug_help(int argc, char *argv[])
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);
    
    rt_kprintf("[S8_DEBUG] S8 CO2 Sensor Debug Commands:\n");
    rt_kprintf("  s8_debug_scan        - Scan common S8 registers\n");
    rt_kprintf("  s8_debug_addr <start> <end> - Scan register range\n");
    rt_kprintf("  s8_debug_slave <addr>     - Test specific slave address\n");
    rt_kprintf("  s8_debug_help        - Show this help\n");
    rt_kprintf("\nExamples:\n");
    rt_kprintf("  s8_debug_scan           # Scan common registers\n");
    rt_kprintf("  s8_debug_addr 0 10       # Scan registers 0-10\n");
    rt_kprintf("  s8_debug_slave 254       # Test slave address 254\n");
}

/* Export to MSH commands */
MSH_CMD_EXPORT(s8_debug_scan, Scan common S8 registers);
MSH_CMD_EXPORT(s8_debug_addr, Test register address range);
MSH_CMD_EXPORT(s8_debug_slave, Test specific slave address);
MSH_CMD_EXPORT(s8_debug_help, Show S8 debug help);

/* Auto-initialization */
static int s8_debug_init(void)
{
    rt_kprintf("[S8_DEBUG] S8 Debug Tool Initialized\n");
    rt_kprintf("[S8_DEBUG] Type 's8_debug_help' for available commands\n");
    return RT_EOK;
}

INIT_APP_EXPORT(s8_debug_init);

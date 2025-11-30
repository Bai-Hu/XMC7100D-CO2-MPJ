/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-25     Developer    S8 system self-test integration
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "s8_sensor.h"

/**
 * Test CRC calculation algorithm
 */
static rt_err_t s8_test_crc_algorithm(void)
{
    rt_uint8_t test_data[] = {0xFE, 0x04, 0x00, 0x03, 0x00, 0x01};
    rt_uint16_t expected_crc = 0xD5C5;  /* Manual expected: D5 C5 (high byte first) */
    rt_uint16_t calculated_crc;
    
    /* Import CRC function from modbus_rtu */
    extern rt_uint16_t modbus_crc16(rt_uint8_t *data, rt_uint16_t length);
    
    calculated_crc = modbus_crc16(test_data, sizeof(test_data));
    
    if (calculated_crc == expected_crc) {
        return RT_EOK;
    } else {
        rt_kprintf("[S8_SELF_TEST] CRC Test FAILED: expected %04X, got %04X\n", 
                   expected_crc, calculated_crc);
        return -RT_ERROR;
    }
}

/**
 * Test UART2 configuration
 */
static rt_err_t s8_test_uart_configuration(void)
{
    rt_device_t uart2;
    
    /* Find UART2 device */
    uart2 = rt_device_find("uart2");
    if (!uart2) {
        rt_kprintf("[S8_SELF_TEST] UART Test FAILED: Cannot find UART2 device\n");
        return -RT_ERROR;
    }
    
    /* Test device is accessible */
    if (rt_device_open(uart2, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX) != RT_EOK) {
        rt_kprintf("[S8_SELF_TEST] UART Test FAILED: Cannot open UART2\n");
        return -RT_ERROR;
    }
    
    rt_device_close(uart2);
    return RT_EOK;
}

/**
 * Test S8 sensor communication
 */
static rt_err_t s8_test_sensor_communication(void)
{
    s8_sensor_device_t *sensor;
    rt_err_t result;
    
    /* Initialize sensor */
    sensor = s8_sensor_init("uart2");
    if (!sensor) {
        rt_kprintf("[S8_SELF_TEST] Communication Test FAILED: S8 initialization failed\n");
        return -RT_ERROR;
    }
    
    /* Wait for sensor to stabilize */
    rt_thread_mdelay(2000);
    
    /* Test CO2 reading */
    result = s8_read_co2_data(sensor);
    
    /* Cleanup */
    s8_sensor_deinit(sensor);
    
    if (result == RT_EOK) {
        return RT_EOK;
    } else {
        rt_kprintf("[S8_SELF_TEST] Communication Test FAILED: CO2 reading failed (error: %d)\n", result);
        return result;
    }
}

/**
 * Test Modbus protocol layer
 */
static rt_err_t s8_test_modbus_protocol(void)
{
    modbus_rtu_device_t *modbus;
    rt_uint16_t co2_value;
    rt_err_t result;
    
    /* Import modbus functions */
    extern modbus_rtu_device_t* modbus_rtu_init(const char *uart_name);
    extern rt_err_t modbus_rtu_deinit(modbus_rtu_device_t *device);
    extern rt_err_t modbus_read_input_registers(modbus_rtu_device_t *device,
                                            rt_uint8_t slave_addr,
                                            rt_uint16_t start_addr,
                                            rt_uint16_t reg_count,
                                            rt_uint16_t *values);
    
    /* Initialize Modbus */
    modbus = modbus_rtu_init("uart2");
    if (!modbus) {
        rt_kprintf("[S8_SELF_TEST] Modbus Test FAILED: Modbus initialization failed\n");
        return -RT_ERROR;
    }
    
    /* Wait for sensor to stabilize */
    rt_thread_mdelay(2000);
    
    /* Test Modbus communication */
    result = modbus_read_input_registers(modbus, 0xFE, 0x0003, 1, &co2_value);
    
    /* Cleanup */
    modbus_rtu_deinit(modbus);
    
    if (result == RT_EOK) {
        return RT_EOK;
    } else {
        rt_kprintf("[S8_SELF_TEST] Modbus Test FAILED: Register reading failed (error: %d)\n", result);
        return result;
    }
}

/**
 * System self-test for S8 CO2 sensor (verbose mode)
 */
void s8_self_test(void)
{
    rt_err_t crc_result, uart_result, comm_result, modbus_result;
    rt_uint8_t test_count = 0;
    rt_uint8_t pass_count = 0;
    
    rt_kprintf("=== S8 CO2 Sensor System Self-Test ===\n");
    
    /* Test 1: CRC Algorithm */
    rt_kprintf("Test 1: CRC Algorithm... ");
    crc_result = s8_test_crc_algorithm();
    test_count++;
    if (crc_result == RT_EOK) {
        rt_kprintf("PASSED\n");
        pass_count++;
    } else {
        rt_kprintf("FAILED\n");
    }
    
    /* Test 2: UART Configuration */
    rt_kprintf("Test 2: UART Configuration... ");
    uart_result = s8_test_uart_configuration();
    test_count++;
    if (uart_result == RT_EOK) {
        rt_kprintf("PASSED\n");
        pass_count++;
    } else {
        rt_kprintf("FAILED\n");
    }
    
    /* Test 3: S8 Sensor Communication */
    rt_kprintf("Test 3: S8 Sensor Communication... ");
    comm_result = s8_test_sensor_communication();
    test_count++;
    if (comm_result == RT_EOK) {
        rt_kprintf("PASSED\n");
        pass_count++;
    } else {
        rt_kprintf("FAILED\n");
    }
    
    /* Test 4: Modbus Protocol */
    rt_kprintf("Test 4: Modbus Protocol... ");
    modbus_result = s8_test_modbus_protocol();
    test_count++;
    if (modbus_result == RT_EOK) {
        rt_kprintf("PASSED\n");
        pass_count++;
    } else {
        rt_kprintf("FAILED\n");
    }
    
    /* Final Result */
    rt_kprintf("\n=== Self-Test Result ===\n");
    rt_kprintf("Tests: %d/%d passed\n", pass_count, test_count);
    
    if (pass_count == test_count) {
        rt_kprintf("[S8_SELF_TEST] System Self-Check: PASSED\n");
    } else {
        rt_kprintf("[S8_SELF_TEST] System Self-Check: FAILED\n");
        
        /* Report specific failures */
        if (crc_result != RT_EOK) {
            rt_kprintf("  - CRC Algorithm error\n");
        }
        if (uart_result != RT_EOK) {
            rt_kprintf("  - UART Configuration error\n");
        }
        if (comm_result != RT_EOK) {
            rt_kprintf("  - S8 Sensor Communication error\n");
        }
        if (modbus_result != RT_EOK) {
            rt_kprintf("  - Modbus Protocol error\n");
        }
    }
    
    rt_kprintf("========================\n");
}

/**
 * System self-test for S8 CO2 sensor (silent mode - only show failures)
 */
rt_err_t s8_self_test_silent(void)
{
    rt_err_t crc_result, uart_result, comm_result, modbus_result;
    rt_uint8_t test_count = 0;
    rt_uint8_t pass_count = 0;
    rt_bool_t has_failure = RT_FALSE;
    
    /* Run all tests silently */
    crc_result = s8_test_crc_algorithm();
    test_count++;
    if (crc_result == RT_EOK) {
        pass_count++;
    } else {
        has_failure = RT_TRUE;
    }
    
    uart_result = s8_test_uart_configuration();
    test_count++;
    if (uart_result == RT_EOK) {
        pass_count++;
    } else {
        has_failure = RT_TRUE;
    }
    
    comm_result = s8_test_sensor_communication();
    test_count++;
    if (comm_result == RT_EOK) {
        pass_count++;
    } else {
        has_failure = RT_TRUE;
    }
    
    modbus_result = s8_test_modbus_protocol();
    test_count++;
    if (modbus_result == RT_EOK) {
        pass_count++;
    } else {
        has_failure = RT_TRUE;
    }
    
    /* Only show output if there are failures */
    if (has_failure) {
        rt_kprintf("[S8_SELF_TEST] System Self-Check: FAILED\n");
        
        /* Report specific failures */
        if (crc_result != RT_EOK) {
            rt_kprintf("  - CRC Algorithm error\n");
        }
        if (uart_result != RT_EOK) {
            rt_kprintf("  - UART Configuration error\n");
        }
        if (comm_result != RT_EOK) {
            rt_kprintf("  - S8 Sensor Communication error\n");
        }
        if (modbus_result != RT_EOK) {
            rt_kprintf("  - Modbus Protocol error\n");
        }
        rt_kprintf("========================\n");
        
        return -RT_ERROR;
    }
    
    return RT_EOK;
}

/* Export to msh */
MSH_CMD_EXPORT(s8_self_test, S8 CO2 sensor system self-test);

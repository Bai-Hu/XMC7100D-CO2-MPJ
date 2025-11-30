/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-21     Developer    Basic S8 CO2 sensor test
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "s8_sensor.h"
#include "modbus_rtu.h"

/**
 * Basic S8 sensor test function
 */
static void s8_basic_test(int argc, char *argv[])
{
    s8_sensor_device_t *sensor;
    s8_status_t result;
    s8_sensor_data_t data;
    s8_sensor_info_t info;
    
    RT_UNUSED(argc);
    RT_UNUSED(argv);
    
    rt_kprintf("[S8_TEST] Starting basic S8 sensor test...\n");
    
    /* Test 1: Initialize sensor */
    rt_kprintf("[S8_TEST] Test 1: Initializing sensor...\n");
    sensor = s8_sensor_init("uart2");
    if (sensor == RT_NULL) {
        rt_kprintf("[S8_TEST] FAILED: Could not initialize sensor\n");
        return;
    }
    rt_kprintf("[S8_TEST] PASSED: Sensor initialized successfully\n");
    
    /* Test 2: Read sensor info */
    rt_kprintf("[S8_TEST] Test 2: Reading sensor info...\n");
    result = s8_read_sensor_info(sensor, &info);
    if (result == S8_STATUS_OK) {
        rt_kprintf("[S8_TEST] PASSED: Sensor info - Type: 0x%04X, Firmware: %d.%d\n", 
                   info.sensor_type, 
                   (info.firmware_version >> 8) & 0xFF, 
                   info.firmware_version & 0xFF);
    } else {
        rt_kprintf("[S8_TEST] FAILED: Could not read sensor info (error: %d)\n", result);
    }
    
    /* Test 3: Read CO2 data */
    rt_kprintf("[S8_TEST] Test 3: Reading CO2 data...\n");
    result = s8_read_co2_data(sensor);
    if (result == S8_STATUS_OK) {
        rt_kprintf("[S8_TEST] PASSED: CO2: %d ppm\n", sensor->data.co2_ppm);
    } else {
        rt_kprintf("[S8_TEST] FAILED: Could not read CO2 data (error: %d)\n", result);
    }
    
    /* Test 4: Get sensor data */
    rt_kprintf("[S8_TEST] Test 4: Getting sensor data...\n");
    result = s8_get_sensor_data(sensor, &data);
    if (result == S8_STATUS_OK) {
        rt_kprintf("[S8_TEST] PASSED: Got sensor data - CO2: %d ppm, Alarm: %d\n", 
                   data.co2_ppm, data.alarm_state);
    } else {
        rt_kprintf("[S8_TEST] FAILED: Could not get sensor data (error: %d)\n", result);
    }
    
    /* Test 5: Check alarm state */
    rt_kprintf("[S8_TEST] Test 5: Checking alarm state...\n");
    rt_uint8_t alarm_state = s8_get_alarm_state(sensor);
    rt_kprintf("[S8_TEST] PASSED: Alarm state: %d\n", alarm_state);
    
    /* Test 6: Read status */
    rt_kprintf("[S8_TEST] Test 6: Reading sensor status...\n");
    rt_uint16_t status;
    result = s8_read_status(sensor, &status);
    if (result == S8_STATUS_OK) {
        rt_kprintf("[S8_TEST] PASSED: Status register: 0x%04X\n", status);
    } else {
        rt_kprintf("[S8_TEST] FAILED: Could not read status (error: %d)\n", result);
    }
    
    /* Test 7: Cleanup */
    rt_kprintf("[S8_TEST] Test 7: Cleaning up...\n");
    result = s8_sensor_deinit(sensor);
    if (result == RT_EOK) {
        rt_kprintf("[S8_TEST] PASSED: Sensor deinitialized successfully\n");
    } else {
        rt_kprintf("[S8_TEST] FAILED: Could not deinitialize sensor (error: %d)\n", result);
    }
    
    rt_kprintf("[S8_TEST] Basic test completed.\n");
}

/* Export to MSH commands */
MSH_CMD_EXPORT(s8_basic_test, Run basic S8 sensor test);

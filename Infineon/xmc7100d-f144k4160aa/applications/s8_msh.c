/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-21     Developer    MSH commands for S8 CO2 sensor
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "s8_sensor.h"
#include "modbus_rtu.h"

/* Global sensor instance - shared with main.c */
extern s8_sensor_device_t *g_main_s8_device;
static s8_sensor_device_t *g_s8_sensor = RT_NULL;

/**
 * Show help information
 */
static void s8_help(void)
{
    rt_kprintf("S8 CO2 Sensor Commands:\n");
    rt_kprintf("  s8_init               - Initialize S8 sensor\n");
    rt_kprintf("  s8_read               - Read CO2 concentration\n");
    rt_kprintf("  s8_status             - Read sensor status\n");
    rt_kprintf("  s8_monitor [interval]  - Start continuous monitoring\n");
    rt_kprintf("  s8_stop               - Stop continuous monitoring\n");
    rt_kprintf("  s8_calibrate          - Start zero calibration\n");
    rt_kprintf("  s8_reset              - Reset sensor\n");
    rt_kprintf("  s8_info               - Show sensor information\n");
    rt_kprintf("  s8_help               - Show this help\n");
    rt_kprintf("\nExamples:\n");
    rt_kprintf("  s8_init              # Initialize sensor\n");
    rt_kprintf("  s8_read              # Read CO2 value\n");
    rt_kprintf("  s8_monitor 3000      # Start monitoring every 3 seconds\n");
    rt_kprintf("  s8_stop              # Stop monitoring\n");
    rt_kprintf("  s8_calibrate         # Start calibration\n");
}

/**
 * Initialize S8 sensor
 */
static void s8_init(int argc, char *argv[])
{
    RT_UNUSED(argc);
    RT_UNUSED(argv);

    /* Use global sensor from main.c if available */
    if (g_main_s8_device != RT_NULL) {
        g_s8_sensor = g_main_s8_device;
        rt_kprintf("[S8] Using already initialized sensor\n");
        return;
    }

    rt_kprintf("[S8] Initializing sensor...\n");

    /* Initialize sensor with default UART (uart2) */
    g_s8_sensor = s8_sensor_init("uart2");
    if (g_s8_sensor == RT_NULL) {
        rt_kprintf("[S8] Failed to initialize sensor\n");
        return;
    }

    rt_kprintf("[S8] Sensor initialized successfully\n");
}

/**
 * Read CO2 concentration
 */
static void s8_read(int argc, char *argv[])
{
    rt_uint16_t co2_ppm;
    s8_status_t result;

    RT_UNUSED(argc);
    RT_UNUSED(argv);

    /* Auto-detect sensor if not initialized */
    if (g_s8_sensor == RT_NULL && g_main_s8_device != RT_NULL) {
        g_s8_sensor = g_main_s8_device;
        rt_kprintf("[S8] Auto-detected initialized sensor\n");
    }

    if (g_s8_sensor == RT_NULL) {
        rt_kprintf("[S8] Error: Sensor not initialized. Use 's8_init' first\n");
        return;
    }

    result = s8_read_co2_data(g_s8_sensor);
    if (result == S8_STATUS_OK) {
        co2_ppm = g_s8_sensor->data.co2_ppm;
        rt_kprintf("[S8] CO2 Concentration: %d ppm\n", co2_ppm);
    } else {
        rt_kprintf("[S8] Failed to read CO2: %d\n", result);
    }
}

/**
 * Read sensor status
 */
static void s8_status(int argc, char *argv[])
{
    rt_uint16_t status;
    s8_status_t result;

    RT_UNUSED(argc);
    RT_UNUSED(argv);

    /* Auto-detect sensor if not initialized */
    if (g_s8_sensor == RT_NULL && g_main_s8_device != RT_NULL) {
        g_s8_sensor = g_main_s8_device;
        rt_kprintf("[S8] Auto-detected initialized sensor\n");
    }

    if (g_s8_sensor == RT_NULL) {
        rt_kprintf("[S8] Error: Sensor not initialized. Use 's8_init' first\n");
        return;
    }

    result = s8_read_status(g_s8_sensor, &status);
    if (result == S8_STATUS_OK) {
        rt_kprintf("[S8] Status Register: 0x%04X\n", status);
        
        /* Decode status bits */
        if (status & 0x0001) {
            rt_kprintf("  - Calibration ongoing\n");
        }
        if (status & 0x0002) {
            rt_kprintf("  - Warm-up mode\n");
        }
        if (status & 0x0004) {
            rt_kprintf("  - Single point calibration\n");
        }
        if (status & 0x0008) {
            rt_kprintf("  - Output mode active\n");
        }
        if (status & 0x0010) {
            rt_kprintf("  - Measurement active\n");
        }
        if (status & 0x0020) {
            rt_kprintf("  - Alarm threshold 1 exceeded\n");
        }
        if (status & 0x0040) {
            rt_kprintf("  - Alarm threshold 2 exceeded\n");
        }
        if (status & 0x0080) {
            rt_kprintf("  - Sensor error\n");
        }
        if (status & 0x8000) {
            rt_kprintf("  - Software version available\n");
        }
    } else {
        rt_kprintf("[S8] Failed to read status: %d\n", result);
    }
}

/**
 * Start zero calibration
 */
static void s8_calibrate(int argc, char *argv[])
{
    s8_status_t result;

    RT_UNUSED(argc);
    RT_UNUSED(argv);

    /* Auto-detect sensor if not initialized */
    if (g_s8_sensor == RT_NULL && g_main_s8_device != RT_NULL) {
        g_s8_sensor = g_main_s8_device;
        rt_kprintf("[S8] Auto-detected initialized sensor\n");
    }

    if (g_s8_sensor == RT_NULL) {
        rt_kprintf("[S8] Error: Sensor not initialized. Use 's8_init' first\n");
        return;
    }

    rt_kprintf("[S8] Starting zero calibration (this may take several minutes)...\n");
    
    result = s8_zero_calibration(g_s8_sensor);
    if (result == S8_STATUS_OK) {
        rt_kprintf("[S8] Zero calibration started successfully\n");
        rt_kprintf("[S8] Please wait for calibration to complete...\n");
    } else {
        rt_kprintf("[S8] Failed to start calibration: %d\n", result);
    }
}

/**
 * Reset sensor
 */
static void s8_reset(int argc, char *argv[])
{
    rt_err_t result;

    RT_UNUSED(argc);
    RT_UNUSED(argv);

    /* Auto-detect sensor if not initialized */
    if (g_s8_sensor == RT_NULL && g_main_s8_device != RT_NULL) {
        g_s8_sensor = g_main_s8_device;
        rt_kprintf("[S8] Auto-detected initialized sensor\n");
    }

    if (g_s8_sensor == RT_NULL) {
        rt_kprintf("[S8] Error: Sensor not initialized. Use 's8_init' first\n");
        return;
    }

    rt_kprintf("[S8] Resetting sensor...\n");
    
    result = s8_trigger_calibration(g_s8_sensor);
    if (result == RT_EOK) {
        rt_kprintf("[S8] Sensor reset successfully\n");
        rt_kprintf("[S8] Sensor will restart automatically\n");
    } else {
        rt_kprintf("[S8] Failed to reset sensor: %d\n", result);
    }
}

/**
 * Start continuous monitoring
 */
static void s8_monitor(int argc, char *argv[])
{
    rt_uint32_t interval_ms = 5000;  /* Default 5 seconds */
    rt_err_t result;

    /* Auto-detect sensor if not initialized */
    if (g_s8_sensor == RT_NULL && g_main_s8_device != RT_NULL) {
        g_s8_sensor = g_main_s8_device;
        rt_kprintf("[S8] Auto-detected initialized sensor\n");
    }

    if (g_s8_sensor == RT_NULL) {
        rt_kprintf("[S8] Error: Sensor not initialized. Use 's8_init' first\n");
        return;
    }

    /* Parse interval parameter if provided */
    if (argc > 1) {
        interval_ms = atoi(argv[1]);
        if (interval_ms < 1000) {
            rt_kprintf("[S8] Warning: Interval too short, using minimum 1000ms\n");
            interval_ms = 1000;
        } else if (interval_ms > 60000) {
            rt_kprintf("[S8] Warning: Interval too long, using maximum 60000ms\n");
            interval_ms = 60000;
        }
    }

    rt_kprintf("[S8] Starting continuous monitoring with %d ms interval...\n", interval_ms);

    result = s8_start_monitoring(g_s8_sensor, interval_ms);
    if (result == RT_EOK) {
        rt_kprintf("[S8] Monitoring started successfully\n");
        rt_kprintf("[S8] Use 's8_stop' to stop monitoring\n");
    } else if (result == -RT_EBUSY) {
        rt_kprintf("[S8] Monitoring already running. Use 's8_stop' first\n");
    } else {
        rt_kprintf("[S8] Failed to start monitoring: %d\n", result);
    }
}

/**
 * Stop continuous monitoring
 */
static void s8_stop(int argc, char *argv[])
{
    rt_err_t result;

    RT_UNUSED(argc);
    RT_UNUSED(argv);

    /* Auto-detect sensor if not initialized */
    if (g_s8_sensor == RT_NULL && g_main_s8_device != RT_NULL) {
        g_s8_sensor = g_main_s8_device;
        rt_kprintf("[S8] Auto-detected initialized sensor\n");
    }

    if (g_s8_sensor == RT_NULL) {
        rt_kprintf("[S8] Error: Sensor not initialized. Use 's8_init' first\n");
        return;
    }

    rt_kprintf("[S8] Stopping continuous monitoring...\n");

    result = s8_stop_monitoring(g_s8_sensor);
    if (result == RT_EOK) {
        rt_kprintf("[S8] Monitoring stopped successfully\n");
    } else {
        rt_kprintf("[S8] Failed to stop monitoring: %d\n", result);
    }
}

/**
 * Show sensor information
 */
static void s8_info(int argc, char *argv[])
{
    s8_sensor_info_t info;
    s8_status_t result;

    RT_UNUSED(argc);
    RT_UNUSED(argv);

    /* Auto-detect sensor if not initialized */
    if (g_s8_sensor == RT_NULL && g_main_s8_device != RT_NULL) {
        g_s8_sensor = g_main_s8_device;
        rt_kprintf("[S8] Auto-detected initialized sensor\n");
    }

    if (g_s8_sensor == RT_NULL) {
        rt_kprintf("[S8] Error: Sensor not initialized. Use 's8_init' first\n");
        return;
    }

    result = s8_read_sensor_info(g_s8_sensor, &info);
    if (result == S8_STATUS_OK) {
        rt_kprintf("[S8] Sensor Information:\n");
        rt_kprintf("  Type: 0x%04X\n", info.sensor_type);
        rt_kprintf("  Firmware Version: %d.%d\n", 
                   (info.firmware_version >> 8) & 0xFF, 
                   info.firmware_version & 0xFF);
    } else {
        rt_kprintf("[S8] Failed to read sensor info: %d\n", result);
    }
}

/**
 * Initialize S8 MSH commands
 */
int s8_msh_init(void)
{
    rt_kprintf("[S8] S8 CO2 Sensor MSH Commands Loaded\n");
    rt_kprintf("[S8] Type 's8_help' for available commands\n");
    
    /* Check if sensor is already initialized from main.c */
    if (g_main_s8_device != RT_NULL) {
        g_s8_sensor = g_main_s8_device;
        rt_kprintf("[S8] Auto-detected initialized sensor from main.c\n");
    }
    
    return RT_EOK;
}

/* Export to MSH commands */
MSH_CMD_EXPORT(s8_init, Initialize S8 CO2 sensor);
MSH_CMD_EXPORT(s8_read, Read CO2 concentration);
MSH_CMD_EXPORT(s8_status, Read sensor status);
MSH_CMD_EXPORT(s8_monitor, Start continuous monitoring);
MSH_CMD_EXPORT(s8_stop, Stop continuous monitoring);
MSH_CMD_EXPORT(s8_calibrate, Start zero calibration);
MSH_CMD_EXPORT(s8_reset, Reset sensor);
MSH_CMD_EXPORT(s8_info, Show sensor information);
MSH_CMD_EXPORT(s8_help, Show S8 sensor command help);

/* Auto-initialization - use lower priority to run after main() */
INIT_APP_EXPORT(s8_msh_init);

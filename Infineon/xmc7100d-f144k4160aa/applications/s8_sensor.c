/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-21     Developer    S8 CO2 sensor driver
 */

#include "s8_sensor.h"
#include "drv_gpio.h"

/* Global sensor device for MSH commands */
static s8_sensor_device_t *g_s8_device = RT_NULL;

/* Monitor thread function */
static void s8_monitor_thread_entry(void *parameter)
{
    s8_sensor_device_t *device = (s8_sensor_device_t *)parameter;
    s8_status_t result;
    
    
    while (device->running) {
        result = s8_read_co2_data(device);
        
        if (result == S8_STATUS_OK) {
            rt_kprintf("[S8] CO2: %d ppm\n", device->data.co2_ppm);
        } else {
            rt_kprintf("[S8] Read error: %d\n", result);
        }
        
        /* Use time-slicing approach for better MSH responsiveness */
        for (int i = 0; i < (device->read_interval_ms / 50) && device->running; i++) {
            rt_thread_mdelay(50);   /* 50ms short delay */
            rt_thread_yield();       /* Yield CPU to allow MSH thread to run */
        }
    }
    
}

/**
 * Initialize S8 sensor device
 */
s8_sensor_device_t* s8_sensor_init(const char *uart_name)
{
    s8_sensor_device_t *device;

    if (!uart_name) {
        return RT_NULL;
    }

    device = (s8_sensor_device_t*)rt_malloc(sizeof(s8_sensor_device_t));
    if (!device) {
        return RT_NULL;
    }

    rt_memset(device, 0, sizeof(s8_sensor_device_t));

    /* Initialize Modbus RTU device */
    device->modbus = modbus_rtu_init(uart_name);
    if (!device->modbus) {
        rt_free(device);
        return RT_NULL;
    }

    /* Initialize GPIO pins */
    rt_pin_mode(S8_ALARM_PIN, PIN_MODE_INPUT);
    rt_pin_mode(S8_UART_RXT_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(S8_BCAL_PIN, PIN_MODE_OUTPUT);

    /* Set initial GPIO states */
    rt_pin_write(S8_UART_RXT_PIN, PIN_HIGH);  /* Enable receive mode */
    rt_pin_write(S8_BCAL_PIN, PIN_LOW);       /* Normal operation */

    /* Initialize data structure */
    device->data.co2_ppm = 0;
    device->data.alarm_state = 0;
    device->data.timestamp = 0;
    device->data.data_valid = RT_FALSE;

    device->running = RT_FALSE;
    device->read_interval_ms = 5000;  /* Default 5 seconds */

    return device;
}

/**
 * Deinitialize S8 sensor device
 */
rt_err_t s8_sensor_deinit(s8_sensor_device_t *device)
{
    if (!device) {
        return -RT_ERROR;
    }

    /* Stop monitoring if running */
    s8_stop_monitoring(device);

    /* Deinitialize Modbus device */
    if (device->modbus) {
        modbus_rtu_deinit(device->modbus);
    }

    rt_free(device);
    return RT_EOK;
}

/**
 * Read CO2 data from S8 sensor
 */
s8_status_t s8_read_co2_data(s8_sensor_device_t *device)
{
    rt_uint16_t co2_value;
    rt_err_t result;

    if (!device || !device->modbus) {
        return S8_STATUS_NOT_INITIALIZED;
    }

    /* Read CO2 concentration from input register 0x0003 using broadcast address 0xFE */
    result = modbus_read_input_registers(device->modbus,
                                          0xFE,     /* Broadcast address */
                                          S8_REG_CO2_CONCENTRATION,
                                          1,         /* Read 1 register */
                                          &co2_value);

    if (result != RT_EOK) {
        return (result == -RT_ETIMEOUT) ? S8_STATUS_TIMEOUT : S8_STATUS_ERROR;
    }

    /* Update sensor data */
    device->data.co2_ppm = co2_value;
    device->data.alarm_state = s8_get_alarm_state(device);
    device->data.timestamp = rt_tick_get();
    device->data.data_valid = RT_TRUE;

    return S8_STATUS_OK;
}

/**
 * Read all sensor data (CO2 only)
 */
s8_status_t s8_read_all_data(s8_sensor_device_t *device, s8_sensor_data_t *data)
{
    rt_err_t result;

    if (!device || !device->modbus || !data) {
        return S8_STATUS_NOT_INITIALIZED;
    }

    /* Read CO2 data */
    result = s8_read_co2_data(device);
    if (result != S8_STATUS_OK) {
        return result;
    }

    /* Copy to output */
    *data = device->data;

    rt_kprintf("[S8] All data - CO2: %d ppm\n", device->data.co2_ppm);

    return S8_STATUS_OK;
}

/**
 * Read sensor information
 */
s8_status_t s8_read_sensor_info(s8_sensor_device_t *device, s8_sensor_info_t *info)
{
    rt_uint16_t type_high, type_low, firmware;
    rt_err_t result;

    if (!device || !device->modbus || !info) {
        return S8_STATUS_NOT_INITIALIZED;
    }

    /* Read sensor type ID high (IR26) */
    result = modbus_read_input_registers(device->modbus,
                                          0xFE,
                                          S8_REG_SENSOR_TYPE_HIGH,
                                          1,
                                          &type_high);
    if (result != RT_EOK) {
        return (result == -RT_ETIMEOUT) ? S8_STATUS_TIMEOUT : S8_STATUS_ERROR;
    }

    /* Read sensor type ID low (IR27) */
    result = modbus_read_input_registers(device->modbus,
                                          0xFE,
                                          S8_REG_SENSOR_TYPE_LOW,
                                          1,
                                          &type_low);
    if (result != RT_EOK) {
        return (result == -RT_ETIMEOUT) ? S8_STATUS_TIMEOUT : S8_STATUS_ERROR;
    }

    /* Read firmware version (IR29) */
    result = modbus_read_input_registers(device->modbus,
                                          0xFE,
                                          S8_REG_FIRMWARE_VERSION,
                                          1,
                                          &firmware);
    if (result != RT_EOK) {
        return (result == -RT_ETIMEOUT) ? S8_STATUS_TIMEOUT : S8_STATUS_ERROR;
    }

    /* Combine sensor type (high << 16 | low) but we only use high for now */
    info->sensor_type = type_high;
    info->firmware_version = firmware;

    rt_kprintf("[S8] Sensor info - Type: 0x%04X%04X, Firmware: %d\n", 
               type_high, type_low, firmware);

    return S8_STATUS_OK;
}

/**
 * Read sensor status
 */
s8_status_t s8_read_status(s8_sensor_device_t *device, rt_uint16_t *status)
{
    rt_uint16_t status_value;
    rt_err_t result;

    if (!device || !device->modbus || !status) {
        return S8_STATUS_NOT_INITIALIZED;
    }

    /* Read status from Input Register IR1 (0x0000) using function 0x04 */
    result = modbus_read_input_registers(device->modbus,
                                          0xFE,     /* Broadcast address */
                                          0x0000,   /* MeterStatus register IR1 */
                                          1,         /* Read 1 register */
                                          &status_value);

    if (result != RT_EOK) {
        return (result == -RT_ETIMEOUT) ? S8_STATUS_TIMEOUT : S8_STATUS_ERROR;
    }

    *status = status_value;
    return S8_STATUS_OK;
}

/**
 * Read sensor type
 */
s8_status_t s8_read_sensor_type(s8_sensor_device_t *device, rt_uint16_t *sensor_type)
{
    s8_sensor_info_t info;
    s8_status_t result = s8_read_sensor_info(device, &info);
    
    if (result == S8_STATUS_OK) {
        *sensor_type = info.sensor_type;
    }
    
    return result;
}

/**
 * Read firmware version
 */
s8_status_t s8_read_firmware_version(s8_sensor_device_t *device, rt_uint16_t *firmware_version)
{
    s8_sensor_info_t info;
    s8_status_t result = s8_read_sensor_info(device, &info);
    
    if (result == S8_STATUS_OK) {
        *firmware_version = info.firmware_version;
    }
    
    return result;
}

/**
 * Single point calibration
 */
s8_status_t s8_single_point_calibration(s8_sensor_device_t *device, rt_uint16_t ppm_value)
{
    rt_err_t result;

    if (!device || !device->modbus) {
        return S8_STATUS_NOT_INITIALIZED;
    }

    /* Write calibration value to holding register */
    result = modbus_write_single_register(device->modbus,
                                           0xFE,
                                           S8_REG_SINGLE_POINT_CAL,
                                           ppm_value);

    if (result != RT_EOK) {
        return (result == -RT_ETIMEOUT) ? S8_STATUS_TIMEOUT : S8_STATUS_ERROR;
    }

    rt_kprintf("[S8] Single point calibration set to %d ppm\n", ppm_value);
    return S8_STATUS_OK;
}

/**
 * Background calibration
 */
s8_status_t s8_background_calibration(s8_sensor_device_t *device)
{
    rt_err_t result;

    if (!device || !device->modbus) {
        return S8_STATUS_NOT_INITIALIZED;
    }

    /* Start background calibration */
    result = modbus_write_single_register(device->modbus,
                                           0xFE,
                                           S8_REG_BACKGROUND_CAL,
                                           S8_CAL_COMMAND_START);

    if (result != RT_EOK) {
        return (result == -RT_ETIMEOUT) ? S8_STATUS_TIMEOUT : S8_STATUS_ERROR;
    }

    rt_kprintf("[S8] Background calibration started\n");
    return S8_STATUS_OK;
}

/**
 * Zero calibration
 */
s8_status_t s8_zero_calibration(s8_sensor_device_t *device)
{
    rt_err_t result;

    if (!device || !device->modbus) {
        return S8_STATUS_NOT_INITIALIZED;
    }

    /* Start zero calibration */
    result = modbus_write_single_register(device->modbus,
                                           0xFE,
                                           S8_REG_ZERO_CAL,
                                           S8_CAL_COMMAND_START);

    if (result != RT_EOK) {
        return (result == -RT_ETIMEOUT) ? S8_STATUS_TIMEOUT : S8_STATUS_ERROR;
    }

    rt_kprintf("[S8] Zero calibration started\n");
    return S8_STATUS_OK;
}

/**
 * Set auto calibration
 */
s8_status_t s8_set_auto_calibration(s8_sensor_device_t *device, rt_bool_t enable)
{
    rt_err_t result;

    if (!device || !device->modbus) {
        return S8_STATUS_NOT_INITIALIZED;
    }

    /* Enable or disable auto calibration */
    result = modbus_write_single_register(device->modbus,
                                           0xFE,
                                           S8_REG_AUTO_CAL,
                                           enable ? S8_CAL_COMMAND_START : S8_CAL_COMMAND_STOP);

    if (result != RT_EOK) {
        return (result == -RT_ETIMEOUT) ? S8_STATUS_TIMEOUT : S8_STATUS_ERROR;
    }

    rt_kprintf("[S8] Auto calibration %s\n", enable ? "enabled" : "disabled");
    return S8_STATUS_OK;
}

/**
 * Set alarm threshold
 */
s8_status_t s8_set_alarm_threshold(s8_sensor_device_t *device, rt_uint16_t threshold_ppm)
{
    rt_err_t result;

    if (!device || !device->modbus) {
        return S8_STATUS_NOT_INITIALIZED;
    }

    /* Set alarm threshold */
    result = modbus_write_single_register(device->modbus,
                                           0xFE,
                                           S8_REG_ALARM_THRESHOLD,
                                           threshold_ppm);

    if (result != RT_EOK) {
        return (result == -RT_ETIMEOUT) ? S8_STATUS_TIMEOUT : S8_STATUS_ERROR;
    }

    rt_kprintf("[S8] Alarm threshold set to %d ppm\n", threshold_ppm);
    return S8_STATUS_OK;
}

/**
 * Get sensor data
 */
s8_status_t s8_get_sensor_data(s8_sensor_device_t *device, s8_sensor_data_t *data)
{
    if (!device || !data) {
        return S8_STATUS_ERROR;
    }

    if (!device->data.data_valid) {
        return S8_STATUS_INVALID_DATA;
    }

    *data = device->data;
    return S8_STATUS_OK;
}

/**
 * Start monitoring thread
 */
rt_err_t s8_start_monitoring(s8_sensor_device_t *device, rt_uint32_t interval_ms)
{
    if (!device) {
        return -RT_ERROR;
    }

    if (device->running) {
        return -RT_EBUSY;
    }

    device->read_interval_ms = interval_ms;
    device->running = RT_TRUE;

    /* Create monitor thread with lower priority and time-slicing */
    device->monitor_thread = rt_thread_create("s8_monitor",
                                            s8_monitor_thread_entry,
                                            device,
                                            1024,
                                            30,    /* Lower priority than MSH (which is typically 20) */
                                            10);   /* Time slice for responsive scheduling */

    if (!device->monitor_thread) {
        device->running = RT_FALSE;
        return -RT_ERROR;
    }

    rt_thread_startup(device->monitor_thread);
    rt_kprintf("[S8] Monitoring started, interval: %d ms\n", interval_ms);

    return RT_EOK;
}

/**
 * Stop monitoring thread
 */
rt_err_t s8_stop_monitoring(s8_sensor_device_t *device)
{
    if (!device) {
        return -RT_ERROR;
    }

    if (!device->running) {
        return RT_EOK;
    }

    device->running = RT_FALSE;

    if (device->monitor_thread) {
        rt_thread_delete(device->monitor_thread);
        device->monitor_thread = RT_NULL;
    }

    rt_kprintf("[S8] Monitoring stopped\n");
    return RT_EOK;
}

/**
 * Trigger calibration
 */
rt_err_t s8_trigger_calibration(s8_sensor_device_t *device)
{
    if (!device) {
        return -RT_ERROR;
    }

    /* Trigger calibration by pulsing bCAL pin */
    rt_pin_write(S8_BCAL_PIN, PIN_HIGH);
    rt_thread_mdelay(100);
    rt_pin_write(S8_BCAL_PIN, PIN_LOW);

    rt_kprintf("[S8] Calibration triggered via bCAL pin\n");
    return RT_EOK;
}

/**
 * Get alarm state
 */
rt_uint8_t s8_get_alarm_state(s8_sensor_device_t *device)
{
    if (!device) {
        return 0;
    }

    return rt_pin_read(S8_ALARM_PIN) ? 1 : 0;
}

/**
 * Check if data is valid
 */
rt_bool_t s8_is_data_valid(s8_sensor_device_t *device)
{
    if (!device) {
        return RT_FALSE;
    }

    return device->data.data_valid;
}

/**
 * Get data age in milliseconds
 */
rt_uint32_t s8_get_data_age(s8_sensor_device_t *device)
{
    if (!device || !device->data.data_valid) {
        return 0xFFFFFFFF;
    }

    return rt_tick_get() - device->data.timestamp;
}

/**
 * Convert raw CO2 value to ppm
 */
float s8_co2_to_ppm(rt_uint16_t raw_value)
{
    return (float)raw_value;
}

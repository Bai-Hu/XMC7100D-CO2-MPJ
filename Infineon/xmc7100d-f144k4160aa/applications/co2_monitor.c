/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-21     Developer    CO2 monitoring application
 */

#include "co2_monitor.h"

/* Monitor thread function */
static void co2_monitor_thread_entry(void *parameter)
{
    co2_monitor_t *monitor = (co2_monitor_t *)parameter;
    s8_status_t status;
    s8_sensor_data_t sensor_data;

    while (monitor->running) {
        /* Read CO2 data from sensor */
        status = s8_read_co2_data(monitor->sensor);
        if (status == S8_STATUS_OK) {
            /* Get sensor data */
            if (s8_get_sensor_data(monitor->sensor, &sensor_data) == S8_STATUS_OK) {
                monitor->current_data = sensor_data;
                
                float co2_ppm = s8_co2_to_ppm(sensor_data.co2_ppm);
                rt_kprintf("[CO2] CO2: %.2f ppm, Alarm: %d\n", 
                           co2_ppm, sensor_data.alarm_state);
                
                /* Check alarm threshold */
                if (monitor->alarm_threshold > 0 && co2_ppm > monitor->alarm_threshold) {
                    rt_kprintf("[CO2] WARNING: CO2 level above threshold (%d ppm)\n", 
                               monitor->alarm_threshold);
                }
            }
        } else {
            rt_kprintf("[CO2] Failed to read sensor data: %d\n", status);
        }

        /* Use time-slicing approach - short delays with yield to allow MSH responsiveness */
        for (int i = 0; i < (monitor->read_interval_ms / 50) && monitor->running; i++) {
            rt_thread_mdelay(50);   /* 50ms short delay */
            rt_thread_yield();       /* Yield CPU to allow MSH thread to run */
        }
    }
}

/**
 * Initialize CO2 monitor
 */
co2_monitor_t* co2_monitor_init(void)
{
    co2_monitor_t *monitor;

    monitor = (co2_monitor_t*)rt_malloc(sizeof(co2_monitor_t));
    if (!monitor) {
        return RT_NULL;
    }

    rt_memset(monitor, 0, sizeof(co2_monitor_t));
    monitor->read_interval_ms = 5000;  /* Default 5 seconds */
    monitor->alarm_threshold = 1000;   /* Default 1000 ppm */
    
    rt_kprintf("[CO2] CO2 monitor initialized\n");
    return monitor;
}

/**
 * Deinitialize CO2 monitor
 */
rt_err_t co2_monitor_deinit(co2_monitor_t *monitor)
{
    if (!monitor) {
        return -RT_ERROR;
    }

    /* Stop monitoring if running */
    co2_monitor_stop(monitor);

    rt_free(monitor);
    rt_kprintf("[CO2] CO2 monitor deinitialized\n");
    return RT_EOK;
}

/**
 * Set sensor for CO2 monitor
 */
rt_err_t co2_monitor_set_sensor(co2_monitor_t *monitor, s8_sensor_device_t *sensor)
{
    if (!monitor || !sensor) {
        return -RT_ERROR;
    }

    monitor->sensor = sensor;
    rt_kprintf("[CO2] Sensor set for monitor\n");
    return RT_EOK;
}

/**
 * Start CO2 monitoring
 */
rt_err_t co2_monitor_start(co2_monitor_t *monitor, rt_uint32_t interval_ms)
{
    if (!monitor || !monitor->sensor) {
        return -RT_ERROR;
    }

    if (monitor->running) {
        return -RT_EBUSY;
    }

    monitor->read_interval_ms = interval_ms;
    monitor->running = RT_TRUE;

    /* Create monitor thread with lower priority and time-slicing */
    monitor->monitor_thread = rt_thread_create("co2_monitor",
                                          co2_monitor_thread_entry,
                                          monitor,
                                          2048,
                                          30,    /* Lower priority than MSH (which is typically 20) */
                                          10);   /* Time slice for responsive scheduling */

    if (!monitor->monitor_thread) {
        monitor->running = RT_FALSE;
        return -RT_ERROR;
    }

    rt_thread_startup(monitor->monitor_thread);
    rt_kprintf("[CO2] Monitoring started, interval: %d ms\n", interval_ms);

    return RT_EOK;
}

/**
 * Stop CO2 monitoring
 */
rt_err_t co2_monitor_stop(co2_monitor_t *monitor)
{
    if (!monitor) {
        return -RT_ERROR;
    }

    if (!monitor->running) {
        return RT_EOK;
    }

    monitor->running = RT_FALSE;

    if (monitor->monitor_thread) {
        rt_thread_delete(monitor->monitor_thread);
        monitor->monitor_thread = RT_NULL;
    }

    rt_kprintf("[CO2] Monitoring stopped\n");
    return RT_EOK;
}

/**
 * Get current CO2 data
 */
rt_err_t co2_monitor_get_data(co2_monitor_t *monitor, s8_sensor_data_t *data)
{
    if (!monitor || !data) {
        return -RT_ERROR;
    }

    *data = monitor->current_data;
    return RT_EOK;
}

/**
 * Set alarm threshold
 */
rt_err_t co2_monitor_set_alarm_threshold(co2_monitor_t *monitor, rt_uint16_t threshold_ppm)
{
    if (!monitor) {
        return -RT_ERROR;
    }

    monitor->alarm_threshold = threshold_ppm;
    rt_kprintf("[CO2] Alarm threshold set to %d ppm\n", threshold_ppm);
    return RT_EOK;
}

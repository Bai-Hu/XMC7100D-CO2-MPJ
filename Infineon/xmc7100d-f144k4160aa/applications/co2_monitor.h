/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-21     Developer    CO2 monitoring application header
 */

#ifndef CO2_MONITOR_H__
#define CO2_MONITOR_H__

#include <rtthread.h>
#include <rtdevice.h>
#include "s8_sensor.h"

/* CO2 monitor structure */
typedef struct {
    s8_sensor_device_t *sensor;     /* S8 sensor device */
    s8_sensor_data_t current_data;   /* Current sensor data */
    rt_thread_t monitor_thread;      /* Monitor thread */
    rt_uint32_t read_interval_ms;   /* Read interval in milliseconds */
    rt_bool_t running;               /* Thread running flag */
    rt_uint16_t alarm_threshold;    /* CO2 alarm threshold in ppm */
} co2_monitor_t;

/* Function declarations */
co2_monitor_t* co2_monitor_init(void);
rt_err_t co2_monitor_deinit(co2_monitor_t *monitor);
rt_err_t co2_monitor_set_sensor(co2_monitor_t *monitor, s8_sensor_device_t *sensor);
rt_err_t co2_monitor_start(co2_monitor_t *monitor, rt_uint32_t interval_ms);
rt_err_t co2_monitor_stop(co2_monitor_t *monitor);
rt_err_t co2_monitor_get_data(co2_monitor_t *monitor, s8_sensor_data_t *data);
rt_err_t co2_monitor_set_alarm_threshold(co2_monitor_t *monitor, rt_uint16_t threshold_ppm);

#endif /* CO2_MONITOR_H__ */

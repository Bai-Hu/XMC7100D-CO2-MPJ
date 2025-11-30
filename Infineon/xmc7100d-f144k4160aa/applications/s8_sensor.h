/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-21     Developer    S8 CO2 sensor driver
 */

#ifndef S8_SENSOR_H__
#define S8_SENSOR_H__

#include <rtthread.h>
#include <rtdevice.h>
#include "modbus_rtu.h"

/* GPIO pin definitions for S8 sensor */
#define S8_ALARM_PIN        GET_PIN(19, 3)    /* P19_3 (IO2) - Alarm output */
#define S8_UART_RXT_PIN     GET_PIN(20, 1)    /* P20_1 (IO5) - UART R/T control */
#define S8_BCAL_PIN         GET_PIN(20, 2)    /* P20_2 (IO6) - Calibration input */

/* S8 Modbus register addresses */
#define S8_REG_CO2_CONCENTRATION    0x0003  /* CO2 concentration (input register) */
#define S8_REG_METER_STATUS        0x0000  /* Meter status (input register IR1) */
#define S8_REG_ALARM_STATUS        0x0001  /* Alarm status (input register IR2) */
#define S8_REG_OUTPUT_STATUS       0x0002  /* Output status (input register IR3) */
#define S8_REG_SENSOR_TYPE_HIGH    0x0019  /* Sensor type ID high (input register IR26) */
#define S8_REG_SENSOR_TYPE_LOW     0x001A  /* Sensor type ID low (input register IR27) */
#define S8_REG_FIRMWARE_VERSION   0x001C  /* Firmware version (input register IR29) */
#define S8_REG_SINGLE_POINT_CAL  0x0010  /* Single point calibration (holding register) */
#define S8_REG_BACKGROUND_CAL    0x0011  /* Background calibration (holding register) */
#define S8_REG_ZERO_CAL         0x0012  /* Zero calibration (holding register) */
#define S8_REG_AUTO_CAL         0x0013  /* Auto calibration (holding register) */
#define S8_REG_ALARM_THRESHOLD   0x0014  /* Alarm threshold (holding register) */

/* S8 calibration commands */
#define S8_CAL_COMMAND_START      0x0001
#define S8_CAL_COMMAND_STOP       0x0000

/* S8 sensor info structure */
typedef struct {
    rt_uint16_t sensor_type;      /* Sensor type identifier */
    rt_uint16_t firmware_version;  /* Firmware version */
} s8_sensor_info_t;

/* S8 sensor data structure */
typedef struct {
    rt_uint16_t co2_ppm;        /* CO2 concentration in ppm */
    rt_uint8_t  alarm_state;     /* Alarm state (0=normal, 1=alarm) */
    rt_uint32_t timestamp;       /* Last update timestamp */
    rt_bool_t   data_valid;      /* Data validity flag */
} s8_sensor_data_t;

/* S8 sensor device structure */
typedef struct {
    modbus_rtu_device_t *modbus;     /* Modbus RTU device */
    s8_sensor_data_t data;           /* Sensor data */
    rt_thread_t monitor_thread;      /* Monitor thread */
    rt_uint32_t read_interval_ms;   /* Read interval in milliseconds */
    rt_bool_t running;               /* Thread running flag */
} s8_sensor_device_t;

/* S8 sensor status codes */
typedef enum {
    S8_STATUS_OK = 0,
    S8_STATUS_ERROR = -1,
    S8_STATUS_TIMEOUT = -2,
    S8_STATUS_INVALID_DATA = -3,
    S8_STATUS_NOT_INITIALIZED = -4
} s8_status_t;

/* Function declarations */

/* Device management */
s8_sensor_device_t* s8_sensor_init(const char *uart_name);
rt_err_t s8_sensor_deinit(s8_sensor_device_t *device);

/* Data reading */
s8_status_t s8_read_co2_data(s8_sensor_device_t *device);
s8_status_t s8_read_all_data(s8_sensor_device_t *device, s8_sensor_data_t *data);
s8_status_t s8_get_sensor_data(s8_sensor_device_t *device, s8_sensor_data_t *data);

/* Sensor information */
s8_status_t s8_read_sensor_info(s8_sensor_device_t *device, s8_sensor_info_t *info);
s8_status_t s8_read_sensor_type(s8_sensor_device_t *device, rt_uint16_t *sensor_type);
s8_status_t s8_read_firmware_version(s8_sensor_device_t *device, rt_uint16_t *firmware_version);
s8_status_t s8_read_status(s8_sensor_device_t *device, rt_uint16_t *status);

/* Calibration functions */
rt_err_t s8_trigger_calibration(s8_sensor_device_t *device);
s8_status_t s8_single_point_calibration(s8_sensor_device_t *device, rt_uint16_t ppm_value);
s8_status_t s8_background_calibration(s8_sensor_device_t *device);
s8_status_t s8_zero_calibration(s8_sensor_device_t *device);
s8_status_t s8_set_auto_calibration(s8_sensor_device_t *device, rt_bool_t enable);

/* Control functions */
s8_status_t s8_set_alarm_threshold(s8_sensor_device_t *device, rt_uint16_t threshold_ppm);
rt_uint8_t s8_get_alarm_state(s8_sensor_device_t *device);

/* Monitoring */
rt_err_t s8_start_monitoring(s8_sensor_device_t *device, rt_uint32_t interval_ms);
rt_err_t s8_stop_monitoring(s8_sensor_device_t *device);

/* Utility functions */
rt_bool_t s8_is_data_valid(s8_sensor_device_t *device);
rt_uint32_t s8_get_data_age(s8_sensor_device_t *device);

/* Data conversion helpers */
float s8_co2_to_ppm(rt_uint16_t raw_value);

/* MSH command functions */
#ifdef RT_USING_FINSH
#include <finsh.h>
int s8_msh_init(void);
void s8_msh_read_co2(int argc, char **argv);
void s8_msh_read_all(int argc, char **argv);
void s8_msh_read_info(int argc, char **argv);
void s8_msh_cal_point(int argc, char **argv);
void s8_msh_cal_background(int argc, char **argv);
void s8_msh_cal_zero(int argc, char **argv);
void s8_msh_set_auto_cal(int argc, char **argv);
void s8_msh_set_alarm(int argc, char **argv);
void s8_msh_get_status(int argc, char **argv);
void s8_msh_start_monitor(int argc, char **argv);
void s8_msh_stop_monitor(int argc, char **argv);
#endif

#endif /* S8_SENSOR_H__ */

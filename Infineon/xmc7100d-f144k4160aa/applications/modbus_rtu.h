/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-21     Developer    Modbus RTU protocol for S8 CO2 sensor
 */

#ifndef MODBUS_RTU_H__
#define MODBUS_RTU_H__

#include <rtthread.h>
#include <rtdevice.h>

/* Modbus function codes */
#define MODBUS_FUNC_READ_HOLDING_REGS    0x03
#define MODBUS_FUNC_READ_INPUT_REGS     0x04
#define MODBUS_FUNC_WRITE_SINGLE_REG     0x06

/* Modbus constants */
#define MODBUS_MAX_BUFFER_SIZE           256
#define MODBUS_TIMEOUT_MS               1000

/* S8 Sensor specific constants */
#define S8_MODBUS_ADDRESS               0xFE    /* Broadcast address */
#define S8_CO2_REG_ADDR                 0x0003  /* CO2 concentration register (input register) */

/* Modbus RTU frame structure */
typedef struct {
    rt_uint8_t slave_addr;
    rt_uint8_t function_code;
    rt_uint16_t start_addr;
    rt_uint16_t reg_count;
    rt_uint8_t *data;
    rt_uint16_t data_length;
} modbus_request_t;

typedef struct {
    rt_uint8_t slave_addr;
    rt_uint8_t function_code;
    rt_uint8_t byte_count;
    rt_uint8_t *data;
    rt_uint16_t crc;
} modbus_response_t;

/* Modbus RTU device structure */
typedef struct {
    struct rt_serial_device *serial;
    rt_uint8_t rx_buffer[MODBUS_MAX_BUFFER_SIZE];
    rt_uint16_t rx_index;
    rt_uint32_t timeout_tick;
    rt_mutex_t lock;
} modbus_rtu_device_t;

/* Function declarations */
modbus_rtu_device_t* modbus_rtu_init(const char *uart_name);
rt_err_t modbus_rtu_deinit(modbus_rtu_device_t *device);

rt_uint16_t modbus_crc16(rt_uint8_t *data, rt_uint16_t length);
rt_err_t modbus_read_holding_registers(modbus_rtu_device_t *device,
                                       rt_uint8_t slave_addr,
                                       rt_uint16_t start_addr,
                                       rt_uint16_t reg_count,
                                       rt_uint16_t *values);

rt_err_t modbus_read_input_registers(modbus_rtu_device_t *device,
                                   rt_uint8_t slave_addr,
                                   rt_uint16_t start_addr,
                                   rt_uint16_t reg_count,
                                   rt_uint16_t *values);

rt_err_t modbus_write_single_register(modbus_rtu_device_t *device,
                                    rt_uint8_t slave_addr,
                                    rt_uint16_t reg_addr,
                                    rt_uint16_t value);

rt_err_t modbus_send_request(modbus_rtu_device_t *device,
                            modbus_request_t *request);
rt_err_t modbus_receive_response(modbus_rtu_device_t *device,
                                 modbus_response_t *response);

#endif /* MODBUS_RTU_H__ */

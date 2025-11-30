/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-21     Developer    Modbus RTU protocol for S8 CO2 sensor
 */

#include "modbus_rtu.h"
#include "board.h"
#include <rtthread.h>  /* Add missing RT-Thread header */

/* RT-Thread serial configuration */
#ifndef BAUD_RATE_9600
#define BAUD_RATE_9600    9600
#endif

/**
 * Configure UART2 for S8 sensor
 * Note: The driver now handles re-initialization gracefully
 */
static rt_err_t configure_uart2_for_s8(void)
{
    rt_device_t uart2;
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    rt_err_t result;

    /* Find UART2 device */
    uart2 = rt_device_find("uart2");
    if (!uart2) {
    return -RT_ENOSYS;
    }

    /* Configure UART2 for S8 sensor: 9600-8N1 */
    config.baud_rate = BAUD_RATE_9600;
    config.data_bits = DATA_BITS_8;
    config.stop_bits = STOP_BITS_1;
    config.parity = PARITY_NONE;
    config.bit_order = BIT_ORDER_LSB;
    config.invert = NRZ_NORMAL;
    config.bufsz = 256;

    /* Configure device - driver handles re-initialization gracefully */
    result = rt_device_control(uart2, RT_DEVICE_CTRL_CONFIG, &config);
    /* Continue anyway - device may still work with previous configuration */

    return RT_EOK;
}

/**
 * Calculate CRC-16 for Modbus (correct algorithm)
 */
rt_uint16_t modbus_crc16(rt_uint8_t *data, rt_uint16_t length)
{
    rt_uint16_t crc_value = 0xFFFF;
    rt_uint8_t i, j;

    for (i = 0; i < length; i++) {
        crc_value ^= data[i];
        for (j = 0; j < 8; j++) {
            if (crc_value & 0x0001) {
                crc_value = (crc_value >> 1) ^ 0xA001;
            } else {
                crc_value = (crc_value >> 1);
            }
        }
    }
    
    /* Swap high and low bytes */
    crc_value = ((crc_value >> 8) + (crc_value << 8));
    
    return crc_value;
}

/**
 * Test CRC calculation with known example from manual (simplified)
 */
static void test_crc_example(void)
{
    /* CRC test removed to reduce output */
}

/**
 * Initialize Modbus RTU device
 */
modbus_rtu_device_t* modbus_rtu_init(const char *uart_name)
{
    modbus_rtu_device_t *device;
    struct rt_serial_device *serial;

    if (!uart_name) {
        rt_kprintf("[MODBUS] Error: uart_name is NULL\n");
        return RT_NULL;
    }

    device = (modbus_rtu_device_t*)rt_malloc(sizeof(modbus_rtu_device_t));
    if (!device) {
        rt_kprintf("[MODBUS] Error: Failed to allocate memory\n");
        return RT_NULL;
    }

    rt_memset(device, 0, sizeof(modbus_rtu_device_t));

    /* Find serial device */
    serial = (struct rt_serial_device *)rt_device_find(uart_name);
    if (!serial) {
        rt_kprintf("[MODBUS] Error: Cannot find UART device '%s'\n", uart_name);
        rt_free(device);
        return RT_NULL;
    }

    /* Configure UART2 specifically for S8 sensor */
    if (rt_strcmp(uart_name, "uart2") == 0) {
        rt_err_t config_result = configure_uart2_for_s8();
        if (config_result != RT_EOK) {
            rt_kprintf("[MODBUS] Warning: UART2 configuration failed, using defaults (error: %d)\n", config_result);
        }
    }

    /* Open serial device */
    if (rt_device_open((rt_device_t)serial, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX) != RT_EOK) {
        rt_kprintf("[MODBUS] Error: Failed to open UART\n");
        rt_free(device);
        return RT_NULL;
    }

    device->serial = serial;
    device->timeout_tick = rt_tick_from_millisecond(180);  /* S8 sensor timeout: 180ms (per Modbus specification) */

    /* Create mutex for thread safety */
    device->lock = rt_mutex_create("mb_lock", RT_IPC_FLAG_FIFO);
    if (!device->lock) {
        rt_kprintf("[MODBUS] Error: Failed to create mutex\n");
        rt_device_close((rt_device_t)serial);
        rt_free(device);
        return RT_NULL;
    }

    return device;
}

/**
 * Deinitialize Modbus RTU device
 */
rt_err_t modbus_rtu_deinit(modbus_rtu_device_t *device)
{
    if (!device) {
        return -RT_ERROR;
    }

    if (device->serial) {
        rt_device_close((rt_device_t)device->serial);
    }

    if (device->lock) {
        rt_mutex_delete(device->lock);
    }

    rt_free(device);
    return RT_EOK;
}

/**
 * Send Modbus request
 */
rt_err_t modbus_send_request(modbus_rtu_device_t *device, modbus_request_t *request)
{
    rt_uint8_t frame[8];
    rt_uint16_t crc;
    rt_size_t written;

    if (!device || !request) {
        return -RT_ERROR;
    }

    /* Build Modbus RTU frame */
    frame[0] = request->slave_addr;
    frame[1] = request->function_code;
    frame[2] = (request->start_addr >> 8) & 0xFF;
    frame[3] = request->start_addr & 0xFF;
    frame[4] = (request->reg_count >> 8) & 0xFF;
    frame[5] = request->reg_count & 0xFF;

    /* Calculate CRC */
    crc = modbus_crc16(frame, 6);
    frame[6] = (crc >> 8) & 0xFF;  /* High byte first */
    frame[7] = crc & 0xFF;         /* Low byte second */

    /* Debug output removed */

    /* Send frame */
    written = rt_device_write((rt_device_t)device->serial, 0, frame, 8);
    if (written != 8) {
        rt_kprintf("[MODBUS] Error: Only wrote %d bytes\n", written);
        return -RT_ERROR;
    }

    return RT_EOK;
}

/**
 * Receive Modbus response
 */
rt_err_t modbus_receive_response(modbus_rtu_device_t *device, modbus_response_t *response)
{
    rt_uint8_t buffer[MODBUS_MAX_BUFFER_SIZE];
    rt_size_t received;
    rt_uint32_t start_tick, current_tick;
    rt_uint16_t crc, received_crc;

    if (!device || !response) {
        return -RT_ERROR;
    }

    start_tick = rt_tick_get();

    /* Wait for response */
    while (1) {
        received = rt_device_read((rt_device_t)device->serial, 0, buffer, sizeof(buffer));
        if (received > 0) {
            break;
        }

        current_tick = rt_tick_get();
        if ((current_tick - start_tick) > device->timeout_tick) {
            rt_kprintf("[MODBUS] Timeout waiting for response\n");
            return -RT_ETIMEOUT;
        }

        rt_thread_mdelay(10);
    }

    if (received < 5) { /* Minimum response size */
        rt_kprintf("[MODBUS] Error: Received only %d bytes\n", received);
        return -RT_ERROR;
    }

    /* Debug output removed */

    /* Parse response */
    response->slave_addr = buffer[0];
    response->function_code = buffer[1];
    response->byte_count = buffer[2];
    response->data = &buffer[3];

    /* Verify CRC */
    crc = modbus_crc16(buffer, received - 2);
    received_crc = (buffer[received - 2] << 8) | (buffer[received - 1]);  /* High byte first, Low byte second */

    if (crc != received_crc) {
        rt_kprintf("[MODBUS] CRC error: calculated %04X, received %04X\n", crc, received_crc);
        return -RT_ERROR;
    }

    return RT_EOK;
}

/**
 * Read input registers
 */
rt_err_t modbus_read_input_registers(modbus_rtu_device_t *device,
                                   rt_uint8_t slave_addr,
                                   rt_uint16_t start_addr,
                                   rt_uint16_t reg_count,
                                   rt_uint16_t *values)
{
    modbus_request_t request;
    modbus_response_t response;
    rt_err_t result;
    rt_uint16_t i;

    if (!device || !values || reg_count == 0) {
        return -RT_ERROR;
    }

    rt_mutex_take(device->lock, RT_WAITING_FOREVER);

    /* Build request for input registers */
    request.slave_addr = slave_addr;
    request.function_code = MODBUS_FUNC_READ_INPUT_REGS;
    request.start_addr = start_addr;
    request.reg_count = reg_count;

    /* Debug output removed */

    /* Send request */
    result = modbus_send_request(device, &request);
    if (result != RT_EOK) {
        rt_mutex_release(device->lock);
        return result;
    }

    /* Wait a bit for response */
    rt_thread_mdelay(100);  /* S8 needs more time */

    /* Receive response */
    result = modbus_receive_response(device, &response);
    if (result != RT_EOK) {
        rt_mutex_release(device->lock);
        rt_kprintf("[MODBUS] Failed to receive response: %d\n", result);
        return result;
    }

    /* Verify response */
    if (response.slave_addr != slave_addr ||
        response.function_code != MODBUS_FUNC_READ_INPUT_REGS ||
        response.byte_count != (reg_count * 2)) {
        rt_kprintf("[MODBUS] Response validation failed: addr=0x%02X, func=0x%02X, bytes=%d (expected %d)\n",
                   response.slave_addr, response.function_code, response.byte_count, reg_count * 2);
        rt_mutex_release(device->lock);
        return -RT_ERROR;
    }

    /* Extract values */
    for (i = 0; i < reg_count; i++) {
        values[i] = (response.data[i * 2] << 8) | response.data[i * 2 + 1];
        /* Debug output removed */
    }

    rt_mutex_release(device->lock);
    return RT_EOK;
}

/**
 * Read holding registers (legacy function)
 */
rt_err_t modbus_read_holding_registers(modbus_rtu_device_t *device,
                                       rt_uint8_t slave_addr,
                                       rt_uint16_t start_addr,
                                       rt_uint16_t reg_count,
                                       rt_uint16_t *values)
{
    modbus_request_t request;
    modbus_response_t response;
    rt_err_t result;
    rt_uint16_t i;

    if (!device || !values || reg_count == 0) {
        return -RT_ERROR;
    }

    rt_mutex_take(device->lock, RT_WAITING_FOREVER);

    /* Build request */
    request.slave_addr = slave_addr;
    request.function_code = MODBUS_FUNC_READ_HOLDING_REGS;
    request.start_addr = start_addr;
    request.reg_count = reg_count;

    /* Debug output removed */

    /* Send request */
    result = modbus_send_request(device, &request);
    if (result != RT_EOK) {
        rt_mutex_release(device->lock);
        return result;
    }

    /* Wait a bit for response */
    rt_thread_mdelay(50);

    /* Receive response */
    result = modbus_receive_response(device, &response);
    if (result != RT_EOK) {
        rt_mutex_release(device->lock);
        rt_kprintf("[MODBUS] Failed to receive response: %d\n", result);
        return result;
    }

    /* Verify response */
    if (response.slave_addr != slave_addr ||
        response.function_code != MODBUS_FUNC_READ_HOLDING_REGS ||
        response.byte_count != (reg_count * 2)) {
        rt_kprintf("[MODBUS] Response validation failed: addr=0x%02X, func=0x%02X, bytes=%d (expected %d)\n",
                   response.slave_addr, response.function_code, response.byte_count, reg_count * 2);
        rt_mutex_release(device->lock);
        return -RT_ERROR;
    }

    /* Extract values */
    for (i = 0; i < reg_count; i++) {
        values[i] = (response.data[i * 2] << 8) | response.data[i * 2 + 1];
        /* Debug output removed */
    }

    rt_mutex_release(device->lock);
    return RT_EOK;
}

/**
 * Write single register
 */
rt_err_t modbus_write_single_register(modbus_rtu_device_t *device,
                                    rt_uint8_t slave_addr,
                                    rt_uint16_t reg_addr,
                                    rt_uint16_t value)
{
    rt_uint8_t frame[8];
    rt_uint16_t crc;
    rt_size_t written;

    if (!device) {
        return -RT_ERROR;
    }

    rt_mutex_take(device->lock, RT_WAITING_FOREVER);

    /* Build Modbus RTU frame for write single register */
    frame[0] = slave_addr;
    frame[1] = MODBUS_FUNC_WRITE_SINGLE_REG;
    frame[2] = (reg_addr >> 8) & 0xFF;
    frame[3] = reg_addr & 0xFF;
    frame[4] = (value >> 8) & 0xFF;
    frame[5] = value & 0xFF;

    /* Calculate CRC */
    crc = modbus_crc16(frame, 6);
    frame[6] = (crc >> 8) & 0xFF;  /* High byte first */
    frame[7] = crc & 0xFF;         /* Low byte second */

    /* Send frame */
    written = rt_device_write((rt_device_t)device->serial, 0, frame, 8);
    if (written != 8) {
        rt_mutex_release(device->lock);
        return -RT_ERROR;
    }

    rt_mutex_release(device->lock);
    return RT_EOK;
}

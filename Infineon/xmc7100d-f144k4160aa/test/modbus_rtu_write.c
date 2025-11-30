/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-21     Developer    Modbus RTU write single register function
 */

#include "modbus_rtu.h"

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

    /* Build Modbus RTU frame for write single register */
    frame[0] = slave_addr;
    frame[1] = MODBUS_FUNC_WRITE_SINGLE_REG;
    frame[2] = (reg_addr >> 8) & 0xFF;
    frame[3] = reg_addr & 0xFF;
    frame[4] = (value >> 8) & 0xFF;
    frame[5] = value & 0xFF;

    /* Calculate CRC */
    crc = modbus_crc16(frame, 6);
    frame[6] = crc & 0xFF;
    frame[7] = (crc >> 8) & 0xFF;

    /* Debug: Print sent frame */
    rt_kprintf("[MODBUS] Writing: ");
    for (rt_uint8_t i = 0; i < 8; i++) {
        rt_kprintf("%02X ", frame[i]);
    }
    rt_kprintf("\n");

    /* Send frame */
    written = rt_device_write((rt_device_t)device->serial, 0, frame, 8);
    if (written != 8) {
        rt_kprintf("[MODBUS] Error: Only wrote %d bytes\n", written);
        return -RT_ERROR;
    }

    return RT_EOK;

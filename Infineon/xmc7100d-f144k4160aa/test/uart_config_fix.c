/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-24     Developer    UART configuration fix for S8 sensor
 */

#include <rtthread.h>
#include <rtdevice.h>

/**
 * Configure UART2 specifically for S8 sensor
 */
rt_err_t configure_uart2_for_s8(void)
{
    rt_device_t uart2;
    struct serial_configure config = {0};
    
    rt_kprintf("[UART_CONFIG] Configuring UART2 for S8 sensor...\n");
    
    /* Find UART2 device */
    uart2 = rt_device_find("uart2");
    if (!uart2) {
        rt_kprintf("[UART_CONFIG] Error: Cannot find UART2 device\n");
        return -RT_ERROR;
    }
    
    /* Configure UART2 for S8 sensor: 9600-8N1 */
    config.baud_rate = BAUD_RATE_9600;
    config.data_bits = DATA_BITS_8;
    config.stop_bits = STOP_BITS_1;
    config.parity = PARITY_NONE;
    config.bit_order = BIT_ORDER_LSB;
    config.invert = NRZ_NORMAL;
    config.bufsz = 256;
    config.reserved = 0;
    
    rt_kprintf("[UART_CONFIG] Applying configuration: 9600-8N1\n");
    rt_kprintf("[UART_CONFIG]   - Baud rate: %d\n", config.baud_rate);
    rt_kprintf("[UART_CONFIG]   - Data bits: %d\n", config.data_bits);
    rt_kprintf("[UART_CONFIG]   - Stop bits: %d\n", config.stop_bits);
    rt_kprintf("[UART_CONFIG]   - Parity: %d\n", config.parity);
    
    /* Close device first */
    rt_device_close(uart2);
    
    /* Configure device */
    rt_err_t result = rt_device_control(uart2, RT_DEVICE_CTRL_CONFIG, &config);
    if (result != RT_EOK) {
        rt_kprintf("[UART_CONFIG] Error: Failed to configure UART2 (error: %d)\n", result);
        return result;
    }
    
    /* Re-open device with proper flags */
    result = rt_device_open(uart2, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX);
    if (result != RT_EOK) {
        rt_kprintf("[UART_CONFIG] Error: Failed to open UART2 (error: %d)\n", result);
        return result;
    }
    
    rt_kprintf("[UART_CONFIG] UART2 configured and opened successfully\n");
    rt_kprintf("[UART_CONFIG] Using P19_0(RX) and P19_1(TX) pins\n");
    
    return RT_EOK;
}

/**
 * Test UART configuration by sending test data
 */
rt_err_t test_uart2_configuration(void)
{
    rt_device_t uart2;
    const char test_data[] = "UART2_TEST";
    rt_size_t written;
    
    rt_kprintf("[UART_TEST] Testing UART2 configuration...\n");
    
    uart2 = rt_device_find("uart2");
    if (!uart2) {
        rt_kprintf("[UART_TEST] Error: Cannot find UART2 device\n");
        return -RT_ERROR;
    }
    
    /* Send test data */
    written = rt_device_write(uart2, 0, test_data, sizeof(test_data));
    rt_kprintf("[UART_TEST] Sent %d bytes: '%s'\n", written, test_data);
    
    if (written == sizeof(test_data)) {
        rt_kprintf("[UART_TEST] UART2 test: SUCCESS\n");
        return RT_EOK;
    } else {
        rt_kprintf("[UART_TEST] UART2 test: FAILED (wrote %d, expected %d)\n", 
                   written, sizeof(test_data));
        return -RT_ERROR;
    }
}

/* Export to msh */
MSH_CMD_EXPORT(configure_uart2_for_s8, Configure UART2 for S8 sensor);

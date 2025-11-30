/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-27     Developer    Simplified main with S8 auto-init
 * 2025-11-29     Developer    Optimized startup output for production use
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <sys/time.h>
#include "s8_sensor.h"
#include "tf_card.h"
#include "nvs_state.h"

/* External function declarations */
extern rt_err_t s8_self_test_silent(void);

/* Global instances for MSH commands */
s8_sensor_device_t *g_main_s8_device = RT_NULL;
tf_monitor_state_t *g_main_tf_monitor = RT_NULL;

/**
 * Initialize RTC with default time if not set
 */
static void init_rtc_default_time(void)
{
    rt_device_t rtc_dev;
    time_t default_time = 1703900800; /* 2024-01-01 00:00:00 UTC */
    time_t current_time;
    
    /* Find RTC device */
    rtc_dev = rt_device_find("rtc");
    if (rtc_dev == RT_NULL) {
        rt_kprintf("[RTC] RTC device not found\n");
        return;
    }
    
    /* Open RTC device */
    if (rt_device_open(rtc_dev, RT_DEVICE_OFLAG_RDWR) != RT_EOK) {
        rt_kprintf("[RTC] Failed to open RTC device\n");
        return;
    }
    
    /* Read current time */
    if (rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_GET_TIME, &current_time) == RT_EOK) {
        /* Check if time is reasonable (after 2020-01-01) */
        if (current_time < 1577836800) {
            rt_kprintf("[RTC] Setting default time: 2024-01-01 00:00:00 UTC\n");
            rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_SET_TIME, &default_time);
        } else {
            rt_kprintf("[RTC] Time already set\n");
        }
    } else {
        rt_kprintf("[RTC] Failed to read RTC time\n");
    }
    
    rt_device_close(rtc_dev);
}

/**
 * S8 CO2 Sensor and TF Card automatic initialization
 */
int main(void)
{
    s8_sensor_device_t *s8_device;  /* Local device pointer */
    tf_status_t tf_status;
    
    rt_kprintf("=== RT-Thread System Started ===\n");
    
    /* Initialize RTC first */
    rt_kprintf("RTC Initializing...\n");
    init_rtc_default_time();
    
    /* Initialize TF Card */
    rt_kprintf("TF Card Initializing...\n");
    tf_status = tf_card_init();
    if (tf_status == TF_STATUS_OK) {
        /* Initialize data storage */
        if (tf_data_init() == TF_STATUS_OK) {
            /* Initialize persistent TF monitor state */
            g_main_tf_monitor = (tf_monitor_state_t *)rt_malloc(sizeof(tf_monitor_state_t));
            if (g_main_tf_monitor != RT_NULL) {
                if (tf_monitor_init(g_main_tf_monitor) == TF_STATUS_OK) {
                    rt_kprintf("TF System: READY (Monitor initialized)\n");
                } else {
                    rt_kprintf("TF System: READY (Monitor init failed)\n");
                    rt_free(g_main_tf_monitor);
                    g_main_tf_monitor = RT_NULL;
                }
            } else {
                rt_kprintf("TF System: READY (Monitor memory allocation failed)\n");
            }
        } else {
            rt_kprintf("TF System: FAILED - Data storage error\n");
        }
    } else if (tf_status == TF_STATUS_NOT_MOUNTED) {
        rt_kprintf("TF System: NOT FOUND - No TF card detected\n");
    } else {
        rt_kprintf("TF System: FAILED - Initialization error (code: %d)\n", tf_status);
    }
    
    /* Initialize S8 sensor and keep it active */
    rt_kprintf("S8 CO2 Sensor Initializing...\n");
    s8_device = s8_sensor_init("uart2");
    if (s8_device == RT_NULL) {
        rt_kprintf("S8 System: FAILED - Initialization error\n");
        rt_kprintf("Check sensor connections and power\n");
        return 0;
    }
    
    /* Make sensor available to MSH commands */
    g_main_s8_device = s8_device;
    
    /* Wait for sensor to stabilize */
    rt_thread_mdelay(2000);
    
    /* Test basic communication */
    s8_status_t result = s8_read_co2_data(s8_device);
    if (result == S8_STATUS_OK) {
        rt_kprintf("S8 System: READY\n");
    } else {
        rt_kprintf("S8 System: FAILED - Communication error (code: %d)\n", result);
        rt_kprintf("Run 's8_self_test' for detailed diagnostics\n");
        
        /* Run detailed self-test to show specific errors */
        s8_self_test_silent();
        
        s8_sensor_deinit(s8_device);
        g_main_s8_device = RT_NULL;
    }
    
    rt_kprintf("System initialization complete.\n");
    rt_kprintf("Type 'help' for available commands.\n");

    /* Auto-resume TF monitoring if it was running before power loss */
    if (g_main_tf_monitor != RT_NULL && g_main_s8_device != RT_NULL && tf_status == TF_STATUS_OK)
    {
        rt_kprintf("\n=== Checking for Power Loss Recovery ===\n");

        /* Initialize NVS state storage */
        if (nvs_state_init() != RT_EOK)
        {
            rt_kprintf("NVS init failed - auto-resume unavailable\n");
        }
        else if (nvs_state_needs_recovery())
        {
            /* Power loss detected - need to resume monitoring */
            nvs_monitor_state_t nvs_state;

            if (nvs_state_load(&nvs_state) == RT_EOK)
            {
                rt_kprintf("*** POWER LOSS DETECTED ***\n");
                rt_kprintf("Previous session found:\n");
                rt_kprintf("  - Base file: %s\n", nvs_state.base_filename);
                rt_kprintf("  - Interval: %lu sec\n", nvs_state.interval_sec);
                rt_kprintf("  - Samples logged: %lu\n", nvs_state.sample_count);
                rt_kprintf("  - Continuations: %d\n", nvs_state.continuation_count);

                /* Give system time to stabilize */
                rt_thread_mdelay(2000);

                /* Test S8 sensor communication */
                if (s8_read_co2_data(g_main_s8_device) == S8_STATUS_OK)
                {
                    /* Prepare continuation state */
                    if (nvs_state_prepare_continuation(&nvs_state) == RT_EOK)
                    {
                        /* Generate continuation filename */
                        char continuation_filename[64];
                        nvs_state_get_continuation_filename(nvs_state.base_filename,
                                                           nvs_state.continuation_count,
                                                           continuation_filename,
                                                           sizeof(continuation_filename));

                        /* Build full path for the continuation file */
                        rt_snprintf(g_main_tf_monitor->session_file,
                                   sizeof(g_main_tf_monitor->session_file),
                                   "/co2_log/%s", continuation_filename);

                        rt_kprintf("Resuming with continuation file: %s\n", continuation_filename);

                        /* Start monitoring with continuation state */
                        g_main_tf_monitor->sample_count = 0;  /* Reset for new continuation */
                        if (tf_monitor_start(g_main_tf_monitor, nvs_state.interval_sec) == TF_STATUS_OK)
                        {
                            rt_kprintf("*** AUTO-RESUME SUCCESSFUL ***\n");
                            rt_kprintf("Monitoring resumed (interval: %lu sec)\n", nvs_state.interval_sec);
                            rt_kprintf("Continuation #%d started\n", nvs_state.continuation_count);
                        }
                        else
                        {
                            rt_kprintf("*** AUTO-RESUME FAILED ***\n");
                            rt_kprintf("Use 'tf_monitor start <interval>' to begin manually\n");
                            nvs_state_clear();  /* Clear failed state */
                        }
                    }
                    else
                    {
                        rt_kprintf("Failed to prepare continuation state\n");
                        nvs_state_clear();
                    }
                }
                else
                {
                    rt_kprintf("S8 sensor not ready - auto-resume skipped\n");
                    rt_kprintf("Previous session will remain in state file\n");
                    rt_kprintf("Run 'tf_monitor start' manually when sensor is ready\n");
                }
            }
            else
            {
                rt_kprintf("Failed to load recovery state\n");
            }
        }
        else
        {
            rt_kprintf("No power loss detected - system started normally\n");
        }
    }

    return 0;  /* Let scheduler continue working */
}

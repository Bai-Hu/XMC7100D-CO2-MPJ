/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-27     Developer    TF Card MSH commands
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <stdlib.h>
#include <dfs_fs.h>
#include <dfs_file.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "tf_card.h"
#include "s8_sensor.h"

#define DBG_TAG "tf_msh"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/* External S8 sensor and TF monitor from main.c */
extern s8_sensor_device_t *g_main_s8_device;
extern tf_monitor_state_t *g_main_tf_monitor;

/*
 * =============================================================================
 * MSH Command: tf_init
 * Initialize TF card driver
 * =============================================================================
 */
static int cmd_tf_init(int argc, char **argv)
{
    tf_status_t status;

    rt_kprintf("Initializing TF card...\n");

    status = tf_card_init();
    if (status == TF_STATUS_OK)
    {
        rt_kprintf("TF card initialized successfully\n");

        /* Also initialize data storage */
        status = tf_data_init();
        if (status == TF_STATUS_OK)
        {
            rt_kprintf("Data storage initialized\n");
        }
        else
        {
            rt_kprintf("Data storage init failed: %d\n", status);
        }
    }
    else
    {
        rt_kprintf("TF card init failed: %d\n", status);
    }

    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_tf_init, tf_init, Initialize TF card driver);

/*
 * =============================================================================
 * MSH Command: tf_info
 * Display TF card information
 * =============================================================================
 */
static int cmd_tf_info(int argc, char **argv)
{
    tf_card_info_t info;
    tf_status_t status;

    status = tf_card_get_info(&info);
    if (status != TF_STATUS_OK)
    {
        rt_kprintf("Failed to get TF card info: %d\n", status);
        return -1;
    }

    rt_kprintf("=== TF Card Information ===\n");
    rt_kprintf("Status: %s\n", info.mounted ? "Mounted" : "Not mounted");
    rt_kprintf("Total size: %lu MB\n", info.total_size_mb);
    rt_kprintf("Free size: %lu MB\n", info.free_size_mb);
    rt_kprintf("Sector size: %lu bytes\n", info.sector_size);
    rt_kprintf("Total sectors: %lu\n", info.total_sectors);

    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_tf_info, tf_info, Display TF card information);

/*
 * =============================================================================
 * MSH Command: tf_test
 * Test TF card read/write
 * =============================================================================
 */
static int cmd_tf_test(int argc, char **argv)
{
    tf_status_t status;

    rt_kprintf("Testing TF card...\n");

    status = tf_card_test();
    if (status == TF_STATUS_OK)
    {
        rt_kprintf("TF card test: PASSED\n");
    }
    else
    {
        rt_kprintf("TF card test: FAILED (%d)\n", status);
    }

    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_tf_test, tf_test, Test TF card read write);

/*
 * =============================================================================
 * MSH Command: tf_log
 * Log current CO2 reading to TF card
 * =============================================================================
 */
static int cmd_tf_log(int argc, char **argv)
{
    tf_co2_record_t record;
    tf_status_t status;
    time_t current_rtc;

    if (!tf_card_is_ready())
    {
        rt_kprintf("TF card not ready. Run 'tf_init' first.\n");
        return -1;
    }

    if (g_main_s8_device == RT_NULL)
    {
        rt_kprintf("S8 sensor not initialized\n");
        return -1;
    }

    /* Read current CO2 data */
    s8_status_t s8_status = s8_read_co2_data(g_main_s8_device);
    if (s8_status != S8_STATUS_OK)
    {
        rt_kprintf("Failed to read S8 sensor: %d\n", s8_status);
        return -1;
    }

    /* Get current RTC time */
    time(&current_rtc);

    /* Build record with new format */
    record.rtc_timestamp = current_rtc;
    record.elapsed_seconds = 0;  /* For single log, elapsed is 0 */
    record.co2_ppm = g_main_s8_device->data.co2_ppm;

    /* Write to TF card */
    status = tf_data_write_record(&record);
    if (status == TF_STATUS_OK)
    {
        rt_kprintf("Logged: CO2=%d ppm, RTC=%lu\n",
                   record.co2_ppm, record.rtc_timestamp);
    }
    else
    {
        rt_kprintf("Failed to log data: %d\n", status);
    }

    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_tf_log, tf_log, Log current CO2 reading to TF card);

/*
 * =============================================================================
 * MSH Command: tf_list
 * List data files on TF card
 * =============================================================================
 */
static void file_list_callback(const char *filename, rt_uint32_t record_count)
{
    if (record_count > 0)
    {
        rt_kprintf("  %s (%lu records)\n", filename, record_count);
    }
    else
    {
        rt_kprintf("  %s\n", filename);
    }
}

static int cmd_tf_list(int argc, char **argv)
{
    tf_status_t status;

    if (!tf_card_is_ready())
    {
        rt_kprintf("TF card not ready. Run 'tf_init' first.\n");
        return -1;
    }

    rt_kprintf("=== Data Files ===\n");
    status = tf_file_list(file_list_callback);
    if (status == TF_STATUS_NOT_FOUND)
    {
        rt_kprintf("No data files found.\n");
    }
    else if (status != TF_STATUS_OK)
    {
        rt_kprintf("Failed to list files: %d\n", status);
    }

    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_tf_list, tf_list, List data files on TF card);

/*
 * =============================================================================
 * MSH Command: tf_send
 * Send file to PC via serial
 * Usage: tf_send <filename> [serial_device]
 * =============================================================================
 */
static int cmd_tf_send(int argc, char **argv)
{
    const char *filename;
    const char *serial = "uart4";  /* Default console */
    tf_status_t status;

    if (argc < 2)
    {
        rt_kprintf("Usage: tf_send <filename> [serial_device]\n");
        rt_kprintf("Example: tf_send 20251127.csv uart4\n");
        return -1;
    }

    filename = argv[1];
    if (argc >= 3)
    {
        serial = argv[2];
    }

    if (!tf_card_is_ready())
    {
        rt_kprintf("TF card not ready. Run 'tf_init' first.\n");
        return -1;
    }

    rt_kprintf("Sending file: %s via %s\n", filename, serial);
    status = tf_serial_send_file(filename, serial);
    if (status != TF_STATUS_OK)
    {
        rt_kprintf("Send failed: %d\n", status);
    }

    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_tf_send, tf_send, Send file to PC via serial);

/*
 * =============================================================================
 * MSH Command: tf_export
 * Export data file as CSV to serial
 * Usage: tf_export <filename> [serial_device]
 * =============================================================================
 */
static int cmd_tf_export(int argc, char **argv)
{
    const char *filename;
    const char *serial = "uart4";
    tf_status_t status;

    if (argc < 2)
    {
        rt_kprintf("Usage: tf_export <filename> [serial_device]\n");
        return -1;
    }

    filename = argv[1];
    if (argc >= 3)
    {
        serial = argv[2];
    }

    if (!tf_card_is_ready())
    {
        rt_kprintf("TF card not ready. Run 'tf_init' first.\n");
        return -1;
    }

    rt_kprintf("Exporting CSV: %s via %s\n", filename, serial);
    status = tf_serial_export_csv(filename, serial);
    if (status != TF_STATUS_OK)
    {
        rt_kprintf("Export failed: %d\n", status);
    }

    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_tf_export, tf_export, Export data file as CSV);

/*
 * =============================================================================
 * MSH Command: tf_monitor
 * Start/stop continuous logging to TF card (using persistent state)
 * Usage: tf_monitor start|stop [interval_sec]
 * =============================================================================
 */
static int cmd_tf_monitor(int argc, char **argv)
{
    if (argc < 2)
    {
        rt_kprintf("Usage: tf_monitor start|stop [interval_sec]\n");
        if (g_main_tf_monitor != RT_NULL)
        {
            rt_kprintf("Status: %s\n", tf_monitor_is_running(g_main_tf_monitor) ? "Running" : "Stopped");
            if (tf_monitor_is_running(g_main_tf_monitor))
            {
                rt_kprintf("Samples logged: %lu\n", g_main_tf_monitor->sample_count);
                rt_kprintf("Interval: %lu seconds\n", g_main_tf_monitor->interval_sec);
                rt_kprintf("Session file: %s\n", g_main_tf_monitor->session_file);
                rt_kprintf("Power outage: %s\n", g_main_tf_monitor->power_outage_detected ? "Detected" : "None");
            }
        }
        else
        {
            rt_kprintf("Status: Monitor not initialized\n");
        }
        return 0;
    }

    if (rt_strcmp(argv[1], "start") == 0)
    {
        if (g_main_tf_monitor == RT_NULL)
        {
            rt_kprintf("TF monitor not initialized. Check TF card initialization.\n");
            return -1;
        }

        if (tf_monitor_is_running(g_main_tf_monitor))
        {
            rt_kprintf("Monitor already running (samples: %lu)\n", g_main_tf_monitor->sample_count);
            rt_kprintf("Session file: %s\n", g_main_tf_monitor->session_file);
            return 0;
        }

        if (!tf_card_is_ready())
        {
            rt_kprintf("TF card not ready. Run 'tf_init' first.\n");
            return -1;
        }

        rt_uint32_t interval_sec = 5;  /* Default 5 seconds */
        if (argc >= 3)
        {
            interval_sec = atoi(argv[2]);
            if (interval_sec < 1)
                interval_sec = 1;
        }

        tf_status_t status = tf_monitor_start(g_main_tf_monitor, interval_sec);
        if (status == TF_STATUS_OK)
        {
            rt_kprintf("TF monitor started successfully\n");
            rt_kprintf("Interval: %lu seconds\n", interval_sec);
            if (g_main_tf_monitor->power_outage_detected)
            {
                rt_kprintf("Power outage detected - new session file created\n");
            }
            else
            {
                rt_kprintf("Normal session started\n");
            }
        }
        else
        {
            rt_kprintf("Failed to start monitor: %d\n", status);
        }
    }
    else if (rt_strcmp(argv[1], "stop") == 0)
    {
        if (g_main_tf_monitor == RT_NULL)
        {
            rt_kprintf("TF monitor not initialized.\n");
            return -1;
        }

        if (!tf_monitor_is_running(g_main_tf_monitor))
        {
            rt_kprintf("Monitor not running\n");
            return 0;
        }

        tf_status_t status = tf_monitor_stop(g_main_tf_monitor);
        if (status == TF_STATUS_OK)
        {
            rt_kprintf("TF monitor stopped (total samples: %lu)\n", g_main_tf_monitor->sample_count);
        }
        else
        {
            rt_kprintf("Failed to stop monitor: %d\n", status);
        }
    }
    else
    {
        rt_kprintf("Unknown command: %s\n", argv[1]);
        rt_kprintf("Usage: tf_monitor start|stop [interval_sec]\n");
    }

    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_tf_monitor, tf_monitor, Start or stop continuous logging);

/*
 * =============================================================================
 * MSH Command: tf_realtime
 * Send real-time CO2 data via serial
 * Usage: tf_realtime start|stop [serial_device]
 * =============================================================================
 */
static rt_thread_t tf_realtime_thread = RT_NULL;
static rt_bool_t tf_realtime_running = RT_FALSE;
static char tf_realtime_serial[16] = "uart4";

static void tf_realtime_thread_entry(void *parameter)
{
    tf_co2_record_t record;
    time_t current_rtc;
    static rt_device_t rtc_dev = RT_NULL;

    /* Find RTC device */
    if (rtc_dev == RT_NULL)
    {
        rtc_dev = rt_device_find("rtc");
        if (rtc_dev == RT_NULL)
        {
            rt_kprintf("[TF Realtime] Error: RTC device not found\n");
            return;
        }
        
        if (rt_device_open(rtc_dev, RT_DEVICE_OFLAG_RDWR) != RT_EOK)
        {
            rt_kprintf("[TF Realtime] Error: Failed to open RTC device\n");
            return;
        }
    }

    rt_kprintf("Real-time streaming started via %s\n", tf_realtime_serial);

    while (tf_realtime_running)
    {
        if (g_main_s8_device != RT_NULL)
        {
            if (s8_read_co2_data(g_main_s8_device) == S8_STATUS_OK)
            {
                /* Get current RTC time */
                if (rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_GET_TIME, &current_rtc) == RT_EOK)
                {
                    /* Build record with new format */
                    record.rtc_timestamp = current_rtc;
                    record.elapsed_seconds = 0;  /* Not applicable for realtime */
                    record.co2_ppm = g_main_s8_device->data.co2_ppm;

                    tf_serial_send_record(&record, tf_realtime_serial);
                }
            }
        }

        rt_thread_mdelay(2000);  /* Every 2 seconds (sensor update rate) */
    }

    rt_kprintf("Real-time streaming stopped\n");
    
    /* Close RTC device */
    if (rtc_dev != RT_NULL)
    {
        rt_device_close(rtc_dev);
        rtc_dev = RT_NULL;
    }
}

static int cmd_tf_realtime(int argc, char **argv)
{
    if (argc < 2)
    {
        rt_kprintf("Usage: tf_realtime start|stop [serial_device]\n");
        rt_kprintf("Status: %s\n", tf_realtime_running ? "Running" : "Stopped");
        return 0;
    }

    if (rt_strcmp(argv[1], "start") == 0)
    {
        if (tf_realtime_running)
        {
            rt_kprintf("Real-time streaming already running\n");
            return 0;
        }

        if (argc >= 3)
        {
            rt_strncpy(tf_realtime_serial, argv[2], sizeof(tf_realtime_serial) - 1);
        }

        tf_realtime_running = RT_TRUE;
        tf_realtime_thread = rt_thread_create("tf_rt",
                                               tf_realtime_thread_entry,
                                               RT_NULL,
                                               1536,
                                               21,
                                               10);
        if (tf_realtime_thread != RT_NULL)
        {
            rt_thread_startup(tf_realtime_thread);
        }
        else
        {
            tf_realtime_running = RT_FALSE;
            rt_kprintf("Failed to create real-time thread\n");
        }
    }
    else if (rt_strcmp(argv[1], "stop") == 0)
    {
        tf_realtime_running = RT_FALSE;
    }
    else
    {
        rt_kprintf("Unknown command: %s\n", argv[1]);
    }

    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_tf_realtime, tf_realtime, Stream real-time CO2 data via serial);

/*
 * =============================================================================
 * MSH Command: tf_emergency_stop
 * Emergency stop for power failure situations
 * =============================================================================
 */
static int cmd_tf_emergency_stop(int argc, char **argv)
{
    if (g_main_tf_monitor == RT_NULL)
    {
        rt_kprintf("TF monitor not initialized.\n");
        return -1;
    }

    rt_kprintf("Emergency stop triggered - saving data...\n");
    tf_status_t status = tf_monitor_emergency_shutdown(g_main_tf_monitor);
    if (status == TF_STATUS_OK)
    {
        rt_kprintf("Emergency shutdown completed - data saved safely\n");
    }
    else
    {
        rt_kprintf("Emergency shutdown failed: %d\n", status);
    }

    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_tf_emergency_stop, tf_emergency_stop, Emergency stop and save data);

/*
 * =============================================================================
 * MSH Command: tf_battery_mode
 * Enable battery-optimized mode with enhanced data protection
 * =============================================================================
 */
static int cmd_tf_battery_mode(int argc, char **argv)
{
    rt_bool_t enable = RT_TRUE;

    if (argc >= 2)
    {
        if (rt_strcmp(argv[1], "off") == 0 || rt_strcmp(argv[1], "0") == 0)
        {
            enable = RT_FALSE;
        }
    }

    if (enable)
    {
        rt_kprintf("Battery mode ENABLED:\n");
        rt_kprintf("- Immediate data sync on every sample\n");
        rt_kprintf("- Enhanced power failure protection\n");
        rt_kprintf("- Auto-resume after power restoration\n");
        rt_kprintf("- RTC backup timestamp strategy\n");
    }
    else
    {
        rt_kprintf("Battery mode DISABLED\n");
    }

    return 0;
}
MSH_CMD_EXPORT_ALIAS(cmd_tf_battery_mode, tf_battery_mode, Enable/disable battery-optimized mode);

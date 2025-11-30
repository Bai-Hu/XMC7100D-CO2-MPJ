/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-28     Developer    RTC MSH commands implementation
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <sys/time.h>
#include <stdio.h>
#include "rtc_msh.h"

static rt_device_t rtc_dev = RT_NULL;

/**
 * Show RTC help information
 */
static void rtc_help(void)
{
    rt_kprintf("RTC Commands:\n");
    rt_kprintf("  rtc_read                    - Read current RTC time\n");
    rt_kprintf("  rtc_set [YYYY-MM-DD HH:MM:SS] - Set complete date and time\n");
    rt_kprintf("  rtc_date [YYYY-MM-DD]       - Set date only\n");
    rt_kprintf("  rtc_time [HH:MM:SS]         - Set time only\n");
    rt_kprintf("  rtc_info                    - Show RTC information\n");
    rt_kprintf("  rtc_help                    - Show this help\n");
    rt_kprintf("\nExamples:\n");
    rt_kprintf("  rtc_read                    # Read current time\n");
    rt_kprintf("  rtc_set 2025-11-28 14:30:00 # Set complete time\n");
    rt_kprintf("  rtc_date 2025-11-28        # Set date only\n");
    rt_kprintf("  rtc_time 14:30:00          # Set time only\n");
}

/**
 * Initialize RTC device reference
 */
static rt_err_t rtc_init_device(void)
{
    if (rtc_dev == RT_NULL)
    {
        rtc_dev = rt_device_find("rtc");
        if (rtc_dev == RT_NULL)
        {
            rt_kprintf("[RTC] Error: RTC device not found\n");
            return -RT_ERROR;
        }
        
        if (rt_device_open(rtc_dev, RT_DEVICE_OFLAG_RDWR) != RT_EOK)
        {
            rt_kprintf("[RTC] Error: Failed to open RTC device\n");
            return -RT_ERROR;
        }
    }
    return RT_EOK;
}

/**
 * Parse time string in format "HH:MM:SS"
 */
static rt_err_t parse_time_string(const char *time_str, struct tm *tm_time)
{
    int hour, minute, second;
    
    if (sscanf(time_str, "%d:%d:%d", &hour, &minute, &second) != 3)
    {
        return -RT_ERROR;
    }
    
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59)
    {
        return -RT_ERROR;
    }
    
    tm_time->tm_hour = hour;
    tm_time->tm_min = minute;
    tm_time->tm_sec = second;
    
    return RT_EOK;
}

/**
 * Parse date string in format "YYYY-MM-DD"
 */
static rt_err_t parse_date_string(const char *date_str, struct tm *tm_time)
{
    int year, month, day;
    
    if (sscanf(date_str, "%d-%d-%d", &year, &month, &day) != 3)
    {
        return -RT_ERROR;
    }
    
    if (year < 1900 || year > 2100 || month < 1 || month > 12 || day < 1 || day > 31)
    {
        return -RT_ERROR;
    }
    
    tm_time->tm_year = year - 1900;
    tm_time->tm_mon = month - 1;
    tm_time->tm_mday = day;
    
    return RT_EOK;
}

/**
 * Read current RTC time
 */
static void rtc_read(int argc, char *argv[])
{
    time_t timestamp;
    struct tm *tm_info;
    
    RT_UNUSED(argc);
    RT_UNUSED(argv);
    
    if (rtc_init_device() != RT_EOK)
    {
        return;
    }
    
    if (rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_GET_TIME, &timestamp) != RT_EOK)
    {
        rt_kprintf("[RTC] Error: Failed to read time\n");
        return;
    }
    
    tm_info = gmtime(&timestamp);
    if (tm_info == RT_NULL)
    {
        rt_kprintf("[RTC] Error: Failed to convert time\n");
        return;
    }
    
    rt_kprintf("[RTC] Current time: %04d-%02d-%02d %02d:%02d:%02d\n",
               tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
               tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
    rt_kprintf("[RTC] Timestamp: %ld\n", timestamp);
    rt_kprintf("[RTC] Weekday: %s\n", 
               tm_info->tm_wday == 0 ? "Sunday" :
               tm_info->tm_wday == 1 ? "Monday" :
               tm_info->tm_wday == 2 ? "Tuesday" :
               tm_info->tm_wday == 3 ? "Wednesday" :
               tm_info->tm_wday == 4 ? "Thursday" :
               tm_info->tm_wday == 5 ? "Friday" : "Saturday");
}

/**
 * Set complete date and time
 */
static void rtc_set(int argc, char *argv[])
{
    struct tm tm_time;
    time_t timestamp;
    
    if (argc != 3)
    {
        rt_kprintf("[RTC] Usage: rtc_set [YYYY-MM-DD] [HH:MM:SS]\n");
        rt_kprintf("[RTC] Example: rtc_set 2025-11-28 14:30:00\n");
        return;
    }
    
    if (rtc_init_device() != RT_EOK)
    {
        return;
    }
    
    /* Get current time as base */
    time(&timestamp);
    gmtime_r(&timestamp, &tm_time);
    
    /* Parse date */
    if (parse_date_string(argv[1], &tm_time) != RT_EOK)
    {
        rt_kprintf("[RTC] Error: Invalid date format. Use YYYY-MM-DD\n");
        return;
    }
    
    /* Parse time */
    if (parse_time_string(argv[2], &tm_time) != RT_EOK)
    {
        rt_kprintf("[RTC] Error: Invalid time format. Use HH:MM:SS\n");
        return;
    }
    
    /* Convert to timestamp */
    timestamp = timegm(&tm_time);
    
    /* Set RTC time */
    if (rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_SET_TIME, &timestamp) != RT_EOK)
    {
        rt_kprintf("[RTC] Error: Failed to set time\n");
        return;
    }
    
    rt_kprintf("[RTC] Time set successfully: %04d-%02d-%02d %02d:%02d:%02d\n",
               tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
               tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
}

/**
 * Set date only
 */
static void rtc_date(int argc, char *argv[])
{
    struct tm tm_time;
    time_t timestamp;
    
    if (argc != 2)
    {
        rt_kprintf("[RTC] Usage: rtc_date [YYYY-MM-DD]\n");
        rt_kprintf("[RTC] Example: rtc_date 2025-11-28\n");
        return;
    }
    
    if (rtc_init_device() != RT_EOK)
    {
        return;
    }
    
    /* Get current time as base */
    time(&timestamp);
    gmtime_r(&timestamp, &tm_time);
    
    /* Parse date */
    if (parse_date_string(argv[1], &tm_time) != RT_EOK)
    {
        rt_kprintf("[RTC] Error: Invalid date format. Use YYYY-MM-DD\n");
        return;
    }
    
    /* Convert to timestamp */
    timestamp = timegm(&tm_time);
    
    /* Set RTC time */
    if (rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_SET_TIME, &timestamp) != RT_EOK)
    {
        rt_kprintf("[RTC] Error: Failed to set date\n");
        return;
    }
    
    rt_kprintf("[RTC] Date set successfully: %04d-%02d-%02d\n",
               tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday);
}

/**
 * Set time only
 */
static void rtc_time(int argc, char *argv[])
{
    struct tm tm_time;
    time_t timestamp;
    
    if (argc != 2)
    {
        rt_kprintf("[RTC] Usage: rtc_time [HH:MM:SS]\n");
        rt_kprintf("[RTC] Example: rtc_time 14:30:00\n");
        return;
    }
    
    if (rtc_init_device() != RT_EOK)
    {
        return;
    }
    
    /* Get current time as base */
    time(&timestamp);
    gmtime_r(&timestamp, &tm_time);
    
    /* Parse time */
    if (parse_time_string(argv[1], &tm_time) != RT_EOK)
    {
        rt_kprintf("[RTC] Error: Invalid time format. Use HH:MM:SS\n");
        return;
    }
    
    /* Convert to timestamp */
    timestamp = timegm(&tm_time);
    
    /* Set RTC time */
    if (rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_SET_TIME, &timestamp) != RT_EOK)
    {
        rt_kprintf("[RTC] Error: Failed to set time\n");
        return;
    }
    
    rt_kprintf("[RTC] Time set successfully: %02d:%02d:%02d\n",
               tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
}

/**
 * Show RTC information
 */
static void rtc_info(int argc, char *argv[])
{
    time_t timestamp;
    struct tm *tm_info;
    struct timeval tv;
    
    RT_UNUSED(argc);
    RT_UNUSED(argv);
    
    if (rtc_init_device() != RT_EOK)
    {
        return;
    }
    
    rt_kprintf("[RTC] Device Information:\n");
    rt_kprintf("  Device name: %s\n", rtc_dev->parent.name);
    rt_kprintf("  Device type: RTC\n");
    rt_kprintf("  Open count: %d\n", rtc_dev->open_flag);
    
    /* Get current time */
    if (rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_GET_TIME, &timestamp) == RT_EOK)
    {
        tm_info = gmtime(&timestamp);
        if (tm_info != RT_NULL)
        {
            rt_kprintf("  Current time: %04d-%02d-%02d %02d:%02d:%02d\n",
                       tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                       tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
            rt_kprintf("  Timestamp: %ld\n", timestamp);
        }
    }
    
    /* Get timeval if supported */
    if (rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_GET_TIMEVAL, &tv) == RT_EOK)
    {
        rt_kprintf("  Timeval: %ld.%06ld\n", tv.tv_sec, tv.tv_usec);
    }
    
#ifdef BSP_RTC_USING_WCO
    rt_kprintf("  Clock source: WCO (External 32.768kHz crystal)\n");
#elif defined(BSP_RTC_USING_ILO)
    rt_kprintf("  Clock source: ILO (Internal oscillator)\n");
#else
    rt_kprintf("  Clock source: Unknown\n");
#endif
}

/**
 * Initialize RTC MSH commands
 */
int rtc_msh_init(void)
{
    rt_kprintf("[RTC] RTC MSH Commands Loaded\n");
    rt_kprintf("[RTC] Type 'rtc_help' for available commands\n");
    
    return RT_EOK;
}

/* Export to MSH commands */
MSH_CMD_EXPORT(rtc_read, Read current RTC time);
MSH_CMD_EXPORT(rtc_set, Set complete date and time);
MSH_CMD_EXPORT(rtc_date, Set date only);
MSH_CMD_EXPORT(rtc_time, Set time only);
MSH_CMD_EXPORT(rtc_info, Show RTC information);
MSH_CMD_EXPORT(rtc_help, Show RTC command help);

/* Auto-initialization */
INIT_APP_EXPORT(rtc_msh_init);

/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-27     Developer    TF Card driver - Stage 1: Communication
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_fs.h>
#include <dfs_file.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdlib.h>
#include <errno.h>
#include "tf_card.h"
#include "s8_sensor.h"
#include "nvs_state.h"

#define DBG_TAG "tf_card"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/*
 * =============================================================================
 * Local Definitions
 * =============================================================================
 */
#define TF_MOUNT_POINT      "/"
#define TF_LOG_DIR          "/co2_log"
#define TF_TEST_FILE        "/tf_test.tmp"
#define TF_WRITE_BUFFER_SIZE 64

/*
 * =============================================================================
 * Local Variables
 * =============================================================================
 */
static rt_bool_t tf_initialized = RT_FALSE;
static rt_mutex_t tf_mutex = RT_NULL;
static int current_file_fd = -1;

/*
 * =============================================================================
 * Internal Helper Functions
 * =============================================================================
 */

/**
 * @brief Lock TF card access mutex
 */
static rt_err_t tf_lock(void)
{
    if (tf_mutex == RT_NULL)
        return -RT_ERROR;
    return rt_mutex_take(tf_mutex, RT_WAITING_FOREVER);
}

/**
 * @brief Unlock TF card access mutex
 */
static void tf_unlock(void)
{
    if (tf_mutex != RT_NULL)
        rt_mutex_release(tf_mutex);
}

/**
 * @brief Check if file system is mounted
 */
static rt_bool_t tf_is_mounted(void)
{
    struct statfs fs_stat;

    if (dfs_statfs(TF_MOUNT_POINT, &fs_stat) == 0)
    {
        return RT_TRUE;
    }
    return RT_FALSE;
}

/*
 * =============================================================================
 * Stage 1: TF Card Communication API Implementation
 * =============================================================================
 */

tf_status_t tf_card_init(void)
{
    if (tf_initialized)
    {
        LOG_W("TF card already initialized");
        return TF_STATUS_OK;
    }

    /* Create mutex for thread safety */
    tf_mutex = rt_mutex_create("tf_mutex", RT_IPC_FLAG_PRIO);
    if (tf_mutex == RT_NULL)
    {
        LOG_E("Failed to create TF mutex");
        return TF_STATUS_ERROR;
    }

    /* Wait for file system mount (done by INIT_APP_EXPORT) */
    rt_thread_mdelay(100);

    /* Check if file system is mounted */
    if (!tf_is_mounted())
    {
        LOG_E("TF card not mounted");
        rt_mutex_delete(tf_mutex);
        tf_mutex = RT_NULL;
        return TF_STATUS_NOT_MOUNTED;
    }

    tf_initialized = RT_TRUE;
    LOG_I("TF card initialized successfully");

    return TF_STATUS_OK;
}

tf_status_t tf_card_deinit(void)
{
    if (!tf_initialized)
        return TF_STATUS_OK;

    tf_lock();

    /* Close any open file */
    if (current_file_fd >= 0)
    {
        close(current_file_fd);
        current_file_fd = -1;
    }

    tf_unlock();

    /* Delete mutex */
    if (tf_mutex != RT_NULL)
    {
        rt_mutex_delete(tf_mutex);
        tf_mutex = RT_NULL;
    }

    tf_initialized = RT_FALSE;
    LOG_I("TF card deinitialized");

    return TF_STATUS_OK;
}

rt_bool_t tf_card_is_ready(void)
{
    if (!tf_initialized)
        return RT_FALSE;

    return tf_is_mounted();
}

tf_status_t tf_card_get_info(tf_card_info_t *info)
{
    struct statfs fs_stat;

    if (info == RT_NULL)
        return TF_STATUS_INVALID_PARAM;

    if (!tf_initialized)
        return TF_STATUS_NOT_MOUNTED;

    tf_lock();

    rt_memset(info, 0, sizeof(tf_card_info_t));

    if (dfs_statfs(TF_MOUNT_POINT, &fs_stat) == 0)
    {
        info->sector_size = fs_stat.f_bsize;
        info->total_sectors = fs_stat.f_blocks;
        info->total_size_mb = (rt_uint32_t)((rt_uint64_t)fs_stat.f_blocks *
                              fs_stat.f_bsize / (1024 * 1024));
        info->free_size_mb = (rt_uint32_t)((rt_uint64_t)fs_stat.f_bfree *
                             fs_stat.f_bsize / (1024 * 1024));
        info->mounted = RT_TRUE;
    }
    else
    {
        info->mounted = RT_FALSE;
        tf_unlock();
        return TF_STATUS_NOT_MOUNTED;
    }

    tf_unlock();
    return TF_STATUS_OK;
}

tf_status_t tf_card_test(void)
{
    int fd;
    char write_buf[32] = "TF Card Test Data 12345678";
    char read_buf[32] = {0};
    int written, read_bytes;
    tf_status_t status = TF_STATUS_OK;

    if (!tf_initialized)
    {
        LOG_E("TF card not initialized");
        return TF_STATUS_NOT_MOUNTED;
    }

    tf_lock();

    LOG_I("=== TF Card Test Start ===");

    /* Test 1: Write test */
    LOG_I("Test 1: Write test...");
    fd = open(TF_TEST_FILE, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0)
    {
        LOG_E("Failed to create test file");
        status = TF_STATUS_OPEN_FAILED;
        goto test_end;
    }

    written = write(fd, write_buf, sizeof(write_buf));
    close(fd);

    if (written != sizeof(write_buf))
    {
        LOG_E("Write failed: expected %d, wrote %d", sizeof(write_buf), written);
        status = TF_STATUS_WRITE_FAILED;
        goto test_cleanup;
    }
    LOG_I("Test 1: PASSED (wrote %d bytes)", written);

    /* Test 2: Read test */
    LOG_I("Test 2: Read test...");
    fd = open(TF_TEST_FILE, O_RDONLY);
    if (fd < 0)
    {
        LOG_E("Failed to open test file for reading");
        status = TF_STATUS_OPEN_FAILED;
        goto test_cleanup;
    }

    read_bytes = read(fd, read_buf, sizeof(read_buf));
    close(fd);

    if (read_bytes != sizeof(write_buf))
    {
        LOG_E("Read failed: expected %d, read %d", sizeof(write_buf), read_bytes);
        status = TF_STATUS_READ_FAILED;
        goto test_cleanup;
    }
    LOG_I("Test 2: PASSED (read %d bytes)", read_bytes);

    /* Test 3: Data integrity */
    LOG_I("Test 3: Data integrity...");
    if (rt_memcmp(write_buf, read_buf, sizeof(write_buf)) != 0)
    {
        LOG_E("Data integrity check failed");
        status = TF_STATUS_ERROR;
        goto test_cleanup;
    }
    LOG_I("Test 3: PASSED");

test_cleanup:
    /* Remove test file */
    unlink(TF_TEST_FILE);

test_end:
    if (status == TF_STATUS_OK)
    {
        LOG_I("=== TF Card Test: ALL PASSED ===");
    }
    else
    {
        LOG_E("=== TF Card Test: FAILED ===");
    }

    tf_unlock();
    return status;
}

/*
 * =============================================================================
 * Stage 2: Data Storage API Implementation
 * =============================================================================
 */

tf_status_t tf_data_init(void)
{
    struct stat st;

    if (!tf_initialized)
    {
        LOG_E("TF card not initialized");
        return TF_STATUS_NOT_MOUNTED;
    }

    tf_lock();

    /* Check if log directory exists */
    if (stat(TF_LOG_DIR, &st) != 0)
    {
        /* Create directory */
        if (mkdir(TF_LOG_DIR, 0777) != 0)
        {
            LOG_E("Failed to create log directory: %s", TF_LOG_DIR);
            tf_unlock();
            return TF_STATUS_ERROR;
        }
        LOG_I("Created log directory: %s", TF_LOG_DIR);
    }

    tf_unlock();
    LOG_I("Data storage initialized");
    return TF_STATUS_OK;
}

/**
 * @brief Get today's log filename based on timestamp
 */
static void tf_get_daily_filename(rt_uint32_t timestamp, char *filename, rt_size_t size)
{
    struct tm *tm_info;
    time_t ts = (time_t)timestamp;
    
    /* Use standard time functions for accurate conversion */
    tm_info = gmtime(&ts);
    if (tm_info == RT_NULL)
    {
        /* Fallback: use simplified calculation */
        rt_uint32_t days = timestamp / 86400;
        rt_uint32_t year = 1970 + (days / 365);
        rt_uint32_t day_of_year = days % 365;
        rt_uint32_t month = (day_of_year / 30) + 1;
        rt_uint32_t day = (day_of_year % 30) + 1;

        rt_snprintf(filename, size, "%s/%04d%02d%02d.csv",
                    TF_LOG_DIR, year, month, day);
        return;
    }

    rt_snprintf(filename, size, "%s/%04d%02d%02d.csv",
                TF_LOG_DIR, 
                tm_info->tm_year + 1900, 
                tm_info->tm_mon + 1, 
                tm_info->tm_mday);
}

/**
 * @brief Get session filename with second precision
 */
void tf_get_session_filename(rt_uint32_t timestamp, char *filename, rt_size_t size)
{
    struct tm *tm_info;
    time_t ts = (time_t)timestamp;
    
    /* Use standard time functions for accurate conversion */
    tm_info = gmtime(&ts);
    if (tm_info == RT_NULL)
    {
        /* Fallback: use simplified calculation */
        rt_uint32_t total_seconds = timestamp;
        rt_uint32_t days = total_seconds / 86400;
        rt_uint32_t seconds_in_day = total_seconds % 86400;
        
        rt_uint32_t year = 1970 + (days / 365);
        rt_uint32_t day_of_year = days % 365;
        rt_uint32_t month = (day_of_year / 30) + 1;
        rt_uint32_t day = (day_of_year % 30) + 1;
        
        rt_uint32_t hours = seconds_in_day / 3600;
        rt_uint32_t minutes = (seconds_in_day % 3600) / 60;
        rt_uint32_t seconds = seconds_in_day % 60;

        rt_snprintf(filename, size, "%s/%04d%02d%02d_%02d%02d%02d_session.csv",
                    TF_LOG_DIR, year, month, day, hours, minutes, seconds);
        return;
    }
    
    rt_snprintf(filename, size, "%s/%04d%02d%02d_%02d%02d%02d_session.csv",
                TF_LOG_DIR, 
                tm_info->tm_year + 1900, 
                tm_info->tm_mon + 1, 
                tm_info->tm_mday,
                tm_info->tm_hour,
                tm_info->tm_min,
                tm_info->tm_sec);
}

tf_status_t tf_data_write_record(const tf_co2_record_t *record)
{
    char filename[64];
    char line[128];
    int fd;
    int len, written;
    struct stat st;
    rt_bool_t new_file = RT_FALSE;
    struct tm *tm_info;
    time_t ts;

    if (record == RT_NULL)
        return TF_STATUS_INVALID_PARAM;

    if (!tf_initialized)
        return TF_STATUS_NOT_MOUNTED;

    tf_lock();

    /* Get filename for today based on RTC timestamp */
    tf_get_daily_filename(record->rtc_timestamp, filename, sizeof(filename));

    /* Check if file exists */
    if (stat(filename, &st) != 0)
    {
        new_file = RT_TRUE;
    }

    /* Open file for appending */
    fd = open(filename, O_WRONLY | O_CREAT | O_APPEND);
    if (fd < 0)
    {
        LOG_E("Failed to open log file: %s", filename);
        tf_unlock();
        return TF_STATUS_OPEN_FAILED;
    }

    /* Write CSV header if new file */
    if (new_file)
    {
        const char *header = "datetime,elapsed_seconds,co2_ppm\n";
        write(fd, header, rt_strlen(header));
    }

    /* Convert Unix timestamp to readable datetime format (YYYYMMDDHHMMSS) */
    ts = (time_t)record->rtc_timestamp;
    tm_info = gmtime(&ts);
    
    /* Format record as CSV line with datetime format */
    if (tm_info != RT_NULL)
    {
        len = rt_snprintf(line, sizeof(line), "%04d%02d%02d%02d%02d%02d,%lu,%u\n",
                          tm_info->tm_year + 1900,
                          tm_info->tm_mon + 1,
                          tm_info->tm_mday,
                          tm_info->tm_hour,
                          tm_info->tm_min,
                          tm_info->tm_sec,
                          record->elapsed_seconds,
                          record->co2_ppm);
    }
    else
    {
        /* Fallback: use timestamp if conversion fails */
        len = rt_snprintf(line, sizeof(line), "%lu,%lu,%u\n",
                          record->rtc_timestamp,
                          record->elapsed_seconds,
                          record->co2_ppm);
    }

    /* Write record */
    written = write(fd, line, len);
    close(fd);

    if (written != len)
    {
        LOG_E("Write failed: expected %d, wrote %d", len, written);
        tf_unlock();
        return TF_STATUS_WRITE_FAILED;
    }

    tf_unlock();
    return TF_STATUS_OK;
}

tf_status_t tf_data_write_records(const tf_co2_record_t *records, rt_size_t count)
{
    rt_size_t i;
    tf_status_t status;

    if (records == RT_NULL || count == 0)
        return TF_STATUS_INVALID_PARAM;

    for (i = 0; i < count; i++)
    {
        status = tf_data_write_record(&records[i]);
        if (status != TF_STATUS_OK)
        {
            LOG_E("Failed to write record %d/%d", i + 1, count);
            return status;
        }
    }

    return TF_STATUS_OK;
}

tf_status_t tf_data_flush(void)
{
    /* In the current implementation, each write is immediately flushed */
    /* This function is provided for future buffered implementation */
    return TF_STATUS_OK;
}

/*
 * =============================================================================
 * Stage 3: Structured Data API Implementation
 * =============================================================================
 */

tf_status_t tf_file_create(const char *filename, rt_uint16_t interval_sec)
{
    char filepath[64];
    int fd;
    tf_file_header_t header;
    int written;

    if (filename == RT_NULL)
        return TF_STATUS_INVALID_PARAM;

    if (!tf_initialized)
        return TF_STATUS_NOT_MOUNTED;

    tf_lock();

    /* Build full path */
    rt_snprintf(filepath, sizeof(filepath), "%s/%s", TF_LOG_DIR, filename);

    /* Create file */
    fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0)
    {
        LOG_E("Failed to create file: %s", filepath);
        tf_unlock();
        return TF_STATUS_OPEN_FAILED;
    }

    /* Initialize header */
    rt_memset(&header, 0, sizeof(header));
    header.magic = TF_FILE_MAGIC;
    header.version = TF_FILE_VERSION;
    header.record_size = sizeof(tf_co2_record_t);
    header.record_count = 0;
    header.start_timestamp = 0;
    header.end_timestamp = 0;
    header.interval_sec = interval_sec;

    /* Write header */
    written = write(fd, &header, sizeof(header));
    close(fd);

    if (written != sizeof(header))
    {
        LOG_E("Failed to write header");
        unlink(filepath);
        tf_unlock();
        return TF_STATUS_WRITE_FAILED;
    }

    tf_unlock();
    LOG_I("Created data file: %s", filepath);
    return TF_STATUS_OK;
}

tf_status_t tf_file_open(const char *filename, tf_file_header_t *header)
{
    char filepath[64];
    int fd;
    int read_bytes;

    if (filename == RT_NULL)
        return TF_STATUS_INVALID_PARAM;

    if (!tf_initialized)
        return TF_STATUS_NOT_MOUNTED;

    tf_lock();

    /* Close any previously opened file */
    if (current_file_fd >= 0)
    {
        close(current_file_fd);
        current_file_fd = -1;
    }

    /* Build full path */
    rt_snprintf(filepath, sizeof(filepath), "%s/%s", TF_LOG_DIR, filename);

    /* Open file */
    fd = open(filepath, O_RDONLY);
    if (fd < 0)
    {
        LOG_E("Failed to open file: %s", filepath);
        tf_unlock();
        return TF_STATUS_NOT_FOUND;
    }

    /* Read header if requested */
    if (header != RT_NULL)
    {
        read_bytes = read(fd, header, sizeof(tf_file_header_t));
        if (read_bytes != sizeof(tf_file_header_t))
        {
            close(fd);
            tf_unlock();
            return TF_STATUS_READ_FAILED;
        }

        /* Validate magic number */
        if (header->magic != TF_FILE_MAGIC)
        {
            LOG_E("Invalid file format (magic: 0x%08X)", header->magic);
            close(fd);
            tf_unlock();
            return TF_STATUS_ERROR;
        }
    }

    current_file_fd = fd;
    tf_unlock();

    return TF_STATUS_OK;
}

tf_status_t tf_file_close(void)
{
    tf_lock();

    if (current_file_fd >= 0)
    {
        close(current_file_fd);
        current_file_fd = -1;
    }

    tf_unlock();
    return TF_STATUS_OK;
}

tf_status_t tf_file_read_records(tf_co2_record_t *records,
                                  rt_uint32_t start_index,
                                  rt_size_t count,
                                  rt_size_t *actual_count)
{
    off_t offset;
    int read_bytes;
    rt_size_t bytes_to_read;

    if (records == RT_NULL || actual_count == RT_NULL || count == 0)
        return TF_STATUS_INVALID_PARAM;

    if (current_file_fd < 0)
        return TF_STATUS_ERROR;

    tf_lock();

    *actual_count = 0;

    /* Calculate offset (skip header + previous records) */
    offset = sizeof(tf_file_header_t) + (start_index * sizeof(tf_co2_record_t));

    /* Seek to position */
    if (lseek(current_file_fd, offset, SEEK_SET) < 0)
    {
        tf_unlock();
        return TF_STATUS_ERROR;
    }

    /* Read records */
    bytes_to_read = count * sizeof(tf_co2_record_t);
    read_bytes = read(current_file_fd, records, bytes_to_read);

    if (read_bytes < 0)
    {
        tf_unlock();
        return TF_STATUS_READ_FAILED;
    }

    *actual_count = read_bytes / sizeof(tf_co2_record_t);

    tf_unlock();
    return TF_STATUS_OK;
}

tf_status_t tf_file_list(tf_file_list_callback callback)
{
    DIR *dir;
    struct dirent *entry;
    tf_file_header_t header;
    char filepath[64];
    int fd;

    if (callback == RT_NULL)
        return TF_STATUS_INVALID_PARAM;

    if (!tf_initialized)
        return TF_STATUS_NOT_MOUNTED;

    tf_lock();

    dir = opendir(TF_LOG_DIR);
    if (dir == RT_NULL)
    {
        LOG_W("Log directory not found");
        tf_unlock();
        return TF_STATUS_NOT_FOUND;
    }

    while ((entry = readdir(dir)) != RT_NULL)
    {
        /* Skip . and .. */
        if (entry->d_name[0] == '.')
            continue;

        /* Build full path */
        rt_snprintf(filepath, sizeof(filepath), "%s/%s", TF_LOG_DIR, entry->d_name);

        /* Try to read header for binary files */
        fd = open(filepath, O_RDONLY);
        if (fd >= 0)
        {
            if (read(fd, &header, sizeof(header)) == sizeof(header) &&
                header.magic == TF_FILE_MAGIC)
            {
                callback(entry->d_name, header.record_count);
            }
            else
            {
                /* CSV file or unknown format */
                callback(entry->d_name, 0);
            }
            close(fd);
        }
    }

    closedir(dir);
    tf_unlock();

    return TF_STATUS_OK;
}

/*
 * =============================================================================
 * Stage 4: Serial Communication API Implementation
 * =============================================================================
 */

tf_status_t tf_serial_send_file(const char *filename, const char *serial_device)
{
    rt_device_t serial;
    char filepath[64];
    int fd;
    char buffer[128];
    int read_bytes;

    if (filename == RT_NULL || serial_device == RT_NULL)
        return TF_STATUS_INVALID_PARAM;

    if (!tf_initialized)
        return TF_STATUS_NOT_MOUNTED;

    /* Find serial device */
    serial = rt_device_find(serial_device);
    if (serial == RT_NULL)
    {
        LOG_E("Serial device not found: %s", serial_device);
        return TF_STATUS_ERROR;
    }

    tf_lock();

    /* Build full path */
    rt_snprintf(filepath, sizeof(filepath), "%s/%s", TF_LOG_DIR, filename);

    /* Open file */
    fd = open(filepath, O_RDONLY);
    if (fd < 0)
    {
        LOG_E("Failed to open file: %s", filepath);
        tf_unlock();
        return TF_STATUS_NOT_FOUND;
    }

    /* Send start marker */
    rt_device_write(serial, 0, "<<<FILE_START>>>\r\n", 18);
    rt_device_write(serial, 0, filename, rt_strlen(filename));
    rt_device_write(serial, 0, "\r\n", 2);

    /* Send file content */
    while ((read_bytes = read(fd, buffer, sizeof(buffer))) > 0)
    {
        rt_device_write(serial, 0, buffer, read_bytes);
    }

    /* Send end marker */
    rt_device_write(serial, 0, "\r\n<<<FILE_END>>>\r\n", 18);

    close(fd);
    tf_unlock();

    LOG_I("File sent: %s", filename);
    return TF_STATUS_OK;
}

tf_status_t tf_serial_receive_file(const char *filename, const char *serial_device)
{
    /* Placeholder for future implementation */
    /* This requires a more complex protocol with ACK/NAK */
    LOG_W("tf_serial_receive_file not yet implemented");
    return TF_STATUS_ERROR;
}

tf_status_t tf_serial_send_record(const tf_co2_record_t *record,
                                   const char *serial_device)
{
    rt_device_t serial;
    char buffer[128];
    int len;

    if (record == RT_NULL || serial_device == RT_NULL)
        return TF_STATUS_INVALID_PARAM;

    /* Find serial device */
    serial = rt_device_find(serial_device);
    if (serial == RT_NULL)
    {
        LOG_E("Serial device not found: %s", serial_device);
        return TF_STATUS_ERROR;
    }

    /* Format record as JSON with new format */
    len = rt_snprintf(buffer, sizeof(buffer),
                      "{\"ts\":%lu,\"co2\":%u,\"elapsed\":%lu}\r\n",
                      record->rtc_timestamp,
                      record->co2_ppm,
                      record->elapsed_seconds);

    /* Send to serial */
    rt_device_write(serial, 0, buffer, len);

    return TF_STATUS_OK;
}

tf_status_t tf_serial_export_csv(const char *filename, const char *serial_device)
{
    rt_device_t serial;
    char filepath[64];
    int fd;
    char buffer[256];
    int read_bytes;

    if (filename == RT_NULL || serial_device == RT_NULL)
        return TF_STATUS_INVALID_PARAM;

    if (!tf_initialized)
        return TF_STATUS_NOT_MOUNTED;

    /* Find serial device */
    serial = rt_device_find(serial_device);
    if (serial == RT_NULL)
    {
        LOG_E("Serial device not found: %s", serial_device);
        return TF_STATUS_ERROR;
    }

    tf_lock();

    /* Build full path */
    rt_snprintf(filepath, sizeof(filepath), "%s/%s", TF_LOG_DIR, filename);

    /* Check if it's a CSV file already */
    if (rt_strstr(filename, ".csv") != RT_NULL)
    {
        /* Direct send */
        fd = open(filepath, O_RDONLY);
        if (fd < 0)
        {
            tf_unlock();
            return TF_STATUS_NOT_FOUND;
        }

        /* Send CSV content */
        while ((read_bytes = read(fd, buffer, sizeof(buffer))) > 0)
        {
            rt_device_write(serial, 0, buffer, read_bytes);
        }

        close(fd);
    }
    else
    {
        /* Binary file - convert to CSV */
        tf_file_header_t header;
        tf_co2_record_t records[10];
        rt_size_t actual;
        rt_uint32_t index = 0;

        if (tf_file_open(filename, &header) != TF_STATUS_OK)
        {
            tf_unlock();
            return TF_STATUS_NOT_FOUND;
        }

        /* Send CSV header with new format */
        rt_device_write(serial, 0, "rtc_timestamp,elapsed_seconds,co2_ppm\r\n", 36);

        /* Read and convert records */
        while (index < header.record_count)
        {
            if (tf_file_read_records(records, index, 10, &actual) != TF_STATUS_OK)
                break;

            for (rt_size_t i = 0; i < actual; i++)
            {
                int len = rt_snprintf(buffer, sizeof(buffer),
                                      "%lu,%lu,%u\r\n",
                                      records[i].rtc_timestamp,
                                      records[i].elapsed_seconds,
                                      records[i].co2_ppm);
                rt_device_write(serial, 0, buffer, len);
            }

            index += actual;
        }

        tf_file_close();
    }

    tf_unlock();
    LOG_I("CSV export complete: %s", filename);
    return TF_STATUS_OK;
}

/*
 * =============================================================================
 * TF Card Monitor API Implementation (Persistent State)
 * =============================================================================
 */

/* External reference to S8 sensor device */
extern s8_sensor_device_t *g_main_s8_device;

/**
 * @brief Detect if power outage occurred based on RTC time
 * Only called during initialization, not during normal monitor operations
 */
static rt_bool_t tf_detect_power_outage(time_t current_time)
{
    static time_t system_boot_time = 0;

    /* Only detect on first system call (during initialization) */
    if (system_boot_time == 0) {
        system_boot_time = current_time;

        /* Check if RTC time is before reasonable startup time (before 2020) */
        if (current_time < 1577836800) {  /* 2020-01-01 00:00:00 UTC */
            LOG_W("[TF Monitor] RTC time appears reset - likely power outage");
            return RT_TRUE;   /* Power outage detected */
        }

        return RT_FALSE;  /* Normal startup, no power outage */
    }

    /* Subsequent calls - this should not happen in current design */
    return RT_FALSE;  /* Normal operation */
}

/**
 * @brief Initialize TF monitor state structure
 */
tf_status_t tf_monitor_init(tf_monitor_state_t *monitor_state)
{
    time_t current_time;
    rt_device_t rtc_dev;

    if (monitor_state == RT_NULL)
        return TF_STATUS_INVALID_PARAM;

    /* Clear all fields */
    rt_memset(monitor_state, 0, sizeof(tf_monitor_state_t));
    monitor_state->session_file_fd = -1;  /* No open file */
    monitor_state->interval_sec = 5;      /* Default 5 seconds */
    monitor_state->power_outage_detected = RT_FALSE;  /* Default: no outage */

    /* Get current RTC time */
    rtc_dev = rt_device_find("rtc");
    if (rtc_dev != RT_NULL) {
        if (rt_device_open(rtc_dev, RT_DEVICE_OFLAG_RDWR) == RT_EOK) {
            if (rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_GET_TIME, &current_time) == RT_EOK) {
                monitor_state->last_sample_time = current_time;
                monitor_state->rtc_backup_time = current_time;

                /* Check for RTC reset during system initialization */
                /* RTC time before 2020-01-01 indicates power loss/reset */
                if (current_time < 1577836800) {  /* 2020-01-01 00:00:00 UTC */
                    LOG_W("[TF Monitor] RTC time appears reset (before 2020) - using backup timestamp strategy");
                    monitor_state->power_outage_detected = RT_TRUE;
                    /* Set a reasonable current time for session continuity */
                    current_time = 1704067200;  /* 2024-01-01 00:00:00 UTC */
                    monitor_state->rtc_backup_time = current_time;
                }
            }
            rt_device_close(rtc_dev);
        }
    }

    LOG_D("TF monitor state initialized (power outage: %s)",
          monitor_state->power_outage_detected ? "DETECTED" : "none");
    return TF_STATUS_OK;
}

/**
 * @brief Thread entry for persistent TF monitoring
 */
static void tf_persistent_monitor_thread_entry(void *parameter)
{
    tf_monitor_state_t *state = (tf_monitor_state_t *)parameter;
    tf_co2_record_t record;
    char line[128];
    int written;
    static rt_device_t rtc_dev = RT_NULL;

    if (state == RT_NULL)
        return;

    /* Find RTC device */
    if (rtc_dev == RT_NULL)
    {
        rtc_dev = rt_device_find("rtc");
        if (rtc_dev == RT_NULL)
        {
            LOG_E("[TF Monitor] Error: RTC device not found");
            state->running = RT_FALSE;
            return;
        }

        if (rt_device_open(rtc_dev, RT_DEVICE_OFLAG_RDWR) != RT_EOK)
        {
            LOG_E("[TF Monitor] Error: Failed to open RTC device");
            state->running = RT_FALSE;
            return;
        }
    }

    /* Get session start time */
    if (rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_GET_TIME, &state->session_start_time) != RT_EOK)
    {
        LOG_E("[TF Monitor] Error: Failed to get RTC time");
        rt_device_close(rtc_dev);
        state->running = RT_FALSE;
        return;
    }

    /* session_file should already be set in tf_monitor_start */
    /* If not set (empty), generate it now */
    if (state->session_file[0] == '\0')
    {
        tf_get_session_filename(state->session_start_time, state->session_file, sizeof(state->session_file));
        LOG_D("Generated session filename: %s", state->session_file);
    }
    else
    {
        LOG_D("Using existing session filename: %s", state->session_file);
    }

    /* Ensure log directory exists */
    struct stat st;
    if (stat(TF_LOG_DIR, &st) != 0)
    {
        /* Create directory if it doesn't exist */
        if (mkdir(TF_LOG_DIR, 0777) != 0)
        {
            LOG_E("[TF Monitor] Failed to create log directory: %s", TF_LOG_DIR);
            rt_device_close(rtc_dev);
            state->running = RT_FALSE;
            return;
        }
        LOG_I("[TF Monitor] Created log directory: %s", TF_LOG_DIR);
    }

    /* Open session file for appending (kept open during monitoring) */
    state->session_file_fd = open(state->session_file, O_WRONLY | O_CREAT | O_APPEND);
    if (state->session_file_fd < 0)
    {
        LOG_E("[TF Monitor] Failed to open session file: %s", state->session_file);
        LOG_E("[TF Monitor] Error code: %d", errno);
        rt_device_close(rtc_dev);
        state->running = RT_FALSE;
        return;
    }

    LOG_I("TF monitor started (interval: %lu sec)", state->interval_sec);
    if (state->power_outage_detected)
    {
        LOG_I("Post-outage session file: %s", state->session_file);
    }
    else
    {
        LOG_I("Session file: %s", state->session_file);
    }

    while (state->running)
    {
        if (g_main_s8_device != RT_NULL)
        {
            /* Read sensor */
            if (s8_read_co2_data(g_main_s8_device) == S8_STATUS_OK)
            {
                /* Get current RTC time or use backup if RTC is reset */
                time_t current_rtc_time;
                if (rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_GET_TIME, &current_rtc_time) == RT_EOK)
                {
                    /* Use backup time strategy if RTC appears reset */
                    if (current_rtc_time < 1577836800) {  /* Before 2020-01-01 */
                        /* Use backup timestamp based on session duration */
                        current_rtc_time = state->rtc_backup_time + (state->sample_count * state->interval_sec);
                        state->rtc_backup_time = current_rtc_time;
                    }

                    /* Build record */
                    record.rtc_timestamp = current_rtc_time;
                    record.elapsed_seconds = state->sample_count * state->interval_sec;
                    record.co2_ppm = g_main_s8_device->data.co2_ppm;

                    /* Write to session file (kept open) */
                    if (state->session_file_fd >= 0)
                    {
                        written = rt_snprintf(line, sizeof(line), "%lu,%lu,%u\n",
                                              record.rtc_timestamp,
                                              record.elapsed_seconds,
                                              record.co2_ppm);

                        if (write(state->session_file_fd, line, written) == written)
                        {
                            /* Force sync EVERY sample for battery operation */
                            fsync(state->session_file_fd);

                            state->sample_count++;

                            /* Update NVS state every 10 samples for persistence */
                            if (state->sample_count % 10 == 0)
                            {
                                nvs_state_update(state->sample_count);
                                rt_kprintf("Logged %lu samples (state saved)\n", state->sample_count);
                            }
                            else if (state->sample_count % 5 == 0)
                            {
                                rt_kprintf("Logged %lu samples (immediate sync)\n", state->sample_count);
                            }
                        }
                        else
                        {
                            LOG_E("Failed to write to session file");
                        }
                    }
                }
                else
                {
                    LOG_E("Failed to get RTC time");
                }
            }
            else
            {
                LOG_W("Failed to read S8 sensor");
            }
        }
        else
        {
            LOG_W("S8 sensor not available");
        }

        rt_thread_mdelay(state->interval_sec * 1000);
    }

    /* Clean shutdown */
    rt_kprintf("TF monitor stopped (total: %lu samples)\n", state->sample_count);
    rt_kprintf("Session file: %s\n", state->session_file);

    /* Mark as normally stopped in NVS */
    nvs_state_mark_stopped();

    /* Close session file with final sync */
    if (state->session_file_fd >= 0)
    {
        fsync(state->session_file_fd);  /* Final sync */
        close(state->session_file_fd);
        state->session_file_fd = -1;
    }

    /* Close RTC device */
    if (rtc_dev != RT_NULL)
    {
        rt_device_close(rtc_dev);
    }

    LOG_I("TF monitor shutdown complete");
}

/**
 * @brief Start TF monitoring with persistent state
 */
tf_status_t tf_monitor_start(tf_monitor_state_t *monitor_state, rt_uint32_t interval_sec)
{
    time_t session_start;
    char base_filename[64];

    if (monitor_state == RT_NULL)
        return TF_STATUS_INVALID_PARAM;

    if (monitor_state->running)
    {
        LOG_W("TF monitor already running");
        return TF_STATUS_OK;  /* Already running */
    }

    if (!tf_card_is_ready())
    {
        LOG_E("TF card not ready");
        return TF_STATUS_NOT_MOUNTED;
    }

    /* Get current time for session start */
    time(&session_start);

    /* Generate session filename */
    tf_get_session_filename(session_start, monitor_state->session_file, sizeof(monitor_state->session_file));

    /* Extract base filename (without path and extension) for NVS state */
    const char *filename_only = strrchr(monitor_state->session_file, '/');
    if (filename_only != RT_NULL)
        filename_only++;  /* Skip the '/' */
    else
        filename_only = monitor_state->session_file;

    rt_strncpy(base_filename, filename_only, sizeof(base_filename) - 1);
    base_filename[sizeof(base_filename) - 1] = '\0';

    /* Mark as started in NVS (not exited normally) */
    if (nvs_state_mark_started(base_filename, interval_sec, session_start) != RT_EOK)
    {
        LOG_W("Failed to save NVS state - continuing anyway");
    }

    monitor_state->interval_sec = interval_sec;
    monitor_state->running = RT_TRUE;
    monitor_state->sample_count = 0;

    /* Create monitor thread */
    monitor_state->monitor_thread = rt_thread_create("tf_mon_persist",
                                                     tf_persistent_monitor_thread_entry,
                                                     monitor_state,
                                                     2048,
                                                     20,
                                                     10);

    if (monitor_state->monitor_thread == RT_NULL)
    {
        monitor_state->running = RT_FALSE;
        nvs_state_mark_stopped();  /* Clean up NVS state */
        LOG_E("Failed to create TF monitor thread");
        return TF_STATUS_ERROR;
    }

    rt_thread_startup(monitor_state->monitor_thread);
    LOG_I("TF monitor started successfully");

    return TF_STATUS_OK;
}

/**
 * @brief Stop TF monitoring with persistent state
 */
tf_status_t tf_monitor_stop(tf_monitor_state_t *monitor_state)
{
    if (monitor_state == RT_NULL)
        return TF_STATUS_INVALID_PARAM;

    if (!monitor_state->running)
    {
        LOG_W("TF monitor not running");
        return TF_STATUS_OK;  /* Already stopped */
    }

    /* Signal thread to stop */
    monitor_state->running = RT_FALSE;

    LOG_I("Stopping TF monitor...");

    /* Wait for thread to finish */
    if (monitor_state->monitor_thread != RT_NULL)
    {
        rt_thread_delete(monitor_state->monitor_thread);
        monitor_state->monitor_thread = RT_NULL;
    }

    LOG_I("TF monitor stopped");
    return TF_STATUS_OK;
}

/**
 * @brief Get TF monitor status
 */
rt_bool_t tf_monitor_is_running(tf_monitor_state_t *monitor_state)
{
    if (monitor_state == RT_NULL)
        return RT_FALSE;

    return monitor_state->running;
}

/**
 * @brief Emergency save and shutdown (called when power failure detected)
 */
tf_status_t tf_monitor_emergency_shutdown(tf_monitor_state_t *monitor_state)
{
    if (monitor_state == RT_NULL)
        return TF_STATUS_INVALID_PARAM;

    if (!monitor_state->running)
        return TF_STATUS_OK;  /* Already stopped */

    LOG_W("[TF Monitor] Emergency shutdown triggered - saving data...");

    /* Signal thread to stop */
    monitor_state->running = RT_FALSE;

    /* Emergency sync and close session file */
    if (monitor_state->session_file_fd >= 0)
    {
        /* Force immediate sync */
        fsync(monitor_state->session_file_fd);

        /* Add emergency shutdown marker to file */
        const char *shutdown_marker = "# EMERGENCY_SHUTDOWN - Power Failure Detected\n";
        write(monitor_state->session_file_fd, shutdown_marker, rt_strlen(shutdown_marker));
        fsync(monitor_state->session_file_fd);  /* Final sync */

        close(monitor_state->session_file_fd);
        monitor_state->session_file_fd = -1;

        LOG_W("[TF Monitor] Emergency shutdown complete - data saved");
    }

    return TF_STATUS_OK;
}

/**
 * @brief Get session duration for backup timestamp calculation
 */
rt_uint32_t tf_monitor_get_session_duration(tf_monitor_state_t *monitor_state)
{
    if (monitor_state == RT_NULL)
        return 0;

    return monitor_state->sample_count * monitor_state->interval_sec;
}

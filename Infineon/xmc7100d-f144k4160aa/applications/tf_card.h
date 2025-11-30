/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-27     Developer    TF Card driver header
 */

#ifndef __TF_CARD_H__
#define __TF_CARD_H__

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * =============================================================================
 * TF Card Status Codes
 * =============================================================================
 */
typedef enum {
    TF_STATUS_OK = 0,           /* Operation successful */
    TF_STATUS_ERROR,            /* General error */
    TF_STATUS_NOT_MOUNTED,      /* File system not mounted */
    TF_STATUS_OPEN_FAILED,      /* Failed to open file */
    TF_STATUS_WRITE_FAILED,     /* Failed to write data */
    TF_STATUS_READ_FAILED,      /* Failed to read data */
    TF_STATUS_NO_SPACE,         /* Not enough space on card */
    TF_STATUS_INVALID_PARAM,    /* Invalid parameter */
    TF_STATUS_NOT_FOUND,        /* File not found */
    TF_STATUS_BUSY,             /* Card is busy */
} tf_status_t;

/*
 * =============================================================================
 * TF Card Information Structure
 * =============================================================================
 */
typedef struct {
    rt_uint32_t total_size_mb;      /* Total size in MB */
    rt_uint32_t free_size_mb;       /* Free size in MB */
    rt_uint32_t sector_size;        /* Sector size in bytes */
    rt_uint32_t total_sectors;      /* Total sectors */
    rt_bool_t   mounted;            /* Mount status */
} tf_card_info_t;

/*
 * =============================================================================
 * CO2 Data Record Structure
 * =============================================================================
 */
typedef struct {
    rt_uint32_t rtc_timestamp;      /* RTC Unix timestamp */
    rt_uint32_t elapsed_seconds;    /* Elapsed time from session start (seconds) */
    rt_uint16_t co2_ppm;            /* CO2 concentration in ppm */
} tf_co2_record_t;

/*
 * =============================================================================
 * Data File Header Structure (for structured storage)
 * =============================================================================
 */
#define TF_FILE_MAGIC       0x43433032  /* "CC02" - CO2 file magic */
#define TF_FILE_VERSION     0x0001      /* File format version */

typedef struct {
    rt_uint32_t magic;              /* File magic number */
    rt_uint16_t version;            /* File format version */
    rt_uint16_t record_size;        /* Size of each record */
    rt_uint32_t record_count;       /* Total records in file */
    rt_uint32_t start_timestamp;    /* First record timestamp */
    rt_uint32_t end_timestamp;      /* Last record timestamp */
    rt_uint16_t interval_sec;       /* Recording interval in seconds */
    rt_uint8_t  reserved[18];       /* Reserved for future use */
} tf_file_header_t;

/*
 * =============================================================================
 * TF Card Monitor State Structure (for persistence across power cycles)
 * =============================================================================
 */

/**
 * @brief TF Card monitor state structure
 */
typedef struct {
    rt_bool_t running;                    /* Monitor running state */
    rt_uint32_t interval_sec;             /* Monitoring interval in seconds */
    char session_file[64];                /* Current session filename */
    rt_thread_t monitor_thread;           /* Monitor thread handle */
    rt_uint32_t sample_count;             /* Total samples logged */
    time_t session_start_time;            /* Session start timestamp */
    int session_file_fd;                  /* Open file descriptor (-1 if closed) */
    time_t last_sample_time;              /* Last recorded sample timestamp */
    rt_bool_t power_outage_detected;      /* Power outage detection flag */
    rt_uint32_t session_duration_sec;     /* Total session duration in seconds */
    time_t rtc_backup_time;               /* Backup time when RTC might be reset */
} tf_monitor_state_t;

/*
 * =============================================================================
 * TF Card Monitor API (for persistence)
 * =============================================================================
 */

/**
 * @brief Initialize TF monitor state
 * @param monitor_state Pointer to monitor state structure
 * @return TF_STATUS_OK on success
 */
tf_status_t tf_monitor_init(tf_monitor_state_t *monitor_state);

/**
 * @brief Start TF monitoring with persistent state
 * @param monitor_state Pointer to monitor state structure
 * @param interval_sec Monitoring interval in seconds
 * @return TF_STATUS_OK on success
 */
tf_status_t tf_monitor_start(tf_monitor_state_t *monitor_state, rt_uint32_t interval_sec);

/**
 * @brief Stop TF monitoring with persistent state
 * @param monitor_state Pointer to monitor state structure
 * @return TF_STATUS_OK on success
 */
tf_status_t tf_monitor_stop(tf_monitor_state_t *monitor_state);

/**
 * @brief Get TF monitor status
 * @param monitor_state Pointer to monitor state structure
 * @return RT_TRUE if running, RT_FALSE otherwise
 */
rt_bool_t tf_monitor_is_running(tf_monitor_state_t *monitor_state);

/**
 * @brief Emergency save and shutdown (called when power failure detected)
 * @param monitor_state Pointer to monitor state structure
 * @return TF_STATUS_OK on success
 */
tf_status_t tf_monitor_emergency_shutdown(tf_monitor_state_t *monitor_state);

/**
 * @brief Get session duration for backup timestamp calculation
 * @param monitor_state Pointer to monitor state structure
 * @return Duration in seconds
 */
rt_uint32_t tf_monitor_get_session_duration(tf_monitor_state_t *monitor_state);

/*
 * =============================================================================
 * Stage 1: TF Card Communication API
 * =============================================================================
 */

/**
 * @brief Initialize TF card driver
 * @return TF_STATUS_OK on success
 */
tf_status_t tf_card_init(void);

/**
 * @brief Deinitialize TF card driver
 * @return TF_STATUS_OK on success
 */
tf_status_t tf_card_deinit(void);

/**
 * @brief Check if TF card is ready and mounted
 * @return RT_TRUE if ready, RT_FALSE otherwise
 */
rt_bool_t tf_card_is_ready(void);

/**
 * @brief Get TF card information
 * @param info Pointer to info structure to fill
 * @return TF_STATUS_OK on success
 */
tf_status_t tf_card_get_info(tf_card_info_t *info);

/**
 * @brief Test TF card read/write functionality
 * @return TF_STATUS_OK if all tests pass
 */
tf_status_t tf_card_test(void);

/*
 * =============================================================================
 * Stage 2: Data Storage API
 * =============================================================================
 */

/**
 * @brief Initialize data logging directory
 * @return TF_STATUS_OK on success
 */
tf_status_t tf_data_init(void);

/**
 * @brief Get session filename with second precision
 * @param timestamp Unix timestamp
 * @param filename Buffer to store filename
 * @param size Buffer size
 */
void tf_get_session_filename(rt_uint32_t timestamp, char *filename, rt_size_t size);

/**
 * @brief Write a single CO2 record to today's log file
 * @param record Pointer to CO2 record
 * @return TF_STATUS_OK on success
 */
tf_status_t tf_data_write_record(const tf_co2_record_t *record);

/**
 * @brief Write multiple CO2 records to log file
 * @param records Array of records
 * @param count Number of records
 * @return TF_STATUS_OK on success
 */
tf_status_t tf_data_write_records(const tf_co2_record_t *records, rt_size_t count);

/**
 * @brief Flush any buffered data to card
 * @return TF_STATUS_OK on success
 */
tf_status_t tf_data_flush(void);

/*
 * =============================================================================
 * Stage 3: Structured Data API
 * =============================================================================
 */

/**
 * @brief Create a new structured data file
 * @param filename File name (will be created in /co2_log/)
 * @param interval_sec Recording interval in seconds
 * @return TF_STATUS_OK on success
 */
tf_status_t tf_file_create(const char *filename, rt_uint16_t interval_sec);

/**
 * @brief Open an existing data file for reading
 * @param filename File name
 * @param header Pointer to header structure to fill
 * @return TF_STATUS_OK on success
 */
tf_status_t tf_file_open(const char *filename, tf_file_header_t *header);

/**
 * @brief Close the current data file
 * @return TF_STATUS_OK on success
 */
tf_status_t tf_file_close(void);

/**
 * @brief Read records from data file
 * @param records Buffer to store records
 * @param start_index Starting record index
 * @param count Number of records to read
 * @param actual_count Actual records read
 * @return TF_STATUS_OK on success
 */
tf_status_t tf_file_read_records(tf_co2_record_t *records,
                                  rt_uint32_t start_index,
                                  rt_size_t count,
                                  rt_size_t *actual_count);

/**
 * @brief List all data files
 * @param callback Function to call for each file (filename, record_count)
 * @return TF_STATUS_OK on success
 */
typedef void (*tf_file_list_callback)(const char *filename, rt_uint32_t record_count);
tf_status_t tf_file_list(tf_file_list_callback callback);

/*
 * =============================================================================
 * Stage 4: Serial Communication API (PC Transfer)
 * =============================================================================
 */

/**
 * @brief Start file transfer to PC via serial
 * @param filename File to transfer
 * @param serial_device Serial device name (e.g., "uart4")
 * @return TF_STATUS_OK on success
 */
tf_status_t tf_serial_send_file(const char *filename, const char *serial_device);

/**
 * @brief Start file transfer from PC via serial
 * @param filename Destination file name
 * @param serial_device Serial device name
 * @return TF_STATUS_OK on success
 */
tf_status_t tf_serial_receive_file(const char *filename, const char *serial_device);

/**
 * @brief Send real-time CO2 data to PC
 * @param record CO2 record to send
 * @param serial_device Serial device name
 * @return TF_STATUS_OK on success
 */
tf_status_t tf_serial_send_record(const tf_co2_record_t *record,
                                   const char *serial_device);

/**
 * @brief Export data file as CSV to PC
 * @param filename Data file to export
 * @param serial_device Serial device name
 * @return TF_STATUS_OK on success
 */
tf_status_t tf_serial_export_csv(const char *filename, const char *serial_device);

#ifdef __cplusplus
}
#endif

#endif /* __TF_CARD_H__ */

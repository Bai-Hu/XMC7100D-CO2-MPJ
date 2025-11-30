/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-30     Developer    Non-volatile state storage for power loss recovery
 */

#ifndef __NVS_STATE_H__
#define __NVS_STATE_H__

#include <rtthread.h>
#include <rtdevice.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * =============================================================================
 * Non-Volatile State Magic Numbers and Definitions
 * =============================================================================
 */
#define NVS_STATE_MAGIC         0x544D4F4E  /* "TMON" - TF Monitor magic */
#define NVS_STATE_VERSION       0x0001      /* State format version */
#define NVS_STATE_FILE          "/co2_log/.tf_monitor_state"

/*
 * =============================================================================
 * Non-Volatile Monitor State Structure
 * =============================================================================
 */

/**
 * @brief Non-volatile monitor state structure
 * This structure is saved to TF card and survives power cycles
 */
typedef struct {
    rt_uint32_t magic;                  /* Magic number for validation */
    rt_uint16_t version;                /* State format version */
    rt_uint8_t  running;                /* Was monitor running? (1=yes, 0=no) */
    rt_uint8_t  normal_exit;            /* Did it exit normally? (1=yes, 0=no) */

    rt_uint16_t continuation_count;     /* How many times resumed after power loss */
    rt_uint16_t reserved1;              /* Reserved for alignment */

    rt_uint32_t interval_sec;           /* Monitoring interval in seconds */
    rt_uint32_t sample_count;           /* Total samples in this session */
    rt_uint32_t total_samples;          /* Total samples across all continuations */

    time_t session_start_time;          /* Original session start time */
    time_t last_update_time;            /* Last state update timestamp */
    time_t continuation_start_time;     /* Start time of current continuation */

    char base_filename[64];             /* Base filename (without _XXX.csv) */

    rt_uint32_t crc32;                  /* CRC32 for data integrity */
} nvs_monitor_state_t;

/*
 * =============================================================================
 * Non-Volatile State API
 * =============================================================================
 */

/**
 * @brief Initialize non-volatile state storage
 * @return RT_EOK on success, -RT_ERROR on failure
 */
rt_err_t nvs_state_init(void);

/**
 * @brief Save monitor state to non-volatile storage
 * @param state Pointer to state structure to save
 * @return RT_EOK on success, -RT_ERROR on failure
 */
rt_err_t nvs_state_save(const nvs_monitor_state_t *state);

/**
 * @brief Load monitor state from non-volatile storage
 * @param state Pointer to state structure to fill
 * @return RT_EOK on success, -RT_ERROR on failure, -RT_EEMPTY if no valid state
 */
rt_err_t nvs_state_load(nvs_monitor_state_t *state);

/**
 * @brief Clear monitor state (mark as normal exit)
 * @return RT_EOK on success, -RT_ERROR on failure
 */
rt_err_t nvs_state_clear(void);

/**
 * @brief Check if valid state exists
 * @return RT_TRUE if valid state exists, RT_FALSE otherwise
 */
rt_bool_t nvs_state_exists(void);

/**
 * @brief Mark monitor as started (running, not exited normally)
 * @param base_filename Base filename without extension
 * @param interval_sec Monitoring interval
 * @param session_start_time Session start time
 * @return RT_EOK on success
 */
rt_err_t nvs_state_mark_started(const char *base_filename,
                                 rt_uint32_t interval_sec,
                                 time_t session_start_time);

/**
 * @brief Mark monitor as stopped normally
 * @return RT_EOK on success
 */
rt_err_t nvs_state_mark_stopped(void);

/**
 * @brief Update state during monitoring (sample count, timestamp)
 * @param sample_count Current sample count
 * @return RT_EOK on success
 */
rt_err_t nvs_state_update(rt_uint32_t sample_count);

/**
 * @brief Check if power loss recovery is needed
 * @return RT_TRUE if recovery needed, RT_FALSE otherwise
 */
rt_bool_t nvs_state_needs_recovery(void);

/**
 * @brief Prepare for continuation after power loss
 * Creates new continuation state with incremented count
 * @param state Loaded state from before power loss
 * @return RT_EOK on success
 */
rt_err_t nvs_state_prepare_continuation(nvs_monitor_state_t *state);

/**
 * @brief Get continuation filename
 * @param base_filename Base filename without extension
 * @param continuation_count Continuation count (0 = original, 1+ = continuations)
 * @param output Buffer for output filename
 * @param output_size Size of output buffer
 */
void nvs_state_get_continuation_filename(const char *base_filename,
                                          rt_uint16_t continuation_count,
                                          char *output,
                                          rt_size_t output_size);

#ifdef __cplusplus
}
#endif

#endif /* __NVS_STATE_H__ */

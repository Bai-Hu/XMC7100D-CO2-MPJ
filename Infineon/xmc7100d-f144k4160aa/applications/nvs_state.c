/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-30     Developer    Non-volatile state storage implementation
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_file.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "nvs_state.h"

#define DBG_TAG "nvs_state"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/*
 * =============================================================================
 * CRC32 Implementation (for data integrity)
 * =============================================================================
 */

static const rt_uint32_t crc32_table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
    0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
    0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
    0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
    0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
    0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
    0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
    0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
    0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
    0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
    0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
    0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
    0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
    0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
    0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
    0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
    0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
    0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
    0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
    0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
    0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
    0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

static rt_uint32_t nvs_calculate_crc32(const void *data, rt_size_t size)
{
    rt_uint32_t crc = 0xFFFFFFFF;
    const rt_uint8_t *ptr = (const rt_uint8_t *)data;

    for (rt_size_t i = 0; i < size; i++)
    {
        crc = crc32_table[(crc ^ ptr[i]) & 0xFF] ^ (crc >> 8);
    }

    return ~crc;
}

/*
 * =============================================================================
 * Non-Volatile State API Implementation
 * =============================================================================
 */

rt_err_t nvs_state_init(void)
{
    /* Check if TF card is mounted */
    struct statfs fs_stat;
    if (dfs_statfs("/", &fs_stat) != 0)
    {
        LOG_E("TF card not mounted - state storage unavailable");
        return -RT_ERROR;
    }

    LOG_D("Non-volatile state storage initialized");
    return RT_EOK;
}

rt_err_t nvs_state_save(const nvs_monitor_state_t *state)
{
    int fd;
    nvs_monitor_state_t state_copy;
    int written;

    if (state == RT_NULL)
        return -RT_ERROR;

    /* Make a copy and calculate CRC */
    rt_memcpy(&state_copy, state, sizeof(nvs_monitor_state_t));
    state_copy.crc32 = 0;  /* CRC calculated with this field = 0 */
    state_copy.crc32 = nvs_calculate_crc32(&state_copy,
                                            sizeof(nvs_monitor_state_t) - sizeof(rt_uint32_t));

    /* Open state file for writing (atomic write) */
    fd = open(NVS_STATE_FILE, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0)
    {
        LOG_E("Failed to open state file for writing");
        return -RT_ERROR;
    }

    /* Write state */
    written = write(fd, &state_copy, sizeof(nvs_monitor_state_t));

    /* Force sync to ensure data is on disk */
    fsync(fd);
    close(fd);

    if (written != sizeof(nvs_monitor_state_t))
    {
        LOG_E("Failed to write complete state");
        return -RT_ERROR;
    }

    LOG_D("State saved successfully (running=%d, exit=%d, cont=%d)",
          state_copy.running, state_copy.normal_exit, state_copy.continuation_count);

    return RT_EOK;
}

rt_err_t nvs_state_load(nvs_monitor_state_t *state)
{
    int fd;
    int read_bytes;
    rt_uint32_t calculated_crc;

    if (state == RT_NULL)
        return -RT_ERROR;

    /* Open state file */
    fd = open(NVS_STATE_FILE, O_RDONLY);
    if (fd < 0)
    {
        LOG_D("No state file found");
        return -RT_EEMPTY;
    }

    /* Read state */
    read_bytes = read(fd, state, sizeof(nvs_monitor_state_t));
    close(fd);

    if (read_bytes != sizeof(nvs_monitor_state_t))
    {
        LOG_E("Incomplete state file");
        return -RT_ERROR;
    }

    /* Validate magic number */
    if (state->magic != NVS_STATE_MAGIC)
    {
        LOG_E("Invalid state file (bad magic: 0x%08X)", state->magic);
        return -RT_ERROR;
    }

    /* Validate CRC */
    rt_uint32_t stored_crc = state->crc32;
    state->crc32 = 0;
    calculated_crc = nvs_calculate_crc32(state, sizeof(nvs_monitor_state_t) - sizeof(rt_uint32_t));
    state->crc32 = stored_crc;

    if (calculated_crc != stored_crc)
    {
        LOG_E("State file CRC mismatch (calculated: 0x%08X, stored: 0x%08X)",
              calculated_crc, stored_crc);
        return -RT_ERROR;
    }

    LOG_D("State loaded successfully (running=%d, exit=%d, cont=%d)",
          state->running, state->normal_exit, state->continuation_count);

    return RT_EOK;
}

rt_err_t nvs_state_clear(void)
{
    /* Delete state file */
    if (unlink(NVS_STATE_FILE) == 0)
    {
        LOG_D("State file cleared");
        return RT_EOK;
    }

    /* File might not exist, which is OK */
    return RT_EOK;
}

rt_bool_t nvs_state_exists(void)
{
    struct stat st;
    return (stat(NVS_STATE_FILE, &st) == 0) ? RT_TRUE : RT_FALSE;
}

rt_err_t nvs_state_mark_started(const char *base_filename,
                                 rt_uint32_t interval_sec,
                                 time_t session_start_time)
{
    nvs_monitor_state_t state;

    if (base_filename == RT_NULL)
        return -RT_ERROR;

    /* Initialize state */
    rt_memset(&state, 0, sizeof(nvs_monitor_state_t));
    state.magic = NVS_STATE_MAGIC;
    state.version = NVS_STATE_VERSION;
    state.running = 1;              /* Monitor is running */
    state.normal_exit = 0;          /* Not exited yet */
    state.continuation_count = 0;   /* First session */
    state.interval_sec = interval_sec;
    state.sample_count = 0;
    state.total_samples = 0;
    state.session_start_time = session_start_time;
    state.last_update_time = session_start_time;
    state.continuation_start_time = session_start_time;

    /* Copy base filename */
    rt_strncpy(state.base_filename, base_filename, sizeof(state.base_filename) - 1);

    return nvs_state_save(&state);
}

rt_err_t nvs_state_mark_stopped(void)
{
    nvs_monitor_state_t state;
    rt_err_t result;

    /* Load current state */
    result = nvs_state_load(&state);
    if (result != RT_EOK)
    {
        /* No state file - nothing to mark */
        return RT_EOK;
    }

    /* Mark as stopped normally */
    state.running = 0;
    state.normal_exit = 1;

    return nvs_state_save(&state);
}

rt_err_t nvs_state_update(rt_uint32_t sample_count)
{
    nvs_monitor_state_t state;
    rt_err_t result;
    time_t current_time;

    /* Load current state */
    result = nvs_state_load(&state);
    if (result != RT_EOK)
    {
        LOG_W("Cannot update state - no state file");
        return -RT_ERROR;
    }

    /* Update sample count and timestamp */
    state.sample_count = sample_count;
    state.total_samples = state.sample_count;  /* For first session */
    time(&current_time);
    state.last_update_time = current_time;

    return nvs_state_save(&state);
}

rt_bool_t nvs_state_needs_recovery(void)
{
    nvs_monitor_state_t state;
    rt_err_t result;

    result = nvs_state_load(&state);
    if (result != RT_EOK)
    {
        /* No valid state - no recovery needed */
        return RT_FALSE;
    }

    /* Recovery needed if monitor was running and didn't exit normally */
    return (state.running == 1 && state.normal_exit == 0) ? RT_TRUE : RT_FALSE;
}

rt_err_t nvs_state_prepare_continuation(nvs_monitor_state_t *state)
{
    if (state == RT_NULL)
        return -RT_ERROR;

    /* Increment continuation count */
    state->continuation_count++;

    /* Reset current session counters */
    state->sample_count = 0;
    time(&state->continuation_start_time);
    state->last_update_time = state->continuation_start_time;

    /* Keep monitor running flag */
    state->running = 1;
    state->normal_exit = 0;

    /* Save updated state */
    return nvs_state_save(state);
}

void nvs_state_get_continuation_filename(const char *base_filename,
                                          rt_uint16_t continuation_count,
                                          char *output,
                                          rt_size_t output_size)
{
    if (base_filename == RT_NULL || output == RT_NULL || output_size == 0)
        return;

    if (continuation_count == 0)
    {
        /* Original filename */
        rt_strncpy(output, base_filename, output_size - 1);
        output[output_size - 1] = '\0';
    }
    else
    {
        /* Add continuation suffix: _001.csv, _002.csv, etc. */
        char *dot_pos = rt_strstr(base_filename, ".csv");
        if (dot_pos != RT_NULL)
        {
            /* Calculate position of extension */
            rt_size_t base_len = dot_pos - base_filename;

            /* Copy base part */
            if (base_len >= output_size - 8)  /* Need space for _XXX.csv */
                base_len = output_size - 9;

            rt_strncpy(output, base_filename, base_len);
            output[base_len] = '\0';

            /* Add continuation suffix */
            rt_snprintf(output + base_len, output_size - base_len,
                        "_%03d.csv", continuation_count);
        }
        else
        {
            /* No .csv extension found - just append */
            rt_snprintf(output, output_size, "%s_%03d", base_filename, continuation_count);
        }
    }
}

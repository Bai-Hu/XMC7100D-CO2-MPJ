# Power Management and Data Persistence System

**Last Updated**: 2025-11-30
**Author**: Developer
**Purpose**: Battery-powered operation with automatic power loss recovery

---

## Overview

This system enables the CO2 monitoring application to survive unexpected power losses (battery depletion, disconnection, etc.) and automatically resume monitoring when power is restored. All critical state is stored on the TF card in a non-volatile state file.

---

## Key Features

✅ **Non-Volatile State Storage** - Critical monitoring state saved to TF card
✅ **Automatic Recovery** - Detects power loss and resumes monitoring automatically
✅ **Continuation Files** - Creates numbered continuation files after each power loss
✅ **Immediate Data Sync** - Every sample is synced to TF card immediately
✅ **State Updates** - Monitoring state updated every 10 samples
✅ **Normal Exit Detection** - Distinguishes between crashes and normal stops

---

## Architecture

### Components

1. **nvs_state.c/h** - Non-volatile state storage module
2. **tf_card.c** - TF card driver with integrated state management
3. **main.c** - System initialization with power loss recovery

### Data Flow

```
Power ON
    ↓
System Init
    ↓
Check NVS State (/co2_log/.tf_monitor_state)
    ↓
    ├─→ No state found → Normal startup
    └─→ State found
         ↓
         ├─→ normal_exit=1 → Normal startup (previous session ended normally)
         └─→ normal_exit=0 → POWER LOSS DETECTED
              ↓
              Load previous state:
              - base_filename
              - interval_sec
              - sample_count
              - continuation_count
              ↓
              Generate continuation filename:
              - Original: 20251130_120000_session.csv
              - Continue1: 20251130_120000_session_001.csv
              - Continue2: 20251130_120000_session_002.csv
              ↓
              Auto-resume monitoring with continuation file
```

---

## File Naming Convention

### Original Session
```
/co2_log/20251130_143022_session.csv
```

### After First Power Loss
```
/co2_log/20251130_143022_session_001.csv
```

### After Second Power Loss
```
/co2_log/20251130_143022_session_002.csv
```

### Pattern
- **Base**: `YYYYMMDD_HHMMSS_session.csv`
- **Continuation**: `YYYYMMDD_HHMMSS_session_XXX.csv` (where XXX = 001, 002, ...)

---

## Non-Volatile State Structure

The state file is stored at: `/co2_log/.tf_monitor_state`

```c
typedef struct {
    rt_uint32_t magic;                  /* 0x544D4F4E "TMON" */
    rt_uint16_t version;                /* 0x0001 */
    rt_uint8_t  running;                /* 1=running, 0=stopped */
    rt_uint8_t  normal_exit;            /* 1=normal exit, 0=crash/power loss */

    rt_uint16_t continuation_count;     /* Number of continuations (0=original) */
    rt_uint32_t interval_sec;           /* Monitoring interval */
    rt_uint32_t sample_count;           /* Samples in current session */
    rt_uint32_t total_samples;          /* Total across all continuations */

    time_t session_start_time;          /* Original session start */
    time_t last_update_time;            /* Last state update */
    time_t continuation_start_time;     /* Current continuation start */

    char base_filename[64];             /* Base filename without path/continuation */

    rt_uint32_t crc32;                  /* Data integrity check */
} nvs_monitor_state_t;
```

---

## State Lifecycle

### 1. Starting Monitoring

**Command**: `tf_monitor start 5`

**Actions**:
1. Generate session filename: `20251130_143022_session.csv`
2. Extract base filename: `20251130_143022_session.csv`
3. Save NVS state:
   - `running = 1`
   - `normal_exit = 0` ⚠️ (Not exited yet!)
   - `continuation_count = 0`
   - `base_filename = "20251130_143022_session.csv"`
4. Open file for writing
5. Start monitoring thread

**State File Created**: `/co2_log/.tf_monitor_state`

### 2. During Monitoring

**Every Sample**:
- Write CO2 data to CSV file
- Call `fsync()` to force data to disk

**Every 10 Samples**:
- Update NVS state with current `sample_count`
- Call `fsync()` on state file

### 3. Normal Stop

**Command**: `tf_monitor stop`

**Actions**:
1. Signal thread to stop
2. Thread completes current iteration
3. **Mark NVS state**: `normal_exit = 1` ✅
4. Close file with final `fsync()`
5. Clean shutdown

**Result**: State file shows `normal_exit=1`, so no recovery needed on next boot

### 4. Power Loss (Battery Depleted)

**What Happens**:
1. System loses power mid-operation
2. Last `fsync()` ensured data is on TF card
3. NVS state file shows:
   - `running = 1`
   - `normal_exit = 0` ⚠️ (Never marked as exited!)
   - `sample_count = 450` (last updated value)

**Result**: State file indicates abnormal termination

### 5. Power Restored

**System Boot Sequence**:
1. Initialize TF card
2. Initialize NVS state module
3. Check: `nvs_state_needs_recovery()`
   - Loads state file
   - Checks: `running==1 && normal_exit==0`
   - **Returns**: `TRUE` → Recovery needed!

4. Load previous state:
   ```
   Base file: 20251130_143022_session.csv
   Interval: 5 sec
   Samples: 450
   Continuations: 0
   ```

5. Prepare continuation:
   - `continuation_count++` (now = 1)
   - Generate filename: `20251130_143022_session_001.csv`

6. Auto-resume monitoring:
   - Open continuation file
   - Start monitoring thread
   - Continue logging with same interval

**Console Output**:
```
*** POWER LOSS DETECTED ***
Previous session found:
  - Base file: 20251130_143022_session.csv
  - Interval: 5 sec
  - Samples logged: 450
  - Continuations: 0
Resuming with continuation file: 20251130_143022_session_001.csv
*** AUTO-RESUME SUCCESSFUL ***
Monitoring resumed (interval: 5 sec)
Continuation #1 started
```

---

## Usage Scenarios

### Scenario 1: Normal Operation

```bash
msh> tf_monitor start 5
# Monitoring starts...
# Data logged to: 20251130_143022_session.csv

msh> tf_monitor stop
# Normal shutdown
# State file marked: normal_exit=1

# Power cycle system
# System boots normally
# No recovery needed
```

### Scenario 2: Single Power Loss

```bash
msh> tf_monitor start 5
# Monitoring starts...
# Data logged to: 20251130_143022_session.csv
# 450 samples logged

[POWER LOSS - Battery depleted]

# Power restored
# System boots
*** POWER LOSS DETECTED ***
# Auto-resume
# Data continues in: 20251130_143022_session_001.csv
```

### Scenario 3: Multiple Power Losses

```bash
msh> tf_monitor start 5
# Original: 20251130_143022_session.csv

[POWER LOSS #1]
# Auto-resume
# Continue: 20251130_143022_session_001.csv

[POWER LOSS #2]
# Auto-resume
# Continue: 20251130_143022_session_002.csv

[POWER LOSS #3]
# Auto-resume
# Continue: 20251130_143022_session_003.csv
```

**Result**: You have 4 files tracking the complete session history

---

## Data Integrity Guarantees

### What is GUARANTEED after power loss:

✅ **All synced data is preserved**
- Every sample is `fsync()`'d immediately
- Last synced data is on TF card

✅ **State file integrity**
- CRC32 checksum validates state data
- Corrupted state = ignored, manual recovery

✅ **File system consistency**
- FatFS handles power loss gracefully
- Files are in consistent state

### What is NOT guaranteed:

❌ **Data in write buffer** (between `write()` and `fsync()`)
- Negligible risk since we `fsync()` every sample

❌ **Partial line writes** (extremely rare)
- Last line in CSV may be incomplete
- Easy to identify and discard

---

## API Reference

### NVS State Functions

```c
/* Initialize NVS state storage */
rt_err_t nvs_state_init(void);

/* Save state to NVS */
rt_err_t nvs_state_save(const nvs_monitor_state_t *state);

/* Load state from NVS */
rt_err_t nvs_state_load(nvs_monitor_state_t *state);

/* Mark monitoring started (running=1, normal_exit=0) */
rt_err_t nvs_state_mark_started(const char *base_filename,
                                 rt_uint32_t interval_sec,
                                 time_t session_start_time);

/* Mark monitoring stopped normally (normal_exit=1) */
rt_err_t nvs_state_mark_stopped(void);

/* Update sample count during monitoring */
rt_err_t nvs_state_update(rt_uint32_t sample_count);

/* Check if power loss recovery is needed */
rt_bool_t nvs_state_needs_recovery(void);

/* Prepare continuation state after power loss */
rt_err_t nvs_state_prepare_continuation(nvs_monitor_state_t *state);

/* Generate continuation filename */
void nvs_state_get_continuation_filename(const char *base_filename,
                                          rt_uint16_t continuation_count,
                                          char *output,
                                          rt_size_t output_size);
```

---

## Troubleshooting

### Issue: Auto-resume not working

**Check**:
1. Is TF card mounted? (`tf_info`)
2. Does state file exist? (`ls /co2_log/.tf_monitor_state`)
3. Is S8 sensor responsive? (`s8_read`)

**Solution**:
```bash
# Manual recovery
msh> tf_list  # Check existing files
msh> tf_monitor start 5  # Start manually
```

### Issue: State file corrupted

**Symptoms**: CRC32 validation fails

**Solution**:
```bash
# Delete corrupted state
msh> rm /co2_log/.tf_monitor_state

# Start fresh
msh> tf_monitor start 5
```

### Issue: Too many continuation files

**Cleanup**:
```bash
# Stop monitoring
msh> tf_monitor stop

# Export data
msh> tf_export 20251130_143022_session.csv uart4
msh> tf_export 20251130_143022_session_001.csv uart4
# ... etc

# Delete old files
msh> rm /co2_log/20251130_143022_session*.csv

# Start fresh session
msh> tf_monitor start 5
```

---

## Technical Details

### Write Synchronization

**Current Strategy**: Immediate `fsync()` every sample

```c
write(fd, data, size);
fsync(fd);  // Force to disk immediately
```

**Why**: Battery operation requires maximum data safety

**Trade-off**: Slower writes, but data is always safe

### State Update Frequency

**Every 10 samples** (50 seconds at 5-second interval)

**Why**: Balance between:
- State accuracy (how much data loss on crash)
- Write wear on TF card
- Performance overhead

### CRC32 Validation

**Purpose**: Detect state file corruption

**Algorithm**: Standard CRC32 with lookup table

**Coverage**: All fields except `crc32` itself

---

## Known Limitations

1. **RTC Reset**: If on-chip RTC resets, timestamps may be inaccurate
   - **Mitigation**: Relative timestamps (`elapsed_seconds`) still valid

2. **TF Card Removal**: If TF card is removed while monitoring
   - **Result**: Monitoring crashes, no state saved
   - **Recovery**: Insert card and start manually

3. **File System Full**: If TF card is full
   - **Result**: Writes fail, monitoring continues but data lost
   - **Prevention**: Monitor free space with `tf_info`

4. **Maximum Continuations**: Filename supports up to 999 continuations
   - **Workaround**: Stop monitoring, export data, start new session

---

## Future Enhancements

Possible improvements (not currently implemented):

- [ ] External RTC with battery backup for accurate timestamps
- [ ] Voltage monitoring for early power loss detection
- [ ] Buffered writes with smart `fsync()` (e.g., every N seconds)
- [ ] Automatic file merging of continuation files
- [ ] State file backup/redundancy
- [ ] Wear leveling for state file location

---

## Testing Checklist

- [ ] Start monitoring, stop normally → No recovery on reboot
- [ ] Start monitoring, pull power → Auto-resume on reboot
- [ ] Multiple power cycles → Multiple continuation files
- [ ] Verify data integrity in all files
- [ ] Check state file CRC32 validation
- [ ] Test with corrupted state file
- [ ] Test with missing state file
- [ ] Test with TF card full

---

## Document End

**Summary**: This power management system provides robust data persistence for battery-powered CO2 monitoring with automatic recovery after unexpected power losses. All critical state is stored on the TF card, and monitoring resumes automatically with numbered continuation files.

For questions or issues, check the troubleshooting section or examine the state file manually.

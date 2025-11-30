# Power Management Quick Start Guide

## What This Does

Your CO2 monitoring system can now **automatically recover from power losses**. When you unplug the battery or it runs out, the system will:

1. ✅ Save all recorded data safely to the TF card
2. ✅ Remember it was running
3. ✅ Automatically resume monitoring when power returns
4. ✅ Continue logging to a new continuation file

---

## Quick Example

### Before Power Loss
```bash
msh> tf_monitor start 5
TF monitor started successfully
Session file: /co2_log/20251130_143022_session.csv
Logged 50 samples (state saved)
Logged 100 samples (state saved)
...
```

### [Battery Dies or Unplugged]

### After Power Returns
```bash
System boots...
*** POWER LOSS DETECTED ***
Previous session found:
  - Base file: 20251130_143022_session.csv
  - Interval: 5 sec
  - Samples logged: 450
Resuming with continuation file: 20251130_143022_session_001.csv
*** AUTO-RESUME SUCCESSFUL ***
Monitoring resumed (interval: 5 sec)

Logged 5 samples (immediate sync)
Logged 10 samples (state saved)
...
```

**Result**: You now have 2 files:
- `20251130_143022_session.csv` (original, 450 samples)
- `20251130_143022_session_001.csv` (continuation after power loss)

---

## Basic Commands

### Start Monitoring
```bash
msh> tf_monitor start 5
# Logs CO2 every 5 seconds
```

### Stop Monitoring (Normal)
```bash
msh> tf_monitor stop
# Stops normally, no recovery on next boot
```

### Check Status
```bash
msh> tf_monitor
Status: Running
Samples logged: 245
Interval: 5 seconds
Session file: /co2_log/20251130_143022_session.csv
```

### List Data Files
```bash
msh> tf_list
=== Data Files ===
  20251130_143022_session.csv
  20251130_143022_session_001.csv
  20251130_143022_session_002.csv
```

### Export Data to PC
```bash
msh> tf_export 20251130_143022_session.csv uart4
# Sends CSV data via serial
```

---

## What Happens During Power Loss

### Data Safety
- **Every sample is saved immediately** to the TF card
- **No data is lost** (except maybe the very last sample being written)
- **State file tracks progress** so system knows where it left off

### Auto-Resume
1. System detects abnormal shutdown (power loss)
2. Loads previous monitoring state from TF card
3. Creates new continuation file (`_001.csv`, `_002.csv`, etc.)
4. Resumes monitoring automatically with same interval

### Multiple Power Losses
Each power loss creates a new numbered file:
```
20251130_143022_session.csv       (original)
20251130_143022_session_001.csv   (after 1st power loss)
20251130_143022_session_002.csv   (after 2nd power loss)
20251130_143022_session_003.csv   (after 3rd power loss)
...
```

---

## Important Notes

### ✅ Safe Operations
- Unplugging battery during monitoring → **AUTO-RECOVERS**
- Battery running out → **AUTO-RECOVERS**
- System crash/hang → **AUTO-RECOVERS**
- Using `tf_monitor stop` → **NORMAL EXIT** (no recovery needed)

### ⚠️ Manual Recovery Needed
- Removing TF card during monitoring → **Restart manually**
- System boot with S8 sensor disconnected → **Restart manually**

### 📝 Data Management
- Export files regularly to PC
- Delete old files after exporting
- Monitor TF card free space (`tf_info`)

---

## Troubleshooting

### Problem: Auto-resume didn't work

**Check**:
```bash
msh> tf_info
# Is TF card mounted?

msh> ls /co2_log/.tf_monitor_state
# Does state file exist?

msh> s8_read
# Is S8 sensor working?
```

**Solution**: Start manually
```bash
msh> tf_monitor start 5
```

### Problem: Too many continuation files

**Clean up**:
```bash
# Stop monitoring
msh> tf_monitor stop

# Export all files to PC
msh> tf_export 20251130_143022_session.csv uart4
msh> tf_export 20251130_143022_session_001.csv uart4
# etc...

# Delete old files
msh> rm /co2_log/20251130_143022_session*.csv

# Start fresh
msh> tf_monitor start 5
```

---

## CSV File Format

### Session File
```csv
rtc_timestamp,elapsed_seconds,co2_ppm
1701100800,0,450
1701100805,5,452
1701100810,10,455
...
```

### Continuation File (after power loss)
```csv
rtc_timestamp,elapsed_seconds,co2_ppm
1701103200,0,448
1701103205,5,451
1701103210,10,454
...
```

**Note**: `elapsed_seconds` resets to 0 in each continuation file

---

## Testing Power Loss Recovery

### Safe Test Procedure

1. **Start monitoring**:
   ```bash
   msh> tf_monitor start 5
   ```

2. **Wait for some samples**:
   ```
   Logged 50 samples (state saved)
   ```

3. **Simulate power loss**:
   - Simply unplug the USB/battery
   - Don't use `tf_monitor stop` (that's normal exit!)

4. **Restore power**:
   - Plug back in
   - Watch console for recovery message

5. **Verify**:
   ```bash
   msh> tf_list
   # Should see original + continuation file
   ```

---

## Best Practices

### For Long-Term Monitoring

1. **Start monitoring and let it run**:
   ```bash
   msh> tf_monitor start 5
   ```

2. **Don't worry about power losses** - system handles them automatically

3. **Periodically check TF card space**:
   ```bash
   msh> tf_info
   ```

4. **Export data regularly** (e.g., weekly):
   ```bash
   msh> tf_list  # See all files
   msh> tf_export FILENAME.csv uart4
   ```

5. **Clean up after exporting**:
   ```bash
   msh> rm /co2_log/OLD_FILES*.csv
   ```

### For Battery Operation

- Monitor will run until battery is completely dead
- All data up to last sample will be saved
- System will auto-resume when you replace/recharge battery
- **No manual intervention needed!**

---

## System Behavior Summary

| Scenario | What Happens | User Action Needed |
|----------|--------------|-------------------|
| Normal stop (`tf_monitor stop`) | Clean shutdown, state cleared | None - normal restart next time |
| Power loss during monitoring | Data saved, state preserved | **None** - auto-resumes on boot |
| Battery depleted | Same as power loss | **None** - auto-resumes when recharged |
| TF card removed | Monitoring crashes | Restart manually after inserting card |
| S8 sensor error during recovery | Auto-resume skipped | Check sensor, start manually |

---

## Need More Details?

See **POWER_MANAGEMENT_README.md** for:
- Complete architecture documentation
- Detailed API reference
- Advanced troubleshooting
- Technical implementation details

---

**Summary**: Your system is now **power-loss resistant**. Start monitoring and don't worry about battery life - the system will handle power losses gracefully and resume automatically.

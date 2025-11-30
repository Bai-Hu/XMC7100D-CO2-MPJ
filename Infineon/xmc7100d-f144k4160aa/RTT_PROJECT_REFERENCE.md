# RT-Thread Project Reference Document

**Project**: Senseair S8 CO2 Sensor Integration on XMC7100D-F144K4160AA
**RT-Thread Version**: 5.2.1
**Last Updated**: 2025-11-29

---

## 1. Directory Structure Overview

### 1.1 RT-Thread Root Directory
```
RTT_ROOT: C:\Users\32272\Documents\bechlor\code\RTT\rt-thread-master1\rt-thread-master
├── bsp/                    # Board Support Packages (100+ vendors)
├── components/             # Reusable software components
│   ├── dfs/               # Device File System (FatFS, DevFS)
│   ├── drivers/           # Device driver framework
│   ├── finsh/             # MSH Shell component
│   ├── libc/              # C library support
│   └── net/               # Network stack (LwIP, AT)
├── documentation/          # Project documentation
├── examples/               # Sample code
├── include/               # RT-Thread kernel headers
│   ├── rtthread.h         # Main API header
│   ├── rtdef.h            # Core type definitions
│   └── rtdevice.h         # Device framework
├── libcpu/                # CPU architecture-specific code
│   └── arm/               # ARM Cortex-M implementations
│       └── cortex-m7/     # Cortex-M7 specific code
├── src/                   # Core kernel source
│   ├── clock.c            # System clock management
│   ├── ipc.c              # Inter-Process Communication
│   ├── scheduler_up.c     # Single-processor scheduler
│   ├── thread.c           # Thread management
│   └── timer.c            # Software timers
└── tools/                 # Build tools and scripts
```

### 1.2 Project Root Directory
```
PROJECT_ROOT: C:\Users\32272\Documents\bechlor\code\RTT\rt-thread-master1\rt-thread-master\bsp\Infineon\xmc7100d-f144k4160aa
├── applications/          # ★ User application code (EDITABLE)
│   ├── main.c            # Application entry point
│   ├── s8_sensor.c/h     # S8 CO2 sensor driver
│   ├── s8_msh.c          # MSH shell commands for S8
│   ├── s8_self_test.c    # Sensor self-test module
│   ├── s8_debug.c        # Debug utilities
│   ├── modbus_rtu.c/h    # Modbus RTU protocol
│   ├── co2_monitor.c/h   # CO2 monitoring application
│   ├── tf_card.c/h       # TF card driver (SPI SD card)
│   ├── tf_msh.c          # MSH shell commands for TF card
│   ├── rtc_msh.c/h       # RTC MSH shell commands
│   ├── RTC_README.md     # RTC module documentation
│   ├── arduino_main.cpp  # Arduino compatibility layer
│   ├── arduino_pinout/   # Arduino pin mapping
│   │   ├── pins_arduino.c/h  # Pin definitions
│   │   ├── README.md     # Arduino pinout documentation
│   │   └── SConscript    # Arduino build script
│   └── SConscript        # Application build script
├── board/                 # Board initialization (LIMITED EDIT)
│   ├── board.c           # Hardware initialization
│   ├── board.h           # Board definitions
│   ├── Kconfig           # Hardware configuration menu
│   ├── linker_scripts/   # Memory layout (DO NOT MODIFY)
│   │   ├── link.ld       # GCC linker script
│   │   └── link.sct      # ARM linker script
│   └── ports/            # Peripheral ports (EDITABLE)
│       ├── drv_filesystem.c  # FAT file system mount
│       ├── drv_rw007.c       # RW007 WiFi driver
│       ├── spi_flash_init.c  # SPI SD card initialization
│       └── SConscript
├── libs/                  # Vendor SDK (DO NOT MODIFY)
│   └── TARGET_APP_KIT_XMC71_EVK_LITE_V2/
│       ├── COMPONENT_CM7/ # Cortex-M7 core
│       └── config/       # Device configuration
├── packages/              # Downloaded RT-Thread packages
├── doc/                   # Project documentation
│   ├── PSP107.pdf        # S8 Residential Product Specification
│   ├── TDE2067.pdf       # S8 Modbus Protocol Specification
│   ├── 芯启主板说明书.docx  # Board manual (Chinese)
│   └── 芯启主板原理图.pdf   # Board schematic
├── rtconfig.h            # Generated configuration header
├── rtconfig.py           # Toolchain configuration
├── Kconfig               # BSP Kconfig entry
├── SConstruct            # Main SCons build file
└── SConscript            # BSP build script
```

---

## 2. File Modification Rules

### 2.1 EDITABLE Directories (Project Scope)

| Directory | Description | Constraints |
|-----------|-------------|-------------|
| `applications/` | User application code | Max 400 lines/file |
| `board/ports/` | Peripheral port adapters | Follow existing patterns |
| `board/Kconfig` | Add hardware options | Use existing syntax |

### 2.2 READ-ONLY Directories

| Directory | Reason |
|-----------|--------|
| `libs/` | Vendor SDK - changes will break compatibility |
| `board/linker_scripts/` | Memory layout - critical for boot |
| `packages/` | Package manager controlled |
| All RTT root directories | Architecture protection |

### 2.3 Architecture Protection Rules

**NEVER modify files in RTT_ROOT:**
- `src/*` - Kernel source
- `include/*` - Kernel headers
- `libcpu/*` - CPU-specific code
- `components/*` - Shared components
- `bsp/Infineon/libraries/*` - Shared HAL drivers

**Shared HAL Drivers Location:**
```
bsp/Infineon/libraries/HAL_Drivers/
├── drv_uart.c/h      # UART driver (shared)
├── drv_gpio.c/h      # GPIO driver (shared)
├── drv_spi.c/h       # SPI driver (shared)
├── drv_i2c.c         # I2C driver (shared)
├── drv_common.c/h    # Common utilities
└── config/
    └── xmc7100/      # XMC7100-specific config
```

---

## 3. Hardware Configuration

### 3.1 MCU Specifications

| Item | Value |
|------|-------|
| MCU | XMC7100D-F144K4160AA |
| Core | ARM Cortex-M7 + Cortex-M0+ |
| Flash | 4160KB |
| RAM | 1024KB |
| Clock | Up to 350MHz |
| Package | 144-pin LQFP |

### 3.2 S8 Sensor Pin Connections

| S8 Pin | MCU Pin | Board Label | Function |
|--------|---------|-------------|----------|
| G+ | 5V | 5V | Power supply (+) |
| G0 | GND | GND | Power supply (-) |
| UART_RXD | P19_1 | TX | MCU TX → Sensor RX |
| UART_TXD | P19_0 | RX | Sensor TX → MCU RX |
| UART_R/T | P20_1 | IO5 | Direction control |
| Alarm_OC | P19_3 | IO2 | Alarm output (open collector) |
| bCAL_in | P20_2 | IO6 | Background calibration input |

### 3.3 UART Configuration

| Parameter | Value |
|-----------|-------|
| UART Instance | UART2 |
| Baud Rate | 9600 bps |
| Data Bits | 8 |
| Parity | None |
| Stop Bits | 1 (RX), 2 (TX) |
| Flow Control | None |

---

## 4. S8 Sensor Specifications

### 4.1 General Specifications

| Item | Specification |
|------|---------------|
| Target Gas | Carbon Dioxide (CO₂) |
| Measurement Range | 400-2000 ppm (standard), up to 10000 ppm (extended) |
| Accuracy | ±70 ppm ±3% of reading |
| Response Time | 2 minutes (90%) |
| Measurement Interval | 2 seconds |
| Operating Temperature | 0-50°C |
| Operating Humidity | 0-85% RH (non-condensed) |
| Power Supply | 4.5-5.25V |
| Power Consumption | 30mA average, 300mA peak |
| Communication | Modbus RTU over UART |

### 4.2 Modbus Protocol Summary

**Communication Parameters:**
- Slave Address: 0xFE (any sensor) or 1-247 (individual)
- Function Codes: 0x03 (Read Holding), 0x04 (Read Input), 0x06 (Write Single)
- Response Timeout: 180ms max

**Input Registers (Function 0x04, Read-Only):**

| Register | Address | Description |
|----------|---------|-------------|
| IR1 | 0x0000 | MeterStatus (error flags) |
| IR2 | 0x0001 | AlarmStatus |
| IR3 | 0x0002 | OutputStatus |
| IR4 | 0x0003 | **CO₂ Concentration (ppm)** |
| IR22 | 0x0015 | PWM Output |
| IR26 | 0x0019 | Sensor Type ID High |
| IR27 | 0x001A | Sensor Type ID Low |
| IR29 | 0x001C | FW Version (Main.Sub) |
| IR30 | 0x001D | Sensor ID High |
| IR31 | 0x001E | Sensor ID Low |

**Holding Registers (Function 0x03/0x06, Read/Write):**

| Register | Address | Description |
|----------|---------|-------------|
| HR1 | 0x0000 | Acknowledgement Register |
| HR2 | 0x0001 | Special Command Register |
| HR32 | 0x001F | ABC Period (hours) |

**MeterStatus Error Bits (IR1):**

| Bit | Error Code | Description |
|-----|------------|-------------|
| DI1 | 1 | Fatal error |
| DI2 | 2 | Offset regulation error |
| DI3 | 4 | Algorithm error |
| DI4 | 8 | Output error |
| DI5 | 16 | Self-diagnostic error |
| DI6 | 32 | Out of range error |
| DI7 | 64 | Memory error |

### 4.3 Calibration Commands

**Background Calibration (400 ppm):**
```
Write HR2 (0x0001): 0x7C06
Wait: 2+ seconds
Read HR1 (0x0000): Check bit 5 (CI6) = 1
```

**Zero Calibration (0 ppm):**
```
Write HR2 (0x0001): 0x7C07
Wait: 2+ seconds
Read HR1 (0x0000): Check bit 6 (CI7) = 1
```

**Disable ABC:**
```
Write HR32 (0x001F): 0x0000
```

**Enable ABC (7.5 days = 180 hours):**
```
Write HR32 (0x001F): 0x00B4
```

---

## 5. RT-Thread Configuration

### 5.1 Enabled Features (rtconfig.h)

```c
/* Kernel Configuration */
#define RT_NAME_MAX 15
#define RT_TICK_PER_SECOND 1000
#define RT_THREAD_PRIORITY_MAX 32
#define RT_MAIN_THREAD_STACK_SIZE 2048

/* IPC */
#define RT_USING_SEMAPHORE
#define RT_USING_MUTEX
#define RT_USING_EVENT
#define RT_USING_MAILBOX
#define RT_USING_MESSAGEQUEUE

/* Device Drivers */
#define RT_USING_SERIAL
#define RT_USING_PIN
#define RT_USING_SPI
#define RT_USING_I2C

/* Console & Shell */
#define RT_USING_CONSOLE
#define RT_CONSOLE_DEVICE_NAME "uart4"
#define RT_USING_MSH
#define RT_USING_FINSH
#define FINSH_THREAD_STACK_SIZE 4096

/* File System */
#define RT_USING_DFS
#define RT_USING_DFS_ELMFAT
#define RT_USING_DFS_DEVFS

/* Hardware Config */
#define BSP_USING_UART2
#define BSP_USING_UART4
#define BSP_USING_GPIO
#define BSP_USING_SPI
#define BSP_USING_TF_CARD
```

### 5.2 UART Instances

| UART | Function | Pins |
|------|----------|------|
| UART2 | S8 Sensor Communication | P19_0 (RX), P19_1 (TX) |
| UART4 | Console/Debug (USB) | USB-UART Bridge |

---

## 6. Build System

### 6.1 Build Commands (ENV Tool)

```bash
# Configure project
menuconfig

# Build project
scons

# Clean build
scons -c

# Rebuild all
scons -c && scons
```

### 6.2 Toolchain Configuration (rtconfig.py)

```python
ARCH = 'arm'
CPU = 'cortex-m7'
CROSS_TOOL = 'keil'  # or 'gcc'
```

### 6.3 Build Outputs

| File | Description |
|------|-------------|
| `rtthread.bin` | Binary image for flashing |
| `rt-thread.elf` | ELF with debug symbols |
| `rt-thread.hex` | Intel HEX format |
| `rtthread.map` | Memory map |

---

## 7. Application Source Files

### 7.1 File Purposes

| File | Lines | Purpose |
|------|-------|---------|
| `main.c` | ~80 | Application entry, system init |
| `s8_sensor.c` | ~400 | S8 sensor driver core |
| `s8_sensor.h` | ~140 | S8 sensor API definitions |
| `s8_msh.c` | ~300 | MSH commands for S8 |
| `s8_self_test.c` | ~200 | Sensor self-test routines |
| `s8_debug.c` | ~200 | Debug/diagnostic utilities |
| `modbus_rtu.c` | ~400 | Modbus RTU protocol |
| `modbus_rtu.h` | ~100 | Modbus API definitions |
| `co2_monitor.c` | ~150 | Continuous monitoring |
| `co2_monitor.h` | ~50 | Monitor API |
| `tf_card.c` | ~850 | TF card driver (all 4 stages) |
| `tf_card.h` | ~200 | TF card API definitions |
| `tf_msh.c` | ~300 | MSH commands for TF card |
| `rtc_msh.c` | ~150 | RTC MSH shell commands |
| `rtc_msh.h` | ~30 | RTC MSH API definitions |
| `RTC_README.md` | - | RTC module documentation |

### 7.2 Key Data Structures

```c
/* S8 Sensor Data */
typedef struct {
    rt_uint16_t co2_ppm;
    rt_uint8_t  alarm_state;
    rt_uint32_t timestamp;
    rt_bool_t   data_valid;
} s8_sensor_data_t;

/* S8 Sensor Device */
typedef struct {
    modbus_rtu_device_t *modbus;
    s8_sensor_data_t data;
    rt_thread_t monitor_thread;
    rt_uint32_t read_interval_ms;
    rt_bool_t running;
} s8_sensor_device_t;

/* Modbus RTU Device */
typedef struct {
    rt_device_t serial;
    rt_uint8_t slave_addr;
    rt_mutex_t lock;
    rt_uint32_t timeout_ms;
} modbus_rtu_device_t;

/* TF Card CO2 Record (for storage) */
typedef struct {
    rt_uint32_t rtc_timestamp;      /* RTC Unix timestamp */
    rt_uint32_t elapsed_seconds;    /* Elapsed time from session start */
    rt_uint16_t co2_ppm;            /* CO2 concentration in ppm */
} tf_co2_record_t;

/* TF Card Monitor State (for persistence across power cycles) */
typedef struct {
    rt_bool_t running;                    /* Monitor running state */
    rt_uint32_t interval_sec;             /* Monitoring interval in seconds */
    char session_file[64];                /* Current session filename */
    rt_thread_t monitor_thread;           /* Monitor thread handle */
    rt_uint32_t sample_count;             /* Total samples logged */
    time_t session_start_time;            /* Session start timestamp */
    int session_file_fd;                  /* Open file descriptor */
} tf_monitor_state_t;

/* TF Card Info */
typedef struct {
    rt_uint32_t total_size_mb;
    rt_uint32_t free_size_mb;
    rt_uint32_t sector_size;
    rt_bool_t   mounted;
} tf_card_info_t;
```

---

## 8. MSH Commands Reference

### 8.1 S8 Sensor Commands

| Command | Description | Example |
|---------|-------------|---------|
| `s8_read` | Read CO₂ concentration | `s8_read` |
| `s8_info` | Read sensor information | `s8_info` |
| `s8_status` | Read sensor status | `s8_status` |
| `s8_cal_bg` | Background calibration | `s8_cal_bg` |
| `s8_cal_zero` | Zero calibration | `s8_cal_zero` |
| `s8_abc` | Enable/disable ABC | `s8_abc 1` or `s8_abc 0` |
| `s8_alarm` | Set alarm threshold | `s8_alarm 1000` |
| `s8_monitor` | Start/stop monitoring | `s8_monitor start 5` |

### 8.2 TF Card Commands

| Command | Description | Example |
|---------|-------------|---------|
| `tf_init` | Initialize TF card driver | `tf_init` |
| `tf_info` | Display TF card information | `tf_info` |
| `tf_test` | Test TF card read/write | `tf_test` |
| `tf_log` | Log current CO2 reading | `tf_log` |
| `tf_list` | List data files | `tf_list` |
| `tf_send` | Send file to PC via serial | `tf_send 20251127.csv uart4` |
| `tf_export` | Export data as CSV | `tf_export datafile.bin` |
| `tf_monitor` | Start/stop manual logging with power outage detection | `tf_monitor start 5` |
| `tf_realtime` | Stream real-time data | `tf_realtime start uart4` |

### 8.3 System Commands

| Command | Description |
|---------|-------------|
| `help` | List all commands |
| `list_thread` | Show all threads |
| `list_device` | Show all devices |
| `free` | Show memory usage |
| `ps` | Process status |
| `ls` | List files |
| `cat` | Display file content |

---

## 9. TF Card Storage

### 9.1 Hardware Configuration

| Item | Value | MCU Pin |
|------|-------|---------|
| Interface | SPI5 | - |
| SCK (Clock) | SPI_SCK1 | P7_2 (Pin 42) |
| MISO (Data In) | SPI_MISO1 | P7_0 (Pin 40) |
| MOSI (Data Out) | SPI_MOSI1 | P7_1 (Pin 41) |
| CS (Chip Select) | SPI_CS1 | P7_3 (Pin 43) |
| File System | FAT32/FAT16 | - |
| Mount Point | / | - |

**TF Card Slot Pin Mapping (TF-CARD H1.8):**

| TF Slot Pin | Function | Signal |
|-------------|----------|--------|
| 2 | CD/DAT3 | SPI_CS1 |
| 3 | CMD | SPI_MOSI1 |
| 4 | VDD | 3V3 |
| 5 | CLK | SPI_SCK1 |
| 6 | VSS | GND |
| 7 | DAT0 | SPI_MISO1 |
| 9 | CD | Card Detect |
| 10-13 | GND | Ground |

### 9.2 TF Card Driver Stages

**Stage 1: Communication**
- Initialize SPI and SD card
- Mount FAT file system
- Basic read/write operations
- Card info and test functions

**Stage 2: Data Storage**
- Create log directory `/co2_log/`
- Write CO2 records to daily CSV files
- Append mode for continuous logging

**Stage 3: Structured Data**
- Binary file format with header
- Record indexing for fast access
- File management (create, open, read)

**Stage 4: Serial Communication**
- Send files to PC via UART
- Export data as CSV format
- Real-time data streaming (JSON)

### 9.3 Data File Formats

**Daily CSV Log (YYYYMMDD.csv):**
```csv
timestamp,co2_ppm,alarm_state,status
1701100800,450,0,0
1701100802,452,0,0
```

**Binary Data File Header:**
```c
typedef struct {
    rt_uint32_t magic;          /* 0x43433032 "CC02" */
    rt_uint16_t version;        /* File format version */
    rt_uint16_t record_size;    /* Size of each record */
    rt_uint32_t record_count;   /* Total records */
    rt_uint32_t start_timestamp;
    rt_uint32_t end_timestamp;
    rt_uint16_t interval_sec;
} tf_file_header_t;
```

**Real-time JSON Format (Serial):**
```json
{"ts":1701100800,"co2":450,"alarm":0,"status":0}
```

### 9.4 Robust Data Persistence & Power Outage Recovery

**Key Features:**
- **Persistent Monitor State**: Global `tf_monitor_state_t` structure survives power cycles
- **Robust File Handling**: Session files kept open during monitoring with `fsync()` every 10 samples
- **Manual Control**: TF monitoring only starts via MSH command (`tf_monitor start`)
- **Power Outage Detection**: Automatically detects power loss and creates new session files
- **Data Integrity**: Critical data is forced to physical storage regularly

**Power Outage Behavior:**
```
Before Power Outage:
├── Monitor running: YES (started via MSH command)
├── Session file: /co2_log/20251129_143022_session.csv
├── Sample count: 1,250
└── File state: Open with recent data sync'd

Power Outage Occurs:
├── System loses power
├── On-chip RTC resets to default time
├── Session file safely closed by OS/RTT
└── Last sync'd data preserved on TF card

After Power Restoration:
├── System boots → main() runs
├── TF card initialized
├── Monitor state restored
├── Power outage detected (RTC time discrepancy)
├── Monitor remains STOPPED (manual control)
├── Previous session file: Preserved with all data intact
└── New session created when manually started: /co2_log/20250101_120000_OUTAGE_session.csv
```

**Data Loss Mitigation:**
1. **File kept open** during monitoring (reduces open/close overhead)
2. **fsync() every 10 samples** forces data to physical media
3. **Final fsync() on shutdown** ensures all data written
4. **Session files never overwritten** - new sessions created after restart

**Power Outage Detection:**
```c
// RTC time-based power outage detection
static rt_bool_t tf_detect_power_outage(time_t current_time)
{
    static time_t last_known_time = 0;

    if (current_time < last_known_time) {
        // Time went backwards = power outage
        return RT_TRUE;
    }

    if ((current_time - last_known_time) > 24*60*60) {
        // Time jump > 24 hours = likely reset
        return RT_TRUE;
    }

    last_known_time = current_time;
    return RT_FALSE;
}
```

**Manual Control Required:**
```c
// TF monitoring only starts via MSH command
// No auto-resume after power outage
tf_monitor start 5  // User must manually restart

// Power outage detection informs user
if (monitor_state->power_outage_detected) {
    // Create session file with "_OUTAGE" marker
    // User sees: "Power outage detected - new session file created"
}
```

---

## 10. Development Guidelines

### 10.1 Code Style

- Follow RT-Thread coding style
- Use `rt_` prefix for RT-Thread API calls
- Use `s8_` prefix for S8 sensor functions
- Use `modbus_` prefix for Modbus functions
- Use `tf_` prefix for TF card functions
- Maximum 400 lines per source file
- Include copyright header in all files

### 10.2 Error Handling

```c
/* Use RT-Thread error codes */
#define RT_EOK      0    /* Success */
#define RT_ERROR   -1    /* General error */
#define RT_ETIMEOUT -2   /* Timeout */
#define RT_ENOMEM  -3    /* No memory */
#define RT_EBUSY   -4    /* Busy */
```

### 10.3 Thread Safety

- Use `rt_mutex_take()/rt_mutex_release()` for shared resources
- Use `rt_enter_critical()/rt_exit_critical()` sparingly
- Modbus communication is mutex-protected

---

## 11. Compile/Runtime Dependencies

### 11.1 Compile Dependencies

| Component | Required For |
|-----------|--------------|
| RT_USING_SERIAL | UART communication |
| RT_USING_PIN | GPIO control |
| RT_USING_MUTEX | Thread safety |
| RT_USING_MSH | Shell commands |
| RT_USING_DFS | File system |
| RT_USING_DFS_ELMFAT | FAT file system |

### 11.2 Runtime Dependencies

| Resource | Purpose |
|----------|---------|
| UART2 | S8 sensor communication |
| UART4 | Console output |
| GPIO P19_3 | Alarm input |
| GPIO P20_1 | UART R/T control |
| GPIO P20_2 | Calibration control |
| SPI5 | TF card interface |

---

## 12. Quick Reference

### 12.1 Key Paths

```
RTT_ROOT    = C:\Users\32272\Documents\bechlor\code\RTT\rt-thread-master1\rt-thread-master
PROJECT     = ${RTT_ROOT}\bsp\Infineon\xmc7100d-f144k4160aa
APPS        = ${PROJECT}\applications
DOCS        = ${PROJECT}\doc
HAL_DRIVERS = ${RTT_ROOT}\bsp\Infineon\libraries\HAL_Drivers
```

### 12.2 Important Files

| File | Purpose |
|------|---------|
| `rtconfig.h` | All RT-Thread configuration macros |
| `board/Kconfig` | Hardware menu configuration |
| `applications/SConscript` | Application build rules |
| `doc/TDE2067.pdf` | S8 Modbus protocol reference |

### 12.3 GPIO Pin Mapping

```c
/* Pin calculation: port * 8 + pin */
#define GET_PIN(port, pin)  ((port) * 8 + (pin))

/* S8 Sensor Pins */
#define S8_ALARM_PIN     GET_PIN(19, 3)  /* = 155 */
#define S8_UART_RXT_PIN  GET_PIN(20, 1)  /* = 161 */
#define S8_BCAL_PIN      GET_PIN(20, 2)  /* = 162 */
```

---

## 13. Troubleshooting

### 13.1 Common Issues

| Issue | Cause | Solution |
|-------|-------|----------|
| No sensor response | Wiring error | Check TX/RX crossover |
| CRC error | Noise/timing | Check baud rate, wiring |
| MSH not working | Shell disabled | Enable RT_USING_MSH |
| TF card not found | SPI config | Check SPI5 in menuconfig |

### 13.2 Debug Commands

```bash
# Check UART device
list_device

# Check thread status
list_thread

# Test Modbus manually
s8_status

# Verbose debug output
s8_debug 1
```

---

## Document End

**Reload Instruction**: If context compression occurs, reload this document from:
`${PROJECT}\RTT_PROJECT_REFERENCE.md`

All modifications to project files must comply with the rules defined in Section 2.

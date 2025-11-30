# RTC驱动集成说明

## 概述
本项目已成功集成了XMC7100的RTC驱动，提供了完整的MSH命令接口用于时间管理。

## 文件结构
```
applications/
├── rtc_msh.h          # RTC MSH命令头文件
├── rtc_msh.c          # RTC MSH命令实现
└── SConscript          # 构建配置（已更新）

board/
└── Kconfig            # RTC配置选项（已添加）
```

## 配置步骤

### 1. 启用RTC配置
通过menuconfig启用RTC：
```bash
menuconfig
```
导航到：
```
Hardware Drivers Config → On-chip Peripheral Drivers → [*] Enable RTC
```
选择时钟源：
- WCO (外部32.768kHz晶振) - 高精度，低功耗
- ILO (内部振荡器) - 无需外部元件

### 2. 手动配置（如需要）
如果无法使用menuconfig，可手动在rtconfig.h中添加：
```c
#define BSP_USING_RTC
#define RT_USING_RTC
#define RT_USING_ALARM
#define BSP_RTC_USING_ILO  // 或 BSP_RTC_USING_WCO
```

## MSH命令

### 基本命令
- `rtc_read` - 读取当前RTC时间
- `rtc_help` - 显示帮助信息

### 时间设置命令
- `rtc_set YYYY-MM-DD HH:MM:SS` - 设置完整日期时间
- `rtc_date YYYY-MM-DD` - 仅设置日期
- `rtc_time HH:MM:SS` - 仅设置时间

### 信息命令
- `rtc_info` - 显示RTC设备信息

## 使用示例

```bash
# 读取当前时间
rtc_read

# 设置完整时间
rtc_set 2025-11-28 14:30:00

# 仅设置日期
rtc_date 2025-11-28

# 仅设置时间
rtc_time 14:30:00

# 查看RTC信息
rtc_info

# 查看帮助
rtc_help
```

## 输出格式

### 时间读取输出
```
[RTC] Current time: 2025-11-28 14:30:00
[RTC] Timestamp: 1732786200
[RTC] Weekday: Thursday
```

### 设备信息输出
```
[RTC] Device Information:
  Device name: rtc
  Device type: RTC
  Open count: 2
  Current time: 2025-11-28 14:30:00
  Timestamp: 1732786200
  Clock source: ILO (Internal oscillator)
```

## 技术特性

### 硬件支持
- XMC7100D-F144K4160AA MCU
- 支持WCO和ILO时钟源
- 低功耗运行
- 闹钟功能支持

### 软件特性
- RT-Thread标准RTC接口
- 完整的MSH命令集
- 时间格式验证
- 错误处理和用户友好提示
- 自动设备检测和初始化

### 时间格式
- 输入格式：ISO 8601标准（YYYY-MM-DD HH:MM:SS）
- 输出格式：易读的日期时间字符串
- 支持24小时制
- 自动星期计算

## 编译说明

### 使用SCons编译
```bash
scons
```

### 使用Keil编译
1. 打开project.uvprojx
2. 编译项目
3. 确保RTC相关文件被包含

## 故障排除

### RTC设备未找到
```
[RTC] Error: RTC device not found
```
**解决方案**：检查BSP_USING_RTC是否已定义

### 设备打开失败
```
[RTC] Error: Failed to open RTC device
```
**解决方案**：检查RTC驱动是否正确初始化

### 时间读取失败
```
[RTC] Error: Failed to read time
```
**解决方案**：检查RTC硬件是否正常工作

## 注意事项

1. **时钟源选择**：WCO提供更高精度但需要外部晶振，ILO无需外部元件但精度较低
2. **时间格式**：严格按照指定格式输入，日期和时间用空格分隔
3. **断电保持**：RTC在断电情况下需要后备电池维持时间
4. **初始化**：首次使用可能需要设置初始时间

## 相关文件

- `libraries/HAL_Drivers/drv_rtc.c` - RTC底层驱动
- `libraries/HAL_Drivers/config/drv_config.h` - 驱动配置
- `board/Kconfig` - 硬件配置选项
- `applications/rtc_msh.c/h` - MSH命令接口

## 版本信息

- 驱动版本：基于RT-Thread 5.2.1
- 创建日期：2025-11-28
- 支持MCU：XMC7100D-F144K4160AA
- 兼容性：RT-Thread标准接口

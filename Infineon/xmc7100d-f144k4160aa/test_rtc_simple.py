#!/usr/bin/env python3
"""
RTC驱动集成简化测试脚本
"""

import os

def main():
    print("=== RTC Driver Integration Test ===\n")
    
    # 检查关键文件
    files_to_check = [
        ("applications/rtc_msh.h", "RTC MSH Header"),
        ("applications/rtc_msh.c", "RTC MSH Implementation"),
        ("applications/RTC_README.md", "RTC Documentation"),
        ("applications/SConscript", "Build Configuration"),
        ("board/Kconfig", "Hardware Configuration"),
        ("libraries/HAL_Drivers/drv_rtc.c", "RTC Driver"),
    ]
    
    print("1. File Existence Check:")
    all_exist = True
    for filepath, desc in files_to_check:
        if os.path.exists(filepath):
            print(f"[OK] {desc}: {filepath}")
        else:
            print(f"[FAIL] {desc}: {filepath} - File not found")
            all_exist = False
    
    print()
    
    # 检查Kconfig中的RTC配置
    print("2. Kconfig RTC Configuration:")
    if os.path.exists("board/Kconfig"):
        with open("board/Kconfig", 'r') as f:
            content = f.read()
            if 'BSP_USING_RTC' in content:
                print("[OK] BSP_USING_RTC configuration found")
            else:
                print("[FAIL] BSP_USING_RTC configuration not found")
                all_exist = False
    else:
        print("[FAIL] board/Kconfig not found")
        all_exist = False
    
    print()
    
    # 检查SConscript中的RTC构建配置
    print("3. SConscript Build Configuration:")
    if os.path.exists("applications/SConscript"):
        with open("applications/SConscript", 'r') as f:
            content = f.read()
            if 'rtc_msh.c' in content:
                print("[OK] RTC MSH file in build config")
            else:
                print("[FAIL] RTC MSH file not in build config")
                all_exist = False
    else:
        print("[FAIL] SConscript not found")
        all_exist = False
    
    print()
    
    # 检查RTC MSH实现
    print("4. RTC MSH Implementation:")
    if os.path.exists("applications/rtc_msh.c"):
        with open("applications/rtc_msh.c", 'r') as f:
            content = f.read()
            commands = ['rtc_read', 'rtc_set', 'rtc_date', 'rtc_time', 'rtc_info', 'rtc_help']
            missing_commands = []
            for cmd in commands:
                if cmd in content:
                    print(f"[OK] {cmd} command implemented")
                else:
                    print(f"[FAIL] {cmd} command missing")
                    missing_commands.append(cmd)
            
            if 'MSH_CMD_EXPORT' in content:
                print("[OK] MSH commands exported")
            else:
                print("[FAIL] MSH commands not exported")
                all_exist = False
                
            if missing_commands:
                all_exist = False
    else:
        print("[FAIL] rtc_msh.c not found")
        all_exist = False
    
    print()
    
    # 总结
    print("=== Test Summary ===")
    if all_exist:
        print("SUCCESS: RTC driver integration is complete!")
        print("\nNext steps:")
        print("1. Enable RTC: menuconfig -> Hardware Drivers Config -> Enable RTC")
        print("2. Build project: scons")
        print("3. Flash firmware and test MSH commands")
        print("\nAvailable RTC commands:")
        print("  rtc_read    - Read current time")
        print("  rtc_set     - Set date and time")
        print("  rtc_date    - Set date only")
        print("  rtc_time    - Set time only")
        print("  rtc_info    - Show RTC info")
        print("  rtc_help    - Show help")
    else:
        print("WARNING: RTC driver integration needs fixes")
    
    return all_exist

if __name__ == "__main__":
    success = main()
    exit(0 if success else 1)

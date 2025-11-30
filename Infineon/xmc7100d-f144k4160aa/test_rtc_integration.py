#!/usr/bin/env python3
"""
RTC驱动集成测试脚本
用于验证RTC驱动集成是否正确
"""

import os
import sys
import re

def check_file_exists(filepath, description):
    """检查文件是否存在"""
    if os.path.exists(filepath):
        print(f"✓ {description}: {filepath}")
        return True
    else:
        print(f"✗ {description}: {filepath} - 文件不存在")
        return False

def check_file_content(filepath, patterns, description):
    """检查文件内容是否包含特定模式"""
    if not os.path.exists(filepath):
        print(f"✗ {description}: 文件不存在 {filepath}")
        return False
    
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
            all_found = True
            for pattern, desc in patterns:
                if re.search(pattern, content, re.IGNORECASE):
                    print(f"  ✓ {desc}")
                else:
                    print(f"  ✗ {desc} - 未找到")
                    all_found = False
            return all_found
    except Exception as e:
        print(f"✗ {description}: 读取文件失败 - {e}")
        return False

def main():
    print("=== RTC驱动集成测试 ===\n")
    
    # 检查文件存在性
    print("1. 检查文件存在性:")
    files_to_check = [
        ("applications/rtc_msh.h", "RTC MSH头文件"),
        ("applications/rtc_msh.c", "RTC MSH实现文件"),
        ("applications/RTC_README.md", "RTC说明文档"),
        ("applications/SConscript", "应用程序构建配置"),
        ("board/Kconfig", "硬件配置文件"),
        ("libraries/HAL_Drivers/drv_rtc.c", "RTC底层驱动"),
    ]
    
    files_ok = True
    for filepath, desc in files_to_check:
        if not check_file_exists(filepath, desc):
            files_ok = False
    
    print()
    
    # 检查Kconfig配置
    print("2. 检查Kconfig配置:")
    kconfig_patterns = [
        (r'config\s+BSP_USING_RTC', 'BSP_USING_RTC配置项'),
        (r'select\s+RT_USING_RTC', 'RT_USING_RTC依赖'),
        (r'select\s+RT_USING_ALARM', 'RT_USING_ALARM依赖'),
        (r'choice.*RTC clock source', 'RTC时钟源选择'),
        (r'BSP_RTC_USING_WCO', 'WCO时钟源选项'),
        (r'BSP_RTC_USING_ILO', 'ILO时钟源选项'),
    ]
    
    kconfig_ok = check_file_content("board/Kconfig", kconfig_patterns, "RTC配置选项")
    print()
    
    # 检查SConscript配置
    print("3. 检查SConscript构建配置:")
    scons_patterns = [
        (r'BSP_USING_RTC.*rtc_msh\.c', 'RTC MSH文件条件编译'),
    ]
    
    scons_ok = check_file_content("applications/SConscript", scons_patterns, "构建配置")
    print()
    
    # 检查RTC MSH实现
    print("4. 检查RTC MSH实现:")
    rtc_msh_patterns = [
        (r'rtc_read', 'rtc_read命令实现'),
        (r'rtc_set', 'rtc_set命令实现'),
        (r'rtc_date', 'rtc_date命令实现'),
        (r'rtc_time', 'rtc_time命令实现'),
        (r'rtc_info', 'rtc_info命令实现'),
        (r'rtc_help', 'rtc_help命令实现'),
        (r'MSH_CMD_EXPORT.*rtc_', 'MSH命令导出'),
        (r'INIT_APP_EXPORT', '自动初始化导出'),
        (r'rt_device_find.*rtc', 'RTC设备查找'),
        (r'RT_DEVICE_CTRL_RTC_GET_TIME', 'RTC时间读取控制'),
        (r'RT_DEVICE_CTRL_RTC_SET_TIME', 'RTC时间设置控制'),
    ]
    
    rtc_msh_ok = check_file_content("applications/rtc_msh.c", rtc_msh_patterns, "RTC MSH实现")
    print()
    
    # 检查头文件
    print("5. 检查RTC头文件:")
    header_patterns = [
        (r'rtc_msh_init', '初始化函数声明'),
        (r'#ifndef\s+RTC_MSH_H__', '头文件保护'),
    ]
    
    header_ok = check_file_content("applications/rtc_msh.h", header_patterns, "RTC头文件")
    print()
    
    # 检查底层驱动
    print("6. 检查RTC底层驱动:")
    driver_patterns = [
        (r'rt_hw_rtc_register', 'RTC设备注册'),
        (r'cyhal_rtc_init', 'HAL RTC初始化'),
        (r'RT_DEVICE_CTRL_RTC_GET_TIME', 'RTC读取接口'),
        (r'RT_DEVICE_CTRL_RTC_SET_TIME', 'RTC设置接口'),
        (r'BSP_USING_RTC', 'RTC条件编译'),
    ]
    
    driver_ok = check_file_content("libraries/HAL_Drivers/drv_rtc.c", driver_patterns, "RTC底层驱动")
    print()
    
    # 总结
    print("=== 测试总结 ===")
    all_checks = [
        files_ok, kconfig_ok, scons_ok, 
        rtc_msh_ok, header_ok, driver_ok
    ]
    
    passed = sum(all_checks)
    total = len(all_checks)
    
    print(f"通过的检查: {passed}/{total}")
    print(f"完成度: {passed/total*100:.1f}%")
    
    if passed == total:
        print("\n🎉 RTC驱动集成完全成功！")
        print("\n下一步:")
        print("1. 启用RTC配置: menuconfig → Hardware Drivers Config → Enable RTC")
        print("2. 编译项目: scons")
        print("3. 烧录固件并测试MSH命令")
    else:
        print(f"\n⚠️  RTC驱动集成需要完善 ({total-passed}项需要修复)")
    
    return passed == total

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)

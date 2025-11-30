/*
 * Copyright (c) 2006-2023, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-11-28     Developer    RTC MSH commands header
 */

#ifndef RTC_MSH_H__
#define RTC_MSH_H__

#include <rtthread.h>
#include <rtdevice.h>

/**
 * Initialize RTC MSH commands
 *
 * @return RT_EOK on success, error code on failure
 */
int rtc_msh_init(void);

#endif /* RTC_MSH_H__ */

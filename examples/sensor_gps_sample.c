/*
 * Copyright (c) 2020, RudyLo <luhuadong@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-09-14     luhuadong    the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "gps.h"

#ifndef PKG_USING_GPS_SAMPLE_UART
#define GPS_UART_NAME                    "uart3"
#else
#define GPS_UART_NAME                    PKG_USING_GPS_SAMPLE_UART
#endif

static void read_gps_entry(void *args)
{
    rt_device_t sensor = RT_NULL;
    struct rt_sensor_data sensor_data;

    sensor = rt_device_find(args);
    if (!sensor) 
    {
        rt_kprintf("Can't find GPS device.\n");
        return;
    }

    if (rt_device_open(sensor, RT_DEVICE_FLAG_RDWR))
    {
        rt_kprintf("Open GPS device failed.\n");
        return;
    }

    rt_uint16_t loop = 10;
    while (loop--)
    {
        if (1 != rt_device_read(sensor, 0, &sensor_data, 1))
        {
            rt_kprintf("Read GPS data failed.\n");
            continue;
        }
        rt_kprintf("[%d] PM2.5: %d ug/m3\n", sensor_data.timestamp, sensor_data.data.dust);

        rt_thread_mdelay(3000);
    }

    rt_device_close(sensor);
}

static void dump_gps_entry(void *args)
{
    rt_device_t sensor = RT_NULL;
    struct rt_sensor_data sensor_data;
    struct pms_response resp;
    rt_err_t ret;

    sensor = rt_device_find(args);
    if (!sensor)
    {
        rt_kprintf("Can't find PMS device.\n");
        return;
    }

    if (rt_device_open(sensor, RT_DEVICE_FLAG_RDWR))
    {
        rt_kprintf("Open PMS device failed.\n");
        return;
    }

    rt_uint16_t loop = 10;
    while (loop--)
    {
        ret = rt_device_control(sensor, RT_SENSOR_CTRL_PMS_DUMP, &resp);
        
        if (ret != RT_EOK)
            rt_kprintf("Dump PMS data failed.\n");
        else
            pms_show_response(&resp);

        rt_thread_mdelay(3000);
    }

    rt_device_close(sensor);
}

static int gps_read_sample(void)
{
    rt_thread_t gps_thread;

    gps_thread = rt_thread_create("gps_read", read_gps_entry, 
                                  "gnss_l76", 1024, 
                                  RT_THREAD_PRIORITY_MAX / 2, 20);
    
    if (gps_thread) 
        rt_thread_startup(gps_thread);
}

static int gps_dump_sample(void)
{
    rt_thread_t gps_thread;

    gps_thread = rt_thread_create("gps_dump", dump_gps_entry, 
                                  "gnss_l76", 1024, 
                                  RT_THREAD_PRIORITY_MAX / 2, 20);
    
    if (gps_thread) 
        rt_thread_startup(gps_thread);
}

#ifdef FINSH_USING_MSH
MSH_CMD_EXPORT(gps_dump_sample, dump gps response data);
MSH_CMD_EXPORT(gps_read_sample, read GPS data);
#endif

static int rt_hw_gps_port(void)
{
    struct rt_sensor_config cfg;
    
    cfg.intf.type = RT_SENSOR_INTF_UART;
    cfg.intf.dev_name = GPS_UART_NAME;
    rt_hw_pms_init("l76", &cfg);
    
    return RT_EOK;
}
INIT_COMPONENT_EXPORT(rt_hw_gps_port);
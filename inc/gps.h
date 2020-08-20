/*
 * Copyright (c) 2020, RudyLo <luhuadong@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-08-20     luhuadong    the first version
 */

#ifndef __GPS_H__
#define __GPS_H__

#include <rtthread.h>
#include <rtdevice.h>
#include <sensor.h>

#define GPSLIB_VERSION       "0.0.1"


struct gps_response
{

};
typedef struct gps_response *gps_response_t;

struct gps_device
{
    rt_device_t  serial;
#ifdef PKG_USING_GPS_UART_DMA
    rt_mailbox_t rx_mb;
#else
    rt_sem_t     rx_sem;
#endif
    rt_sem_t     tx_done;
    rt_sem_t     ack;
    rt_thread_t  rx_tid;

    struct gps_response resp;

    rt_mutex_t   lock;
    rt_uint8_t   version;
};
typedef struct gps_device *gps_device_t;


gps_device_t gps_create(const char *uart_name);
void         gps_delete(gps_device_t dev);

rt_uint16_t  gps_read(gps_device_t dev, void *buf, rt_uint16_t size, rt_int32_t time);
rt_uint16_t  gps_wait(gps_device_t dev, void *buf, rt_uint16_t size);
rt_bool_t    gps_is_ready(gps_device_t dev);

void         gps_show_response(gps_response_t resp);
void         gps_dump(const char *buf, rt_uint16_t size);

rt_err_t rt_hw_gps_init(const char *name, struct rt_sensor_config *cfg);

#endif /* __GPS_H__ */
